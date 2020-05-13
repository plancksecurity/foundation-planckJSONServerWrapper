#include "session_registry.hh"
#include <sstream>
#include <pEp/call_with_lock.hh>
#include <pEp/status_to_string.hh>


// calls "init" for the given thread
PEP_SESSION SessionRegistry::add(std::thread::id tid)
{
	Lock L(_mtx);
	
	if(m.count(tid) > 0)
	{
		std::stringstream ss; ss << tid;
		throw std::runtime_error("There is already a session for thread " + ss.str() + "!");
	}
	
	PEP_SESSION session = nullptr;
	PEP_STATUS status = pEp::call_with_lock(&init, &session, mts, ise);
	if(status != PEP_STATUS_OK)
	{
		throw std::runtime_error("init() fails: " + pEp::status_to_string(status) );
	}
	m[tid] = session;
	return session;
}


void SessionRegistry::remove(std::thread::id tid)
{
	Lock L(_mtx);
	const auto q = m.find(tid);
	if(q != m.end())
	{
		pEp::call_with_lock(&release, q->second);
		m.erase(q);
	}
}


PEP_SESSION SessionRegistry::get(std::thread::id tid) const
{
	Lock L(_mtx);
	auto q = m.find(tid);
	return (q==m.end()) ? nullptr : q->second;
}
