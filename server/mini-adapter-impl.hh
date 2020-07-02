#ifndef MINI_ADAPTER_IMPL_HH
#define MINI_ADAPTER_IMPL_HH

#include <pEp/keymanagement.h>
#include <pEp/sync_api.h>
#include "json-adapter.hh"
#include <pEp/passphrase_cache.hh>

// FIXME: This should be provided by libpEpAdapter, so not every adapter is required to instantiate its own!
extern pEp::PassphraseCache passphrase_cache;


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

	class Adapter : public JsonAdapter
	{
	public:
		static Adapter& createInstance();
		
	protected:
		virtual inject_sync_event_t getInjectSyncEvent() const override
		{
			return &::pEp::mini::injectSyncMsg;
		}
	};

} // end of namespace pEp::mini
} // end of namespace pEp

#endif // MINI_ADAPTER_IMPL_HH
