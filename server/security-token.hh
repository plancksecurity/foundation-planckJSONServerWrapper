#ifndef SECURITY_TOKEN_HH
#define SECURITY_TOKEN_HH

#include <string>


std::string create_random_token(unsigned length=38);

// deletes the token file, if it exists
void remove_token_file();

// creates a file with restrictive access rights that contains a security token and returns that token, too
std::string create_security_token(const std::string& server_address, unsigned port_nr, const std::string& path);

#endif
