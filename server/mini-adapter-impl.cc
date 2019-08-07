#include "mini-adapter-impl.hh"
#include "json-adapter.hh"
#include <thread>

#include <pEp/keymanagement.h>
#include <pEp/call_with_lock.hh>
#include <pEp/status_to_string.hh>  // from libpEpAdapter.
#include <pEp/locked_queue.hh>


namespace pEp {
namespace mini {

	typedef std::unique_ptr<std::thread> ThreadPtr;

	// keyserver lookup
	utility::locked_queue< pEp_identity*, &free_identity> keyserver_lookup_queue;
	PEP_SESSION keyserver_lookup_session = nullptr; // FIXME: what if another adapter started it already?
	ThreadPtr   keyserver_lookup_thread;

	utility::locked_queue< Sync_event*, &free_Sync_event>  sync_queue;
	PEP_SESSION sync_session = nullptr;
	ThreadPtr   sync_thread;
	
	
	int injectSyncMsg(Sync_event* msg, void* /* management */)
	{
		sync_queue.push_back(msg);
		return 0;
	}
	
	int injectIdentity(pEp_identity* idy)
	{
		keyserver_lookup_queue.push_back(idy);
		return 0;
	}
	
	Sync_event* retrieveNextSyncMsg(void* /*management*/, unsigned timeout)
	{
		Sync_event* msg = nullptr;
		if(timeout)
		{
			const bool success = sync_queue.try_pop_front(msg, std::chrono::seconds(timeout));
			if(!success)
			{
				// this is timeout occurrence
				return new_sync_timeout_event();
			}
		}else{
			msg = sync_queue.pop_front();
		}
		return msg;
	}
	
	pEp_identity* retrieveNextIdentity(void* /*userdata*/)
	{
		return keyserver_lookup_queue.pop_front();
	}
	
	void* syncThreadRoutine(void* arg)
	{
		PEP_STATUS status = pEp::call_with_lock(&init, &sync_session, &JsonAdapter::messageToSend, &injectSyncMsg);
		if (status != PEP_STATUS_OK)
			throw std::runtime_error("Cannot init sync_session! status: " + ::pEp::status_to_string(status));
		
		status = register_sync_callbacks(sync_session,
		                                 (void*) -1,
		                                 &JsonAdapter::notifyHandshake,
		                                 &retrieveNextSyncMsg);
		if (status != PEP_STATUS_OK)
			throw std::runtime_error("Cannot register sync callbacks! status: " + ::pEp::status_to_string(status));
		
		status = do_sync_protocol(sync_session, arg); // does the whole work
		sync_queue.clear(); // remove remaining messages
		return (void*) status;
	}


void startSync()
{
	sync_queue.clear();
	
//	status = attach_sync_session(i->session, i->sync_session);
//	if(status != PEP_STATUS_OK)
//		throw std::runtime_error("Cannot attach to sync session! status: " + status_to_string(status));
	
	sync_thread.reset( new std::thread( syncThreadRoutine, nullptr ) );
}


void stopSync()
{
	// No sync session active
	if(sync_session == nullptr)
		return;
	
	sync_queue.push_front(NULL);
	sync_thread->join();
	
	unregister_sync_callbacks(sync_session);
	sync_queue.clear();
	
	pEp::call_with_lock(&release, sync_session);
	sync_session = nullptr;
}


void startKeyserverLookup()
{
	if(keyserver_lookup_session)
		throw std::runtime_error("KeyserverLookup already started.");

	PEP_STATUS status = pEp::call_with_lock(&init, &keyserver_lookup_session, &JsonAdapter::messageToSend, &injectSyncMsg);
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

} // end of namespace pEp::mini
} // end of namespace pEp
