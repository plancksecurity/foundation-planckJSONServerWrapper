#include "json-adapter.hh"
#include <iostream>


std::string address = "127.0.0.1";
unsigned start_port = 4223;
unsigned end_port   = 9999;

int main(int argc, char** argv)
try
{
	JsonAdapter ja( address, start_port, end_port );
	
	ja.run();
	
	int input = 0;
	do{
		std::cout << "Press <Q> <Enter> to quit." << std::endl;
		input = std::cin.get();
		std::cout << "Oh, I got a '" << input << "'. \n";
	}while(input != 'q' && input != 'Q');
	
	ja.shutdown(nullptr);
	std::cout << "Good bye. :-)" << std::endl;
}
catch (std::exception const &e)
{
	std::cerr << "Exception catched in main(): \"" << e.what() << "\"" << std::endl;
	return 1;
}

