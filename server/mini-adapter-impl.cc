#include "mini-adapter-impl.hh"
#include "json-adapter.hh"
#include <thread>

#include <pEp/status_to_string.hh>  // from libpEpAdapter.
#include <pEp/Adapter.hh>
#include <pEp/callback_dispatcher.hh>

namespace pEp {
namespace mini {

void startSync()
{
    pEp::callback_dispatcher.start_sync();
}


void stopSync()
{
	pEp::callback_dispatcher.stop_sync();
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
