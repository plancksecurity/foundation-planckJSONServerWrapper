#ifndef JSON_ADAPTER_SESSION_REGISTRY_HH
#define JSON_ADAPTER_SESSION_REGISTRY_HH

#include <map>
#include <mutex>
#include <thread>
#include <pEp/pEpEngine.h>

class SessionRegistry
{
public:
	SessionRegistry(messageToSend_t _mts, inject_sync_event_t _ise)
	: mts{_mts}
	, ise{_ise}
	{}
	
	// calls "init" for the given thread if no PEP_SESSION exists, yet for the given thread
	PEP_SESSION get(std::thread::id tid = std::this_thread::get_id());
	void     remove(std::thread::id tid = std::this_thread::get_id());
	
	std::size_t  size() const { return m.size();  }
	bool        empty() const { return m.empty(); }
	
	// calls the given function (which might be a lambda or std::function<> or std::bind thingy)
	// on each stored session.
	void for_each(void(*function)(PEP_SESSION));
	
	std::string to_string() const;
	
private:
	std::map<std::thread::id, PEP_SESSION> m;
	messageToSend_t      mts;
	inject_sync_event_t  ise;
	
	typedef std::recursive_mutex     Mutex;
	typedef std::unique_lock<Mutex>  Lock;
	mutable Mutex  _mtx;
};

#endif // JSON_ADAPTER_SESSION_REGISTRY_HH
