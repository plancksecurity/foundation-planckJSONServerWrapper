#ifndef SECURITY_TOKEN_HH
#define SECURITY_TOKEN_HH

#include <string>

// creates a file with restrictive access rights that contains a security token.
void create_security_token(const std::string& server_address, unsigned port_nr, const std::string& path);

// returns 'true' if 's' is the security token created by the function above.
bool verify_security_token(const std::string& s);

#endif
