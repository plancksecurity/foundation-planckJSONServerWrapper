#include "session_registry.hh"
#include <sstream>
#include <pEp/call_with_lock.hh>
#include <pEp/status_to_string.hh>
#include <pEp/Adapter.hh>


// creates a PEP_SESSION if none yet exists for the given thread
PEP_SESSION SessionRegistry::get(std::thread::id tid, const std::string& client_id)
{
	Lock L(_mtx);
	
	update_last_use(client_id);
	
	auto q = m.find(tid);
	if(q != m.end())
	{
		Log.debug("get() returns %p.", (const void*)q->second);
		return q->second;
	}
	
	PEP_SESSION session = nullptr;

	PEP_STATUS status = pEp::call_with_lock(&init, &session, mts, ise, pEp::Adapter::_ensure_passphrase);
	if(status != PEP_STATUS_OK)
	{
		throw std::runtime_error("init() fails: " + pEp::status_to_string(status) );
	}

	status = register_sync_callbacks(session, nullptr, nhs, nullptr);
	if(status != PEP_STATUS_OK)
	{
		throw std::runtime_error("register_sync_callbacks() fails: " + pEp::status_to_string(status) );
	}

	m[tid] = session;
	
	const auto& cache_for_client = cache[client_id];
	Log.debug("Apply %zu cached config values for client_id \"%s\" to new session.", cache_for_client.size(), client_id.c_str());
	for(const auto& e : cache_for_client)
	{
		Log.debug("\t %s", e.first.c_str());
		e.second(session);
	}
	
	Log.debug("get() created new session at %p.", (const void*)session);
	return session;
}


void SessionRegistry::remove(std::thread::id tid)
{
	Lock L(_mtx);
	const auto q = m.find(tid);
	if(q != m.end())
	{
		Log.debug("remove() session at %p.", (const void*)q->second);
		pEp::call_with_lock(&release, q->second);
		m.erase(q);
	}else{
		Log.info("remove(): no session for this thread!");
	}
}


void SessionRegistry::for_each(void(*function)(PEP_SESSION))
{
	Lock L(_mtx);
	Log.debug("for_each() on %zu session.", m.size());
	for(const auto& e : m)
	{
		function(e.second);
	}
}


void SessionRegistry::add_to_cache(const std::string& client_id, const std::string& fn_name, const std::function<void(PEP_SESSION)>& func)
{
	Lock L(_mtx);
	Log.debug("add_to_cache(\"%s\", \"%s\")", client_id.c_str(), fn_name.c_str());
	cache[client_id][fn_name] = func;
	update_last_use(client_id);
}


std::string SessionRegistry::to_string() const
{
	Lock L(_mtx);
	std::stringstream ss;
	ss << m.size() << " session" << (m.size()==1?"":"s") << " in registry" << (m.empty()?".\n":":\n");
	for(const auto& e:m)
	{
		ss << "\t" << e.first << ": " << e.second << "\n";
	}
	return ss.str();
}


void SessionRegistry::update_last_use(const std::string& client_id)
{
	const auto now = std::chrono::system_clock::now();
	const auto too_old = now - std::chrono::seconds(client_timeout);
	last_use[client_id] = now;
	
	// TODO: replace by C++20 std::erase_if()
	for(auto q = last_use.begin(); q != last_use.end(); /* no increment here */ )
	{
		if(q->second < too_old)
		{
			cache.erase( q->first );
			q = last_use.erase(q);
		}else{
			++q;
		}
	}
}
