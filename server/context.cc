#include "context.hh"
#include "logger.hh"
#include "json-adapter.hh"


namespace
{
	Logger& Log()
	{
		static Logger L("ctx");
		return L;
	}
}


bool Context::verify_security_token(const std::string& token) const
{
	return ja->verify_security_token(token);
}


// Cache a certain function call. See JSON-155.
void Context::cache(const std::string& func_name, const std::function<void(PEP_SESSION)>& fn)
{
	ja->cache(func_name, fn);
}


void Context::store(int position, size_t value)
{
	DEBUG_OUT( Log(), "Store value %zu for position %d.", value, position);
	obj_store.emplace( position, value );
}

size_t Context::retrieve(int position)
{
	try{
		const size_t value = obj_store.at(position);
		DEBUG_OUT( Log(), "Retrieve value %zu for position %d.", value, position);
		return value;
	}catch(const std::out_of_range& e)
	{
		Log() << Logger::Error << "There is no value stored at position " << position << "!";
		throw;
	}
}
