// helper functions needed for the libev_http server

#ifndef EV_SERVER_HH
#define EV_SERVER_HH

#include <string>
#include <boost/filesystem/path.hpp>

struct evhttp_request;

// TODO: might be a class?
namespace ev_server
{
	void sendReplyString(evhttp_request* req, const char* contentType, const std::string& outputText);
	void sendFile( evhttp_request* req, const std::string& mimeType, const boost::filesystem::path& fileName);

	// catch-all callback. Used by demo html & JavaScript client to deliver static HTML & JS files
	void OnOtherRequest(evhttp_request* req, void*);

	// generate a JavaScript file containing the definition of all registered callable functions, see above.
	void OnGetFunctions(evhttp_request* req, void*);

	// handles calls to the JSON-RPC API
	void OnApiRequest(evhttp_request* req, void* obj);

}

#endif
