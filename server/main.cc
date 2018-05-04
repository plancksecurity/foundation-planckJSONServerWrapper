#include "main.hh"
#include "ev_server.hh"
#include "prefix-config.hh"
#include "json-adapter.hh"
#include "daemonize.hh"
#include "nulllogger.hh"

#include <thread>
#include <fstream>
#include <chrono>
#include <iostream>
#include <boost/program_options.hpp>

namespace po = boost::program_options;

bool debug_mode = false;
bool do_sync    = false;
bool ignore_missing_session = false;
bool add_sharks = false;

uintptr_t status_handle = 0;

std::string address = "127.0.0.1";
std::string logfile = "";

unsigned start_port = 4223;
unsigned end_port   = 9999;


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
		("sync"     , po::value<bool>(&do_sync)->default_value(true)    , "Start keysync in an asynchounous thread (default) or not (--sync false)")
		("start-port,s", po::value<unsigned>(&start_port)->default_value(start_port),  "First port to bind on")
		("end-port,e",   po::value<unsigned>(&end_port)->default_value(end_port),      "Last port to bind on")
		("address,a",    po::value<std::string>(&address)->default_value(address),     "Address to bind on")
		("html-directory,H", po::value<boost::filesystem::path>(&ev_server::path_to_html)->default_value(ev_server::path_to_html), "Path to the HTML and JavaScript files")
		("logfile,l", po::value<std::string>(&logfile)->default_value(logfile),   "Name of the logfile. Can be \"stderr\" for log to stderr or empty for no log.")
		("ignore-missing-session", po::bool_switch(&ignore_missing_session), "Ignore when no PEP_SESSION can be created.")
		("add-sharks", po::bool_switch(&add_sharks), "Add sharks to the JSON Adapter.")
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
		my_logfile = &nulllogger;
	}else if(logfile == "stderr")
	{
		my_logfile = &std::cerr;
	}else{
		real_logfile = std::make_shared<std::ofstream>( logfile, std::ios::app );
		my_logfile = real_logfile.get();
	}
	
	if(add_sharks)
	{
		ev_server::addSharks();
	}
	
	if( debug_mode == false )
		daemonize (!debug_mode, (const uintptr_t) status_handle);

	JsonAdapter ja( my_logfile );
	ja.do_sync( do_sync)
	  .ignore_session_errors( ignore_missing_session)
	  ;
	/*
	 * FIXME: why are exceptions risen after the instantiation of JsonAdapter
	 *        not catched in the outer try/catch?
	 */

	try
	{
		ja.prepare_run(address, start_port, end_port);

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
			daemonize_commit(0);
			do{
				std::this_thread::sleep_for(std::chrono::seconds(3));
			}while(ja.running());
		}
		ja.shutdown(nullptr);
		ja.Log() << "Good bye. :-)" << std::endl;
		JsonAdapter::global_shutdown();
	}
	catch (...)
	{
		daemonize_commit(1);
		exit(1);
	}
}
catch(std::exception const& e)
{
	std::cerr << "Exception caught in main(): \"" << e.what() << "\"" << std::endl;
	daemonize_commit(1);
	return 1;
}
catch (...)
{
	std::cerr << "Unknown Exception caught in main()." << std::endl;
	daemonize_commit(20);
	return 20;
}

