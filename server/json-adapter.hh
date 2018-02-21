#ifndef JSON_ADAPTER_HH
#define JSON_ADAPTER_HH

#include <pEp/message.h>
#include <pEp/sync.h>
#include "registry.hh"
#include "context.hh"
#include "server_version.hh"


class JsonAdapter : public Context
{
public:
	// creates an instance of the JSON adapter. It tries to bind the first available port in the given range
	JsonAdapter(std::ostream* logfile);
	
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
	JsonAdapter& do_sync(bool _do_sync);
	JsonAdapter& ignore_session_errors(bool _ig);
	
	// look for a free port to listen on
	void prepare_run(const std::string& address, unsigned start_port, unsigned end_port);
	
	// run the server in another thread and returns immediately.
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

	static PEP_STATUS messageToSend(void* obj, message* msg);
	static PEP_STATUS notifyHandshake(void* obj, pEp_identity* self, pEp_identity* partner, sync_handshake_signal signal);

	// BEWARE: msg is 1st parameter, obj is second!!!
	static int injectSyncMsg(void* msg, void* obj);
	static void* retrieveNextSyncMsg(void* obj, time_t* timeout);
	static void* syncThreadRoutine(void* arg);
	
	void startSync();
	void stopSync();
	
	static void startKeyserverLookup();
	static void stopKeyserverLookup();
	
	static int examineIdentity(pEp_identity*, void* obj);
	static pEp_identity* retrieveNextIdentity(void* obj);
	static void* keyserverLookupThreadRoutine(void* arg);
	
	// returns the associated log stream (either std::cerr or nulllogger)
	std::ostream& Log() const;
	
	// will throw logic_error if guard variables contains illegal values, which means: *this is not a valid JsonAdapter object!
	void check_guard() const;

//private:
	struct Internal;
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
