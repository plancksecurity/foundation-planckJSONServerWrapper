#ifndef JSON_ADAPTER_HH
#define JSON_ADAPTER_HH

#include <thread>
#include <pEp/message.h>
#include <pEp/sync_api.h>
#include "registry.hh"
#include "logger.hh"
#include "server_version.hh"
#include "json_spirit/json_spirit_value.h"


class SessionRegistry;


// allow mocking the JsonAdapter in unittest_rpc
class JsonAdapterBase
{
public:
	~JsonAdapterBase() = default;
	virtual bool verify_security_token(const std::string& s) const = 0;
	
	// Cache a certain function call. See JSON-155.
	virtual void cache(const std::string& client_id, const std::string& fn_name, const std::function<void(PEP_SESSION)>& func) = 0;
};


class JsonAdapter : public JsonAdapterBase
{
public:
	
	// calls shutdown() on the instance if it is still running().
	virtual ~JsonAdapter();
	
	// don't allow copies
	JsonAdapter(const JsonAdapter&) = delete;
	void operator=(const JsonAdapter&) = delete;
	
	// returns all events in queue, if any. Blocks for given number of seconds and returns empty array on timeout
	json_spirit::Array pollForEvents(unsigned timeout_seconds);
	
	// same as above, but with explicit session_id. JUST FOR DEBUG!!!
	json_spirit::Array pollForEvents2(const std::string& session_id, unsigned timeout_seconds);
	
	// set some internal variables and return itself for chaining.
	// these functions shall be called before prepare_run()!
	
	// only if do_sync== true the keysync thread is stared and the keysync callbacks are registered.
	JsonAdapter& ignore_session_errors(bool _ig);
	
	// if called with "false" the JSON Adpapter would no longer deliver HTML and JavaScript files, only handle JSON-RPC requests
	JsonAdapter& deliver_html(bool _deliver_html);
	
	// sets the timeout to drop client's config cache
	JsonAdapter& set_client_session_timeout(int timeout_seconds);
	
	// look for a free port to listen on and set the given configuration
	void prepare_run(const std::string& address, unsigned start_port, unsigned end_port, ::messageToSend_t messageToSend);

	// run the server in another thread and returns immediately. prepare_run() has to be called before!
	void run();
	
	// non-static: does the real work. :-)
	void connection_close_cb();
	
	void close_session(const std::string& session_id);
	
	static
	std::string create_session();
	
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
	
	virtual std::thread::id  get_sync_thread_id() const = 0;
	
	// returns 'true' if 's' is the security token created by the function above.
	virtual bool verify_security_token(const std::string& s) const override;
	
	virtual void cache(const std::string& client_id, const std::string& fn_name, const std::function<void(PEP_SESSION)>& func) override;
	
	// returns the version of the JsonAdapter
	static
	ServerVersion version();
	
	// returns the PEP_SESSION registered for the current thread
	static PEP_SESSION getSessionForThread(const std::string& client_id);
	
	static PEP_STATUS messageToSend(message* msg);
	static PEP_STATUS notifyHandshake(pEp_identity* self, pEp_identity* partner, sync_handshake_signal signal);

	Logger::Stream Log(Logger::Severity s = Logger::Severity::Debug) const;
	
	// will throw logic_error if guard variables contains illegal values, which means: *this is not a valid JsonAdapter object!
	void check_guard() const;
	
	static JsonAdapter& getInstance();
	
	static SessionRegistry& getSessionRegistry();
	
	JsonAdapter& startup(::messageToSend_t messageToSend);

protected:

	// might be overridden for multi-protocol adapters to dispatch to different client types.
	virtual messageToSend_t getMessageToSend() const { return &JsonAdapter::messageToSend; }
	
	// MUST be overridden, because the sync event queue is _not_ part of the Json Adapter
	virtual inject_sync_event_t getInjectSyncEvent() const = 0;
	
	static
	JsonAdapter& createInstance(JsonAdapter* instance);
	
	static
	JsonAdapter* singleton;

	// creates the one and only instance of the JSON adapter.
	JsonAdapter();

//private:
	struct Internal;
	friend struct Internal;
	
	unsigned long long guard_0;
	Internal* i; // pimpl for stable interface.
	unsigned long long guard_1;

private:


	static
	void staticThreadFunc(JsonAdapter* that) { that->threadFunc(); }
	
	void threadFunc();
	std::exception_ptr initExcept;
};

// just for debug:
// returns a string with a pretty-printed JSON array containing the session registry
std::string getSessions();


#endif // JSON_ADAPTER_HH
