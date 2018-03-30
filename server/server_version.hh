#ifndef SERVER_VERSION_HH
#define SERVER_VERSION_HH

#include <string>
#include <ostream>


struct ServerVersion
{
	unsigned major;
	unsigned minor;
	unsigned patch;
	
	ServerVersion(unsigned ma, unsigned mi, unsigned p);
	
	// This version name is updated from time to time when some "important" changes are made.
	// The version name is of the form "(##) name" where ## is a monotonic increasing number.
	std::string name;
	
	const char* const package_version; // must be set via -D, e.g. -D'PACKAGE_VERSION="deb9-0.1"'

	// returns "major.minor.patch"
	std::string major_minor_patch() const;
};


const ServerVersion& server_version();


std::ostream& operator<<(std::ostream& o, const ServerVersion&);

#endif //SERVER_VERSION_HH
