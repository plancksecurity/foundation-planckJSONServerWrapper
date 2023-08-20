#ifndef JSON_ADAPTER_SESSION_REGISTRY_HH
#define JSON_ADAPTER_SESSION_REGISTRY_HH

#include <chrono>
#include <map>
#include <mutex>
#include <thread>
#include <functional>
#include <pEp/pEpEngine.h>
#include <pEp/sync_api.h>
#include "logger.hh"

// 19.08.2023/DZ - Add a mandatory `notifyHandshake_t` member.

class SessionRegistry
{
public:
	SessionRegistry(messageToSend_t _mts, notifyHandshake_t _nhs, inject_sync_event_t _ise, int _client_timeout)
	: mts{_mts}
	, nhs{_nhs}
	, ise{_ise}
	, Log{"SR"}
	, client_timeout{_client_timeout} // in seconds
	{}
	
	// calls "init" for the given thread if no PEP_SESSION exists, yet for the given thread
	PEP_SESSION get(std::thread::id tid, const std::string& client_id);
	void     remove(std::thread::id tid = std::this_thread::get_id());
	
	std::size_t  size() const { return m.size();  }
	bool        empty() const { return m.empty(); }
	
	// calls the given function (which might be a lambda or std::function<> or std::bind thingy)
	// on each stored session.
	void for_each(void(*function)(PEP_SESSION));
	
	void add_to_cache(const std::string& client_id, const std::string& fn_name, const std::function<void(PEP_SESSION)>& func);
	
	std::string to_string() const;
	
private:
	std::map<std::thread::id, PEP_SESSION> m;
	messageToSend_t      mts;
	notifyHandshake_t nhs;
	inject_sync_event_t  ise;
	Logger               Log;
	int       client_timeout; // in seconds
	
	// function name -> functor
	typedef
	std::map<std::string, std::function<void(PEP_SESSION)>> cache_per_client_t;
	
	// key=client_id
	std::map<std::string, cache_per_client_t> cache;
	
	std::map<std::string, std::chrono::time_point<std::chrono::system_clock> > last_use;
	
	typedef std::recursive_mutex     Mutex;
	typedef std::unique_lock<Mutex>  Lock;
	mutable Mutex  _mtx;
	
	void update_last_use(const std::string& client_id);
};

#endif // JSON_ADAPTER_SESSION_REGISTRY_HH
