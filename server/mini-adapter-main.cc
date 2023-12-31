#include "main.hh"
#include "mini-adapter-impl.hh"

#include "ev_server.hh"
#include "prefix-config.hh"
#include "json-adapter.hh"
#include "daemonize.hh"
#include "logger.hh"

#include <thread>
#include <fstream>
#include <chrono>
#include <iostream>
#include <boost/program_options.hpp>

#include <pEp/call_with_lock.hh>
#include <pEp/status_to_string.hh>
#include <pEp/Adapter.hh>
#include <pEp/callback_dispatcher.hh>


namespace po = boost::program_options;

bool debug_mode = false;
bool foreground = false;
bool do_sync    = false;
bool ignore_missing_session = false;
bool add_sharks = false;
bool no_html    = false;
int  client_session_timeout = 7*60; // in secondds

uintptr_t status_handle = 0;

std::string address = "127.0.0.1";
std::string logfile = "";

unsigned start_port = 4223;
unsigned end_port   = 9999;

// albait not documented, the first PEP_SESSION in a process is special:
// It must be alive as long as any other PEP_SESSIONS are living.
static PEP_SESSION first_session = nullptr;


void print_version()
{
	std::cout << "pEp JSON Adapter.\n"
		"\tversion " << JsonAdapter::version() << "\n"
		"\tpEpEngine version " << get_engine_version() << "\n"
		"\tpEp protocol version " << PEP_VERSION << "\n"
		"\n";
}

std::ostream* my_logfile = nullptr;
std::shared_ptr<std::ostream> real_logfile;


int main(int argc, char** argv)
try
{
#ifdef _WIN32
	std::ios::sync_with_stdio(false);
#endif

	po::options_description desc("Program options for the JSON Server Adapter");
	desc.add_options()
		("help,h", "print this help messages")
		("version,v", "print program version")
		("debug,d"  , po::value<bool>(&debug_mode)->default_value(false), "Run in debug mode, don't fork() in background: --debug true")
        ("foreground,D", po::bool_switch(&foreground), "do not fork and detach terminal")
		("sync"     , po::value<bool>(&do_sync)->default_value(true)    , "Start keysync in an asynchounous thread (default) or not (--sync false)")
		("start-port,s", po::value<unsigned>(&start_port)->default_value(start_port),  "First port to bind on")
		("end-port,e",   po::value<unsigned>(&end_port)->default_value(end_port),      "Last port to bind on")
		("address,a",    po::value<std::string>(&address)->default_value(address),     "Address to bind on")
		("html-directory,H", po::value<boost::filesystem::path>(&ev_server::path_to_html)->default_value(ev_server::path_to_html), "Path to the HTML and JavaScript files")
		("logfile,l", po::value<std::string>(&logfile)->default_value(logfile),   "Name of the logfile. Can be \"stderr\" for log to stderr or empty for no log.")
		("ignore-missing-session", po::bool_switch(&ignore_missing_session), "Ignore when no PEP_SESSION can be created.")
		("add-sharks", po::bool_switch(&add_sharks), "Add sharks to the JSON Adapter.")
		("no-html"   , po::bool_switch(&no_html   ), "Don't deliver HTML and JavaScript files, only accept JSON-RPC calls.")
		("client-timeout", po::value<int>(&client_session_timeout)->default_value(client_session_timeout), "Drop cached client session config after this timeout (in seconds).")
#ifdef _WIN32
		((STATUS_HANDLE), po::value<uintptr_t>(&status_handle)->default_value(0), "Status file handle, for internal use.")
#endif
	;
	
	po::variables_map vm;
	
	try{
		po::store(po::parse_command_line(argc, argv, desc), vm);
		po::notify(vm);
	}catch(const po::error& e)
	{
		std::cerr << "Cannot parse command line: " << e.what() << "\n\n" << desc << std::endl;
		return 2;
	}
	
	if (vm.count("help"))
	{
		std::cout << desc << "\n";
		return 0;
	}
	if (vm.count("version"))
	{
		print_version();
		return 0;
	}
	
	if(logfile.empty())
	{
		Logger::setDefaultTarget(Logger::Target::None);
	}else if(logfile == "stderr")
	{
		Logger::setDefaultTarget(Logger::Target::Console);
	}else{
		Logger::setDefaultTarget(Logger::Target::File);
	}

	Logger::start("JsonAdapter", logfile);
	
	Logger L("main");
	L.info("main logger started");

	if(add_sharks)
	{
		ev_server::addSharks();
	}
	
	if( debug_mode == false && !foreground )
		daemonize (!debug_mode, (const uintptr_t) status_handle);

        pEp::Adapter::session.initialize();
        pEp::callback_dispatcher.add(JsonAdapter::messageToSend, JsonAdapter::notifyHandshake);

	JsonAdapter& ja = pEp::mini::Adapter::createInstance();
	ja.ignore_session_errors( ignore_missing_session)
	  .deliver_html( !no_html )
	  .set_client_session_timeout(client_session_timeout)
	  ;
	/*
	 * FIXME: why are exceptions risen after the instantiation of JsonAdapter
	 *        not caught in the outer try/catch?
	 */

	try
	{
		ja.prepare_run(address, start_port, end_port, pEp::CallbackDispatcher::messageToSend);
		
		if( debug_mode )
		{
			ja.run();
			// run until "Q" from stdin
			int input = 0;
			do{
				std::cout << "Press <Q> <Enter> to quit." << std::endl;
				input = std::cin.get();
				std::cout << "Oh, I got a '" << input << "'. \n";
			}while(std::cin && input != 'q' && input != 'Q');
		}else{
			ja.run();
            if (!foreground)
                daemonize_commit(0);
			do{
				std::this_thread::sleep_for(std::chrono::seconds(3));
			}while(ja.running());
		}
		ja.shutdown(nullptr);
		ja.Log() << "Good bye. :-)";
		pEp::Adapter::session.release();
                pEp::callback_dispatcher.remove(JsonAdapter::messageToSend);
	}
	catch(std::exception const& e)
	{
		std::cerr << "Inner exception caught in main(): \"" << e.what() << "\"" << std::endl;
		daemonize_commit(1);
		exit(1);
	}
	catch (...)
	{
		std::cerr << "Unknown inner exception caught in main()." << std::endl;
		daemonize_commit(1);
		exit(1);
	}
}
catch(std::exception const& e)
{
	std::cerr << "Exception caught in main(): \"" << e.what() << "\"" << std::endl;
	daemonize_commit(1);
	exit(1);
}
catch (...)
{
	std::cerr << "Unknown Exception caught in main()." << std::endl;
	daemonize_commit(20);
	exit(20);
}
