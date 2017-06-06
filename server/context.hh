#ifndef JSON_ADAPTER_CONTEXT_HH
#define JSON_ADAPTER_CONTEXT_HH

#include <string>
#include "json_spirit/json_spirit_value.h"

class Context
{
public:
	virtual ~Context() = default;
	
	virtual bool verify_security_token(const std::string& token) const = 0;
	virtual void augment(json_spirit::Object& returnObject) = 0;
};

#endif // JSON_ADAPTER_CONTEXT_HH
