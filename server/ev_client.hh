#ifndef PEP_JSON_EV_CLIENT_HH
#define PEP_JSON_EV_CLIENT_HH

#include <pEp/pEpEngine.h> // for PEP_STATUS etc.
#include "json_spirit/json_spirit_value.h"
#include <string>


struct evhttp_request;
struct event_base;


class ev_client
{
public:
	ev_client(const std::string& _server_name, unsigned _server_port, const std::string& _server_path = "/")
	: server_name(_server_name)
	, server_port(_server_port)
	, server_path(_server_path)
	, server_uri( "http://" + server_name + ":" + std::to_string(server_port) + server_path )
	{}
	
	virtual ~ev_client() = default;
	
	virtual
	void requestDone(evhttp_request* req)
	{
		// do nothing by default.
	}
	
	PEP_STATUS deliverRequest(event_base* base, const json_spirit::Object& request);

protected:
	const std::string server_name;
	const unsigned server_port;
	const std::string server_path;
	const std::string server_uri;
	
	static
	void requestDoneTrampoline(evhttp_request* req, void* userdata);
};

#endif // PEP_JSON_EV_CLIENT_HH
