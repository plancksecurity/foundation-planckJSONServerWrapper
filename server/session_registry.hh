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
	
	// calls "init" for the given thread
	PEP_SESSION add(std::thread::id tid = std::this_thread::get_id());
	void     remove(std::thread::id tid = std::this_thread::get_id());
	PEP_SESSION get(std::thread::id tid = std::this_thread::get_id()) const;
	
private:
	std::map<std::thread::id, PEP_SESSION> m;
	messageToSend_t      mts;
	inject_sync_event_t  ise;
	
	typedef std::recursive_mutex     Mutex;
	typedef std::unique_lock<Mutex>  Lock;
	mutable Mutex  _mtx;
};

#endif // JSON_ADAPTER_SESSION_REGISTRY_HH
