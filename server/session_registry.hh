#ifndef JSON_ADAPTER_SESSION_REGISTRY_HH
#define JSON_ADAPTER_SESSION_REGISTRY_HH

#include <map>
#include <mutex>
#include <thread>
#include <functional>
#include <pEp/pEpEngine.h>
#include "logger.hh"

class SessionRegistry
{
public:
	SessionRegistry(messageToSend_t _mts, inject_sync_event_t _ise)
	: mts{_mts}
	, ise{_ise}
	, Log{"SR"}
	{}
	
	// calls "init" for the given thread if no PEP_SESSION exists, yet for the given thread
	PEP_SESSION get(std::thread::id tid = std::this_thread::get_id());
	void     remove(std::thread::id tid = std::this_thread::get_id());
	
	std::size_t  size() const { return m.size();  }
	bool        empty() const { return m.empty(); }
	
	// calls the given function (which might be a lambda or std::function<> or std::bind thingy)
	// on each stored session.
	void for_each(void(*function)(PEP_SESSION));
	
	void add_to_cache(const std::string& fn_name, const std::function<void(PEP_SESSION)>& func);
	
	std::string to_string() const;
	
private:
	std::map<std::thread::id, PEP_SESSION> m;
	messageToSend_t      mts;
	inject_sync_event_t  ise;
	Logger Log;
	std::map<std::string, std::function<void(PEP_SESSION)>> cache;
	
	typedef std::recursive_mutex     Mutex;
	typedef std::unique_lock<Mutex>  Lock;
	mutable Mutex  _mtx;
};

#endif // JSON_ADAPTER_SESSION_REGISTRY_HH
