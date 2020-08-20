#include "mini-adapter-impl.hh"
#include "json-adapter.hh"
#include <thread>

#include <pEp/keymanagement.h>
#include <pEp/call_with_lock.hh>
#include <pEp/status_to_string.hh>  // from libpEpAdapter.
#include <pEp/locked_queue.hh>
#include <pEp/Adapter.hh>
#include <pEp/callback_dispatcher.hh>

namespace pEp {
namespace mini {

	typedef std::unique_ptr<std::thread> ThreadPtr;

	// keyserver lookup
	utility::locked_queue< pEp_identity*, &free_identity> keyserver_lookup_queue;
	PEP_SESSION keyserver_lookup_session = nullptr; // FIXME: what if another adapter started it already?
	ThreadPtr   keyserver_lookup_thread;

	int injectIdentity(pEp_identity* idy)
	{
		keyserver_lookup_queue.push_back(idy);
		return 0;
	}
	
	pEp_identity* retrieveNextIdentity(void* /*userdata*/)
	{
		return keyserver_lookup_queue.pop_front();
	}
	
	struct dummy_t{};
	dummy_t dummy{};


void startSync()
{
    pEp::callback_dispatcher.start_sync();
}


void stopSync()
{
	pEp::callback_dispatcher.stop_sync();
}


void startKeyserverLookup()
{
	if(keyserver_lookup_session)
		throw std::runtime_error("KeyserverLookup already started.");

	PEP_STATUS status = pEp::call_with_lock(&init, &keyserver_lookup_session, pEp::CallbackDispatcher::messageToSend, ::pEp::Adapter::_inject_sync_event, pEp::Adapter::_ensure_passphrase);
	if(status != PEP_STATUS_OK || keyserver_lookup_session==nullptr)
	{
		throw std::runtime_error("Cannot create keyserver lookup session! status: " + ::pEp::status_to_string(status));
	}
	
	keyserver_lookup_queue.clear();
	status = register_examine_function(keyserver_lookup_session,
			examineIdentity,
			&keyserver_lookup_session // nullptr is not accepted, so any dummy ptr is used here
			);
	if (status != PEP_STATUS_OK)
		throw std::runtime_error("Cannot register keyserver lookup callbacks! status: " + ::pEp::status_to_string(status));
	
	keyserver_lookup_thread.reset( new std::thread( &keyserverLookupThreadRoutine, &keyserver_lookup_session /* just a dummy */ ) );
}


void stopKeyserverLookup()
{
	// No keyserver lookup session active
	if(keyserver_lookup_session == nullptr)
		return;
	
	keyserver_lookup_queue.push_front(NULL);
	keyserver_lookup_thread->join();

	// there is no unregister_examine_callback() function. hum...
	keyserver_lookup_queue.clear();
	pEp::call_with_lock(&release, keyserver_lookup_session);
	keyserver_lookup_session = nullptr;
}


int examineIdentity(pEp_identity* idy, void* obj)
{
//	JsonAdapter* ja = static_cast<JsonAdapter*>(obj);
	return injectIdentity(idy);
}


void* keyserverLookupThreadRoutine(void* arg)
{
	PEP_STATUS status = do_keymanagement(
		&retrieveNextIdentity,
		arg); // does the whole work
	
	keyserver_lookup_queue.clear();
	return (void*) status;
}


Adapter& Adapter::createInstance()
{
	return dynamic_cast<Adapter&>(JsonAdapter::createInstance( new Adapter() ));
}


std::thread::id  Adapter::get_sync_thread_id() const
{
	return ::pEp::Adapter::sync_thread_id();
}


} // end of namespace pEp::mini
} // end of namespace pEp
