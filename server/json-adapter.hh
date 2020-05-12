#ifndef JSON_ADAPTER_HH
#define JSON_ADAPTER_HH

#include <pEp/message.h>
#include <pEp/sync_api.h>
#include "registry.hh"
#include "context.hh"
#include "logger.hh"
#include "server_version.hh"


class JsonAdapter : public Context
{
public:
	
	// calls shutdown() on the instance if it is still running().
	virtual ~JsonAdapter();
	
	// don't allow copies
	JsonAdapter(const JsonAdapter&) = delete;
	void operator=(const JsonAdapter&) = delete;
	
	void   registerEventListener(const std::string& address, unsigned port, const std::string& securityContext);
	void unregisterEventListener(const std::string& address, unsigned port, const std::string& securityContext);
	
	// set some internal variables and return itself for chaining.
	// these functions shall be called before prepare_run()!
	
	// only if do_sync== true the keysync thread is stared and the keysync callbacks are registered.
	JsonAdapter& ignore_session_errors(bool _ig);
	
	// if called with "false" the JSON Adpapter would no longer deliver HTML and JavaScript files, only handle JSON-RPC requests
	JsonAdapter& deliver_html(bool _deliver_html);
	
	// look for a free port to listen on and set the given configuration
	void prepare_run(const std::string& address, unsigned start_port, unsigned end_port, inject_sync_event_t se);
	
	// run the server in another thread and returns immediately. prepare_run() has to be called before!
	void run();
	
	// exits gracefully after the given number of seconds.
	// if "tv" is NULL it means: exits immediately after _all_ currently processed events have been finished.
	void shutdown(struct timeval* tv);
	
	void shutdown_now() { shutdown(nullptr); }
	
	// exit immediately or after the currently processed event has been finished.
	//void abort();
	
	// returns "true" after run() was called, until shutdown() or abort() is called.
	bool running() const;
	
	unsigned port() const;
	const std::string& address() const;
	
	unsigned request_count() const;
	
	// returns 'true' if 's' is the security token created by the function above.
	virtual bool verify_security_token(const std::string& s) const override;
	
	virtual void augment(json_spirit::Object& returnObject) override;
	
	// returns the version of the JsonAdapter
	static
	ServerVersion version();
	
	// returns the PEP_SESSION registered for the current thread
	static PEP_SESSION getSessionForThread();
	
	static PEP_STATUS messageToSend(message* msg);
	static PEP_STATUS notifyHandshake(pEp_identity* self, pEp_identity* partner, sync_handshake_signal signal);

	Logger::Stream Log(Logger::Severity s = Logger::Severity::Debug) const;
	
	// will throw logic_error if guard variables contains illegal values, which means: *this is not a valid JsonAdapter object!
	void check_guard() const;
	
	static JsonAdapter& getInstance();
	
	// Very ugly: that function ptr has to be known to the JA before any instance is created. -.-
	static JsonAdapter& startup(inject_sync_event_t inject_fn);

//private:
	struct Internal;
	unsigned long long guard_0;
	Internal* i; // pimpl for stable interface.
	unsigned long long guard_1;

private:

	// creates the one and only instance of the JSON adapter.
	JsonAdapter();

	static
	void staticThreadFunc(JsonAdapter* that) { that->threadFunc(); }
	
	void threadFunc();
	std::exception_ptr initExcept;
};

// just for debug:
// returns a string with a pretty-printed JSON array containing the session registry
std::string getSessions();


#endif // JSON_ADAPTER_HH
