#include "security-token.hh"
#include "json_spirit/json_spirit_value.h"
#include "json_spirit/json_spirit_writer.h"
#include <random>
#include <algorithm>
#include <iterator>


namespace
{
	// 36 alphanumeric characters
	static const char token_alphabet[] = "qaywsxedcrfvtgbzhnujmikolp1234567890POIUZTREWQASDFGHJKLMNBVCXY";
	
	std::string create_random_token(unsigned length=38)
	{
		static std::random_device rd;
		static std::mt19937 gen(rd());
		static std::uniform_int_distribution<> dis( 0, sizeof(token_alphabet)-2 ); // sizeof-2 because the range is a closed interval!
		
		const unsigned left_len = length/2;
		const unsigned right_len = length-left_len;
		
		std::string ret; ret.reserve(length+1);
		
		std::generate_n( std::back_inserter(ret), left_len, [&](){ return token_alphabet[dis(gen)]; } );
		ret += '_';
		std::generate_n( std::back_inserter(ret), right_len, [&](){ return token_alphabet[dis(gen)]; } );
		return ret;
	}

// platform dependent:
#ifdef _WIN32

	#error "Please implement get_token_filename() and write_security_file() for Win32!"

#else

// version for POSIX-compliant systems:

#include <cstdlib>     // for getenv()
#include <sys/types.h> // for creat()
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h> // for unlink()

std::string get_token_filename()
{
	const char* const temp_dir = getenv("TEMP");
	const char* const user_name = getenv("USER");
	
	const std::string ret = std::string(temp_dir ? temp_dir : "/tmp") + "/pEp-json-token-" + std::string( user_name ? user_name : "XXX" ); 
	return ret;
}


void write_security_file(const std::string& content)
{
	const std::string filename = get_token_filename();
	int fd = creat( filename.c_str(), S_IRUSR | S_IWUSR );
	if(fd < 0)
	{
		throw std::runtime_error("Cannot create security token file \"" + filename + "\": " + std::to_string(errno) );
	}
	
	const ssize_t ss = write(fd, content.data(), content.size());
	if(ss<0 || uint64_t(ss)!=content.size())
	{
		throw std::runtime_error("Cannot write into security token file \"" + filename + "\": " + std::to_string(errno));
	}
	close(fd);
}

/*
void remove_token_file()
{
	const std::string filename = get_token_filename();
	unlink(filename.c_str());
}
*/

#endif // ! _WIN32

} // end of anonymous namespace


namespace js = json_spirit;



// creates a file with restrictive access rights that contains a security token.
std::string create_security_token(const std::string& server_address, unsigned port_nr, const std::string& path)
{
	const std::string sec_token = create_random_token();
	
	js::Object o;
	o.emplace_back("address", server_address);
	o.emplace_back("port", uint64_t(port_nr));
	o.emplace_back("path", path);
	o.emplace_back("security_token", sec_token );
	
	const std::string content = js::write( o, js::pretty_print | js::raw_utf8 ) + '\n';
	write_security_file( content );
	
	return sec_token;
}


