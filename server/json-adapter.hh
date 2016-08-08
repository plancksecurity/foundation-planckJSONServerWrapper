#ifndef JSON_ADAPTER_HH
#define JSON_ADAPTER_HH

#include <pEp/pEpEngine.h>
#include "registry.hh"

class JsonAdapter
{
public:
	// creates an instance of the JSON adapter. It tries to bind the first available port in the given range
	// throws std::runtime_error if no port cannot be bound.
	JsonAdapter(const std::string& address, unsigned start_port, unsigned end_port);
	
	// calls abort() on the instance if it is still running().
	~JsonAdapter();
	
	// don't allow copies
	JsonAdapter(const JsonAdapter&) = delete;
	void operator=(const JsonAdapter&) = delete;
	
	// run the server in another thread and returns immediately.
	void run();
	
	// exits gracefully after the given number of seconds.
	// if "tv" is NULL it means: exits immediately after _all_ currently processed events have been finished.
	void shutdown(struct timeval* tv);
	
	// exit immediately or after the currently processed event has been finished.
	void abort();
	
	// returns "true" after run() was called, until shutdown() or abort() is called.
	bool running() const;
	
	unsigned port() const;
	const std::string& address() const;
	
	unsigned request_count() const;
	
	// returns 'true' if 's' is the security token created by the function above.
	bool verify_security_token(const std::string& s) const;
	
	const std::string& sec_token() const;
	
	static
	unsigned apiVersion();
	
	// returns a version name
	static
	std::string version();

private:
	struct Internal;
	Internal* i; // pimpl for stable interface.
};

// just for debug:
// returns a string with a pretty-printed JSON array containing the session registry
std::string getSessions();


#endif // JSON_ADAPTER_HH
