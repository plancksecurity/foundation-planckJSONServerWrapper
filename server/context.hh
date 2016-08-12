#ifndef JSON_ADAPTER_CONTEXT_HH
#define JSON_ADAPTER_CONTEXT_HH

#include <string>

class Context
{
public:
	virtual ~Context() = default;
	
	virtual bool verify_security_token(const std::string& token) const = 0;
};

#endif // JSON_ADAPTER_CONTEXT_HH
