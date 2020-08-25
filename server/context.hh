#ifndef JSON_ADAPTER_CONTEXT_HH
#define JSON_ADAPTER_CONTEXT_HH

#include <map>
#include <pEp/pEpEngine.h>


class JsonAdapterBase;

class Context
{
public:
	Context(JsonAdapterBase* _ja) : ja{_ja} {}
	
	Context(const Context&) = delete;
	void operator=(const Context&) = delete;
	
	// delegate call to the 'ja' member
	bool verify_security_token(const std::string& token) const ;
	
	// Cache a certain function call. See JSON-155.
	// delegate call to the 'ja' member
	void cache(const std::string& func_name, const std::function<void(PEP_SESSION)>& fn);
	
	// store and retrieve other parameters into the context.
	// that allows semantic actions based on other function parameters
	// KISS: at the moment only "size_t" objects are supported.
	void store(int position, size_t value);
	size_t retrieve(int position);
	void clear();

private:
	std::map<int, size_t> obj_store;
	JsonAdapterBase* ja;
};

#endif // JSON_ADAPTER_CONTEXT_HH
