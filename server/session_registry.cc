#include "session_registry.hh"
#include <sstream>
#include <pEp/call_with_lock.hh>
#include <pEp/status_to_string.hh>
#include "logger.hh"

namespace
{
	Logger Log("SR");
}


// creates a PEP_SESSION if none yet exists for the given thread
PEP_SESSION SessionRegistry::get(std::thread::id tid)
{
	Lock L(_mtx);
	
	auto q = m.find(tid);
	if(q != m.end())
	{
		Log.debug("get() returns %p.", (const void*)q->second);
		return q->second;
	}
	
	PEP_SESSION session = nullptr;
	PEP_STATUS status = pEp::call_with_lock(&init, &session, mts, ise);
	if(status != PEP_STATUS_OK)
	{
		throw std::runtime_error("init() fails: " + pEp::status_to_string(status) );
	}
	m[tid] = session;
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
