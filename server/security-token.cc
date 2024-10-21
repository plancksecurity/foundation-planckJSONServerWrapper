#include "security-token.hh"
#include "json_spirit/json_spirit_value.h"
#include "json_spirit/json_spirit_writer.h"
#include <random>
#include <algorithm>
#include <iterator>

#include <cstdlib>     // for getenv()
#include <boost/filesystem.hpp>
#include <pEp/pEpEngine.h> // for per_user_directory()

namespace fs = boost::filesystem;

namespace
{
	// 36 alphanumeric characters
	static const char token_alphabet[] = "qaywsxedcrfvtgbzhnujmikolp1234567890POIUZTREWQASDFGHJKLMNBVCXY";

}

	std::string create_random_token(unsigned length)
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
#include <fstream>
fs::path get_token_filename()
{
	// Get the directory from the pEp Engine.
	return per_user_directory() / fs::path("json-token");
}

void write_security_file(const std::string& content)
{
	const fs::path filename = get_token_filename();
	const fs::path parent = filename.parent_path();
	fs::create_directories(parent);
	std::string secfn = (fs::canonical(parent)/filename).string();
	std::ofstream secfile(secfn);
	secfile << content;
	secfile.close();
}

#else

// version for POSIX-compliant systems:
#include <sys/types.h> // for creat()
#include <sys/stat.h>
#include <fcntl.h>

fs::path get_token_filename()
{
	const char* const pEp_home_dir = per_user_directory();
	if(pEp_home_dir == nullptr)
	{
		throw std::runtime_error("per_user_directory() fails.");
	}
	
	const fs::path pEp_dir = fs::path(pEp_home_dir);
	boost::system::error_code ec;
	fs::create_directory( pEp_dir );
	
	fs::permissions( pEp_dir, fs::perms(0700), ec);
	if(ec)
	{
		throw boost::system::system_error(ec, "Cannot change permissions of pEp home directory to 0700 ");
	}
	
	return  pEp_dir / "json-token";
}


void write_security_file(const std::string& content)
{
	const fs::path filename = get_token_filename();
	
	// TODO: how to atomically create a file with restricted permissions using boost::filesystem?
	int fd = creat( filename.c_str(), S_IRUSR | S_IWUSR );
	if(fd < 0)
	{
		throw std::runtime_error("Cannot create security token file \"" + filename.string() + "\": " + std::to_string(errno) );
	}
	
	const ssize_t ss = write(fd, content.data(), content.size());
	if(ss<0 || uint64_t(ss)!=content.size())
	{
		throw std::runtime_error("Cannot write into security token file \"" + filename.string() + "\": " + std::to_string(errno));
	}
	close(fd);
}

#endif // ! _WIN32



namespace js = json_spirit;

// creates a file with restrictive access rights that contains a security token.
std::string create_security_token(const std::string& server_address, unsigned port_nr, const std::string& path)
{
	const std::string sec_token = create_random_token(38);
	
	js::Object o;
	o.emplace_back("address", server_address);
	o.emplace_back("port", uint64_t(port_nr));
	o.emplace_back("path", path);
	o.emplace_back("security_token", sec_token );
	
	const std::string content = js::write( o, js::pretty_print | js::raw_utf8 ) + '\n';
	write_security_file( content );
	
	return sec_token;
}


void remove_token_file()
{
	const fs::path filename = get_token_filename();
	fs::remove(filename.c_str());
}
