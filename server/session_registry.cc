#include "session_registry.hh"
#include <sstream>
#include <pEp/call_with_lock.hh>
#include <pEp/status_to_string.hh>


// creates a PEP_SESSION if none yet exists for the given thread
PEP_SESSION SessionRegistry::get(std::thread::id tid)
{
	Lock L(_mtx);
	
	auto q = m.find(tid);
	if(q != m.end())
	{
		return q->second;
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
