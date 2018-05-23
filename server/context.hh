#ifndef JSON_ADAPTER_CONTEXT_HH
#define JSON_ADAPTER_CONTEXT_HH

#include <string>
#include "json_spirit/json_spirit_value.h"
#include <map>

class Context
{
public:
	virtual ~Context() = default;
	
	virtual bool verify_security_token(const std::string& token) const = 0;
	virtual void augment(json_spirit::Object& returnObject) = 0;
	
	// store and retrieve other parameters into the context.
	// that allows semantic actions based on other function parameters
	virtual void store(int position, json_spirit::Object& obj);
	virtual json_spirit::Object& retrieve(int position);
	virtual void clear();

private:
	std::map<int, std::reference_wrapper<json_spirit::Object>> obj_store;
};

#endif // JSON_ADAPTER_CONTEXT_HH
