#include "json-adapter.hh"
#include <iostream>
#include <boost/program_options.hpp>
namespace po = boost::program_options;

bool debug_mode = false;
std::string address = "127.0.0.1";
unsigned start_port = 4223;
unsigned end_port   = 9999;

void print_version()
{
	std::cout << "pEp JSON Adapter.\n"
		"\tversion \"" << JsonAdapter::version() << "\"\n"
		"\tAPI version " << JsonAdapter::apiVersion() << "\n" 
		"\tpEpEngine version " << get_engine_version() << "\n"
		"\n";
}

int main(int argc, char** argv)
try
{
	po::options_description desc("Program options for the JSON Server Adapter");
	desc.add_options()
		("help,h", "print this help messages")
		("version,v", "print program version")
		("debug,d", po::value<bool>(&debug_mode)->default_value(false), "Run in debug mode, don't fork() in background")
		("start-port,s", po::value<unsigned>(&start_port)->default_value(start_port),  "First port to bind on")
		("end-port,e",   po::value<unsigned>(&end_port)->default_value(end_port),      "Last port to bind on")
		("address,a",    po::value<std::string>(&address)->default_value(address),     "Address to bind on")
	;
	
	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);
	
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

	if( vm.count("debug"))
	{
		JsonAdapter ja( address, start_port, end_port, !debug_mode );
		ja.run();
		
		int input = 0;
		do{
			std::cout << "Press <Q> <Enter> to quit." << std::endl;
			input = std::cin.get();
			std::cout << "Oh, I got a '" << input << "'. \n";
		}while(input != 'q' && input != 'Q');
		
		ja.shutdown(nullptr);
		std::cout << "Good bye. :-)" << std::endl;
	}else{
	
		
	
	}
}
catch (std::exception const &e)
{
	std::cerr << "Exception catched in main(): \"" << e.what() << "\"" << std::endl;
	return 1;
}

