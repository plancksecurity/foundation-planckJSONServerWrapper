#ifndef MINI_ADAPTER_IMPL_HH
#define MINI_ADAPTER_IMPL_HH

#include <pEp/sync_api.h>
#include "json-adapter.hh"
#include <pEp/Adapter.hh>

namespace pEp{
namespace mini {

	void startSync();
	void stopSync();

	class Adapter : public JsonAdapter
	{
	public:
		static Adapter&  createInstance();
		std::thread::id  get_sync_thread_id() const override;
		
	protected:
		virtual inject_sync_event_t getInjectSyncEvent() const override
		{
			return &::pEp::Adapter::_cb_inject_sync_event_enqueue_sync_event;
		}
	};

} // end of namespace pEp::mini
} // end of namespace pEp

#endif // MINI_ADAPTER_IMPL_HH
