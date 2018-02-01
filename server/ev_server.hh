// helper functions needed for the libev_http server

#ifndef EV_SERVER_HH
#define EV_SERVER_HH

#include <string>
#include <boost/filesystem/path.hpp>

struct evhttp_request;

class ev_server
{
public:
	static
	void sendReplyString(evhttp_request* req, const char* contentType, const std::string& outputText);
	
	static
	void sendFile( evhttp_request* req, const std::string& mimeType, const boost::filesystem::path& fileName);

	// catch-all callback. Used by demo html & JavaScript client to deliver static HTML & JS files
	static
	void OnOtherRequest(evhttp_request* req, void*);

	// generate a JavaScript file containing the definition of all registered callable functions, see above.
	static
	void OnGetFunctions(evhttp_request* req, void*);

	// handles calls to the JSON-RPC API
	static
	void OnApiRequest(evhttp_request* req, void* obj);

	// should be set before any of the methods above is called, due to static initializers use that value,
	// so changing it later might be useless.
	static boost::filesystem::path path_to_html;
};

#endif
