#ifndef SECURITY_TOKEN_HH
#define SECURITY_TOKEN_HH

#include <string>

// creates a file with restrictive access rights that contains a security token and returns that token, too
std::string create_security_token(const std::string& server_address, unsigned port_nr, const std::string& path);

#endif
