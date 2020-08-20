#ifndef MINI_ADAPTER_IMPL_HH
#define MINI_ADAPTER_IMPL_HH

#include <pEp/keymanagement.h>
#include <pEp/sync_api.h>
#include "json-adapter.hh"
#include <pEp/passphrase_cache.hh>
#include <pEp/Adapter.hh>


namespace pEp{
namespace mini {

	int injectIdentity(pEp_identity* idy);
	
	pEp_identity* retrieveNextIdentity( void* /*management*/);
	
	void startSync();
	void stopSync();

	void startKeyserverLookup();
	void stopKeyserverLookup();

	int examineIdentity(pEp_identity* idy, void* obj);

	void* keyserverLookupThreadRoutine(void* arg);

	class Adapter : public JsonAdapter
	{
	public:
		static Adapter&  createInstance();
		std::thread::id  get_sync_thread_id() const override;
		
	protected:
		virtual inject_sync_event_t getInjectSyncEvent() const override
		{
			return &::pEp::Adapter::_inject_sync_event;
		}
	};

} // end of namespace pEp::mini
} // end of namespace pEp

#endif // MINI_ADAPTER_IMPL_HH
