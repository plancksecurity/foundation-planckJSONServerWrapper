#include "ev_client.hh"
#include "pep-utils.hh"
#include <evhttp.h>
#include "json_spirit/json_spirit_writer.h"


namespace js = json_spirit;
using pEp::utility::make_c_ptr;

PEP_STATUS ev_client::deliverRequest(event_base* base, const js::Object& request)
{

	auto conn = make_c_ptr( evhttp_connection_base_new( base, (evdns_base*)nullptr, server_name.c_str(), server_port ),
	                       &evhttp_connection_free
	                       );
	
	const std::string request_s = js::write(request, js::raw_utf8);
	evhttp_request* ereq = evhttp_request_new( &requestDoneTrampoline, this ); // ownership of ereq goes to the connection in evhttp_make_request() below.
	evhttp_add_header(ereq->output_headers, "Content-Length", std::to_string(request_s.length()).c_str());
	auto output_buffer = evhttp_request_get_output_buffer(ereq);
	evbuffer_add(output_buffer, request_s.data(), request_s.size());
	
	// ownership of ereq goes to the connection in evhttp_make_request() below.
	const int ret = evhttp_make_request(conn.get(), ereq, EVHTTP_REQ_POST, server_uri.c_str() );
	
	return (ret == 0) ? PEP_STATUS_OK : PEP_UNKNOWN_ERROR;
}


void ev_client::requestDoneTrampoline(evhttp_request* req, void* userdata)
{
	if(!userdata)
		return;
	
	ev_client* evc = static_cast<ev_client*>(userdata);
	evc->requestDone(req);
}
