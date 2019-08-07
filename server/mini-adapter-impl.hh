#ifndef MINI_ADAPTER_IMPL_HH
#define MINI_ADAPTER_IMPL_HH

#include <pEp/keymanagement.h>
#include <pEp/sync_api.h>


namespace pEp{
namespace mini {

	int injectSyncMsg(Sync_event* msg, void* /*management*/ );
	
	int injectIdentity(pEp_identity* idy);
	
	Sync_event* retrieveNextSyncMsg(void* /*management*/,  unsigned timeout);
	
	pEp_identity* retrieveNextIdentity( void* /*management*/);
	
	void* syncThreadRoutine(void* arg);

	void startSync();
	void stopSync();

	void startKeyserverLookup();
	void stopKeyserverLookup();

	int examineIdentity(pEp_identity* idy, void* obj);

	void* keyserverLookupThreadRoutine(void* arg);

} // end of namespace pEp::mini
} // end of namespace pEp

#endif // MINI_ADAPTER_IMPL_HH
