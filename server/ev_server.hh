// helper functions needed for the libev_http server

#ifndef EV_SERVER_HH
#define EV_SERVER_HH

#include <string>
#include <boost/filesystem/path.hpp>
#include <pEp/webserver.hh>


class Logger;

class ev_server
{
public:
	static
	pEp::Webserver::response sendReplyString(const pEp::Webserver::request& req, const char* contentType, std::string&& outputText);
	
	static
	pEp::Webserver::response sendFile(const pEp::Webserver::request& req, const char* mimeType, const boost::filesystem::path& fileName);

	// catch-all callback. Used by demo html & JavaScript client to deliver static HTML & JS files
	static
	pEp::Webserver::response OnOtherRequest(boost::cmatch match, const pEp::Webserver::request& req);

	// generate a JavaScript file containing the definition of all registered callable functions, see above.
	static
	pEp::Webserver::response OnGetFunctions(boost::cmatch match, const pEp::Webserver::request& req);

	// handles calls to the JSON-RPC API
	static
	pEp::Webserver::response OnApiRequest(boost::cmatch match, const pEp::Webserver::request& req);

	// should be set before any of the methods above is called, due to static initializers use that value,
	// so changing it later might be useless.
	static boost::filesystem::path path_to_html;
	
	// add sharks to the JSON Adapter
	static
	void addSharks();

protected:
	static
	Logger& Log();
};

#endif
