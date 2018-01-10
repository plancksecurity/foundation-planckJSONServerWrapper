#include "ev_client.hh"
#include <evhttp.h>

namespace js = json_spirit;


PEP_STATUS ev_client::deliverRequest(const js::Object& request)
{

}


void ev_client::requestDoneTrampoline(evhttp_request* req, void* userdata)
{
	if(!userdata)
		return;
	
	ev_client* evc = static_cast<ev_client*>(userdata);
	evc->requestDone(req);
}
