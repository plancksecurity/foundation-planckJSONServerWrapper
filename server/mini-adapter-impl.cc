#include "mini-adapter-impl.hh"
#include "json-adapter.hh"
#include "pEp-utils.hh" // for make_c_ptr() wrapper
#include <thread>

#include <pEp/keymanagement.h>
#include <pEp/call_with_lock.hh>
#include <pEp/status_to_string.hh>  // from libpEpAdapter.
#include <pEp/locked_queue.hh>


namespace pEp {
namespace mini {

	static
	void release_with_lock(PEP_SESSION s)
	{
		pEp::call_with_lock(&release, s);
	}

	typedef std::unique_ptr<std::thread> ThreadPtr;

	// keyserver lookup
	::utility::locked_queue< pEp_identity*, &free_identity> keyserver_lookup_queue;
	ThreadPtr   keyserver_lookup_thread;

	::utility::locked_queue< Sync_event*, &free_Sync_event>  sync_queue;
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
	
	void syncThreadRoutine()
	try{
		Logger L(Log(), "syncTR");
		PEP_SESSION sync_session = nullptr;
		
		PEP_STATUS status = pEp::call_with_lock(&init, &sync_session, &JsonAdapter::messageToSend, &injectSyncMsg);
		if (status != PEP_STATUS_OK)
			throw std::runtime_error("Cannot init sync_session! status: " + ::pEp::status_to_string(status));
		
		L << Logger::Info << "sync_session initialized. Session = " << (void*)sync_session;
		auto sync_session_wrapper = pEp::utility::make_c_ptr(sync_session, &release_with_lock);
		
		status = register_sync_callbacks(sync_session,
		                                 (void*) -1,
		                                 &JsonAdapter::notifyHandshake,
		                                 &retrieveNextSyncMsg);
		if (status != PEP_STATUS_OK)
		{
			L << Logger::Error << "Cannot register sync callbacks! status: " << ::pEp::status_to_string(status);
			throw std::runtime_error("Cannot register sync callbacks! status: " + ::pEp::status_to_string(status));
		}
		
		L.info("sync callbacks registered. Now call do_sync_protocol()");
		status = do_sync_protocol(sync_session, (void*)"Dummy!"); // does the whole work
		
		L << Logger::Info << "do_sync_protocol() returned with status " << pEp::status_to_string(status) << ".  " << sync_queue.size() << " elements in sync_queue.";
		sync_queue.clear(); // remove remaining messages
		
		unregister_sync_callbacks(sync_session);
		
		// release(sync_session is not necessary due to sync_session_wrapper! .-D
		DEBUG_OUT(L, "I am done.");
	}catch(std::exception& e)
	{
		Log().error(std::string("Got std::exception in syncThreadRoutine: ") + e.what() );
	}catch(...)
	{
		Log().error("Got unknown exception in syncThreadRoutine");
	}


void startSync()
{
	Logger L(Log(), "startSync");
	if(sync_thread)
	{
		L.info("sync_thread already exists. Therefore: Nothing to do. :-)");
		return;
	}
	
	L << Logger::Info << "Remove " << sync_queue.size() << " elements from sync_queue";
	sync_queue.clear();
	
	L.info("Start sync thread");
	sync_thread.reset( new std::thread(syncThreadRoutine) );
	L.info("Done.");
}


void stopSync()
{
	Logger L(Log(), "stopSync");

	if(sync_thread)
	{
		DEBUG_OUT(L, "Joining sync_thread... waiting for sync_thread to quit");
		
		// this triggers do_sync_protocol() to quit
		sync_queue.push_front(NULL);
		sync_thread->join();
		sync_thread.reset(nullptr);
		DEBUG_OUT(L, "sync_thread is NULL now.");
	}else{
		DEBUG_OUT(L, "sync_thread is already NULL. Okay, nothing to do.");
	}
	
	sync_queue.clear();
	DEBUG_OUT(L, "Done.");
}


void startKeyserverLookup()
{
	if(keyserver_lookup_thread)
		throw std::runtime_error("KeyserverLookup already started.");
	
	keyserver_lookup_thread.reset( new std::thread( keyserverLookupThreadRoutine ) );
}


void stopKeyserverLookup()
{
	// No keyserver lookup session active
	if(!keyserver_lookup_thread)
		return;
	
	keyserver_lookup_queue.push_front(NULL);
	keyserver_lookup_thread->join();

}


int examineIdentity(pEp_identity* idy, void* obj)
{
//	JsonAdapter* ja = static_cast<JsonAdapter*>(obj);
	return injectIdentity(idy);
}


void keyserverLookupThreadRoutine()
{
	Logger L("keysrvLookupThreadRoutine");
	PEP_SESSION keyserver_lookup_session = nullptr;

	PEP_STATUS status = pEp::call_with_lock(&init, &keyserver_lookup_session, &JsonAdapter::messageToSend, &injectSyncMsg);
	if(status != PEP_STATUS_OK || keyserver_lookup_session==nullptr)
	{
		L.error("Cannot create keyserver lookup session! status: " + ::pEp::status_to_string(status));
		return;
	}
	
	auto keyserver_session_wrapper = pEp::utility::make_c_ptr(keyserver_lookup_session, &release_with_lock);
	
	keyserver_lookup_queue.clear();
	status = register_examine_function(keyserver_lookup_session,
			examineIdentity,
			&keyserver_lookup_session // nullptr is not accepted, so any dummy ptr is used here
			);
	if (status != PEP_STATUS_OK)
	{
		L.error("Cannot register keyserver lookup callbacks! status: " + ::pEp::status_to_string(status));
		return;
	}
	
	status = do_keymanagement(&retrieveNextIdentity, (void*)"Dummy!"); // does the whole work
	if (status != PEP_STATUS_OK)
	{
		L.error("do_keymanagement() returns with status: " + ::pEp::status_to_string(status));
		return;
	}

	// there is no unregister_examine_callback() function. hum...
	// release(keyserver_lookup_session is not necessary due to keyserver_session_wrapper! .-D
	keyserver_lookup_queue.clear();
}


Logger& Log()
{
	static Logger L("mini");
	return L;
}


} // end of namespace pEp::mini
} // end of namespace pEp
