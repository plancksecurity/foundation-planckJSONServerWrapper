#include <stdexcept>
#include <iostream>
#include <memory>
#include <chrono>
#include <thread>
#include <cstdint>
#include <vector>
#include <evhttp.h>

#include <string>
#include <set>
#include <fcntl.h>
#include <functional>
#include <tuple>

#include "json-adapter.hh"
#include "function_map.hh"
#include "pep-types.hh"
#include "json_rpc.hh"
#include "security-token.hh"

#include <pEp/message_api.h>

#include <boost/filesystem.hpp>
#include "json_spirit/json_spirit_writer.h"
#include "json_spirit/json_spirit_reader.h"
#include "json_spirit/json_spirit_utils.h"


namespace {

static const unsigned API_VERSION = 0x0002;

std::string BaseUrl    = "/ja/0.1/";
int SrvThreadCount     = 1;

const std::string CreateSessionUrl = BaseUrl + "createSession";
const std::string GetAllSessionsUrl = BaseUrl + "getAllSessions";
const std::string ApiRequestUrl = BaseUrl + "callFunction";


// version names comes from here:
// https://de.wikipedia.org/wiki/Bundesautobahn_4
const std::string server_version =
//	"(4) Kreuz Aachen"; // first version with this version scheme :-)
//	"(5a) Eschweiler-West"; // add support for log_event() and trustwords()
//	"(5b) Eschweiler-Ost";  // add support for get_identity() and get_languagelist()
//	"(5c) Weisweiler";      // add missing members of struct message
//	"(5d) Langerwehe";      // add the remaining functions from pEpEngine.h
//	"(6) Düren";            // some bug fixes for missing data types, UTF-8 output etc., status in hex etc.
//	"(7a) Merzenich";       // InOut parameters added. Untested, yet.
//	"(7b) Elsdorf";         // add own_key functions, which are new in the pEp Engine
//	"(8) Kerpen";           // pEp_identity fixes due to changes in the pEp Engine 
//	"(8a) Kreuz Kerpen";    // remove own_key_add() because pEpEngine doesn't have it anymore.
//	"(9a) Frechen-Königsdorf"; // add security-token
//	"(10) Kreuz Köln-West"; // More fields in JavaScript for "message", 1-element identity list to support message->to attribute
//	"(11) Köln-Klettenberg"; // support for identity_list as output parameter, as needed by import_key() now. Fix some issue with identity.lang
//	"(12) Kreuz Köln Süd";   // support for attachments, so encrypt_message() works now! :-) but we have memory corruption, hence the FIXME in pep-types.cc :-(
//	"(13) Köln-Poll";        // refactoring to avoid copying of parameters. Fixes the memory corruption. Some other clean-ups
//	"(!4) Köln-Gremberg";    // refactoring to use JSON-Adapter as a library
	"(15) Dreieck Heumar";   // PEP_SESSIONs are now handled internally, so the adapter's users don't have to care about anymore. :-)


typedef std::map<std::thread::id, PEP_SESSION> SessionRegistry;

SessionRegistry session_registry;


PEP_STATUS get_gpg_path(const char** path)
{
	const char* gpg_path = nullptr;
	const auto status =get_binary_path( PEP_crypt_OpenPGP, &gpg_path);
	
	if(status == PEP_STATUS_OK && gpg_path!=nullptr)
	{
		*path = strdup(gpg_path);
	}
	return status;
}


// these are the pEp functions that are callable by the client
const FunctionMap functions = {
		
		// from message_api.h
		FP( "—— Message API ——", new Separator ),
		FP( "encrypt_message", new Func<PEP_STATUS, In<PEP_SESSION>, In<message*>, In<stringlist_t*>, Out<message*>, In<PEP_enc_format>>( &encrypt_message ) ),
		FP( "decrypt_message", new Func<PEP_STATUS, In<PEP_SESSION>, In<message*>, Out<message*>, Out<stringlist_t*>, Out<PEP_color>, Out<PEP_decrypt_flags_t>>(  &decrypt_message ) ),
		FP( "outgoing_message_color", new Func<PEP_STATUS, In<PEP_SESSION>, In<message*>, Out<PEP_color>>( &outgoing_message_color ) ),
		FP( "identity_color" , new Func<PEP_STATUS, In<PEP_SESSION>, In<pEp_identity*>, Out<PEP_color>>( &identity_color) ),
		FP( "get_gpg_path",    new Func<PEP_STATUS, Out<const char*>>(&get_gpg_path) ),
		
		FP( "—— pEp Engine Core API ——", new Separator),
		FP( "log_event",  new Func<PEP_STATUS, In<PEP_SESSION>, In<const char*>, In<const char*>, In<const char*>, In<const char*>>( &log_event) ),
		FP( "trustwords", new Func<PEP_STATUS, In<PEP_SESSION>, In<const char*>, In<const char*>, Out<char*>, Out<size_t>, In<int>>( &trustwords) ),
		FP( "get_languagelist", new Func<PEP_STATUS, In<PEP_SESSION>, Out<char*>>( &get_languagelist) ),
		FP( "get_phrase"      , new Func<PEP_STATUS, In<PEP_SESSION>, In<const char*>, In<int>, Out<char*>> ( &get_phrase) ),
		
		FP( "—— Identity Management API ——", new Separator),
		FP( "get_identity"       , new Func<PEP_STATUS, In<PEP_SESSION>, In<const char*>, In<const char*>, Out<pEp_identity*>>( &get_identity) ),
		FP( "set_identity"       , new Func<PEP_STATUS, In<PEP_SESSION>, In<pEp_identity*>> ( &set_identity) ),
		FP( "mark_as_comprimized", new Func<PEP_STATUS, In<PEP_SESSION>, In<const char*>> ( &mark_as_compromized) ),
		
		FP( "—— Low level Key Management API ——", new Separator),
		FP( "generate_keypair", new Func<PEP_STATUS, In<PEP_SESSION>, InOut<pEp_identity*>> ( &generate_keypair) ),
		FP( "delete_keypair", new Func<PEP_STATUS, In<PEP_SESSION>, In<const char*>> ( &delete_keypair) ),
		FP( "import_key"    , new Func<PEP_STATUS, In<PEP_SESSION>, In<const char*>, In<std::size_t>, Out<identity_list*>> ( &import_key) ),
		FP( "export_key"    , new Func<PEP_STATUS, In<PEP_SESSION>, In<const char*>, Out<char*>, Out<std::size_t>> ( &export_key) ),
		FP( "find_keys"     , new Func<PEP_STATUS, In<PEP_SESSION>, In<const char*>, Out<stringlist_t*>> ( &find_keys) ),
		FP( "get_trust"     , new Func<PEP_STATUS, In<PEP_SESSION>, InOut<pEp_identity*>> ( &get_trust) ),
		FP( "own_key_is_listed", new Func<PEP_STATUS, In<PEP_SESSION>, In<const char*>, Out<bool>> ( &own_key_is_listed) ),
		FP( "own_key_retrieve" , new Func<PEP_STATUS, In<PEP_SESSION>, Out<stringlist_t*>> ( &own_key_retrieve) ),
		
		FP( "least_trust"   , new Func<PEP_STATUS, In<PEP_SESSION>, In<const char*>, Out<PEP_comm_type>> ( &least_trust) ),
		FP( "get_key_rating", new Func<PEP_STATUS, In<PEP_SESSION>, In<const char*>, Out<PEP_comm_type>> ( &get_key_rating) ),
		FP( "renew_key"     , new Func<PEP_STATUS, In<PEP_SESSION>, In<const char*>, In<const timestamp*>> ( &renew_key) ),
		FP( "revoke"        , new Func<PEP_STATUS, In<PEP_SESSION>, In<const char*>, In<const char*>> ( &revoke_key) ),
		FP( "key_expired"   , new Func<PEP_STATUS, In<PEP_SESSION>, In<const char*>, In<time_t>, Out<bool>> ( &key_expired) ),
		
		// my own example function that does something useful. :-)
		FP( "—— Other ——", new Separator ),
		FP( "version",     new Func<std::string>( &JsonAdapter::version ) ),
		FP( "apiVversion", new Func<unsigned>   ( &JsonAdapter::apiVersion ) ),
	};


void sendReplyString(evhttp_request* req, const char* contentType, const std::string& outputText)
{
	auto* outBuf = evhttp_request_get_output_buffer(req);
	if (!outBuf)
		return;
	
	if(contentType)
	{
		evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", contentType);
	}
	const int ret = evbuffer_add(outBuf, outputText.data(), outputText.size());
	if(ret==0)
	{
		evhttp_send_reply(req, HTTP_OK, "", outBuf);
	}else{
		evhttp_send_reply(req, 500, "evbuffer_add() failed.", outBuf);
	}
	
	std::cerr << "\n=== sendReplyString(): ret=" << ret << ", contentType=" << (contentType ? "«" + std::string(contentType)+ "»" : "NULL") 
		<< ", output=«" << outputText << "»." << std::endl;
}


void sendFile( evhttp_request* req, const std::string& mimeType, const std::string& fileName)
{
	auto* outBuf = evhttp_request_get_output_buffer(req);
	if (!outBuf)
		return;
	
	const int fd = open(fileName.c_str(), O_RDONLY);
	if(fd>0)
	{
		const uint64_t fileSize = boost::filesystem::file_size(fileName);
		evbuffer_add_file(outBuf, fd, 0, fileSize);
		evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", mimeType.c_str());
		evhttp_send_reply(req, HTTP_OK, "", outBuf);
		//close(fd);  // not necessary because it is done inside of libevhttp :-)
	}else{
		char error_msg[128];
		snprintf(error_msg, 127, "Cannot open file \"%s\". errno=%d.\n", fileName.c_str(), errno);
		evhttp_send_error(req, HTTP_NOTFOUND, error_msg);
	}
}


struct FileRequest
{
	std::string mimeType;
	std::string fileName;
};

// catch-all callback
void OnOtherRequest(evhttp_request* req, void*)
{
	static const std::map<std::string, FileRequest > files =
		{
			{ "/"                , {"text/html"      , "../html/index.html"       } },
			{ "/jquery.js"       , {"text/javascript", "../html/jquery-2.2.0.min.js"  } },
			{ "/interactive.js"  , {"text/javascript", "../html/interactive.js"   } },
			{ "/favicon.ico"     , {"image/vnd.microsoft.icon", "../html/json-test.ico"} },
		};
	
	const evhttp_uri* uri = evhttp_request_get_evhttp_uri(req);
	const char* path = evhttp_uri_get_path(uri);
	const char* uri_string = evhttp_request_get_uri(req);
	std::cerr << "** Request: [" << uri_string << "] " << (path? " Path: [" + std::string(path) + "]" : "null path") << "\n";
	
	if(path)
	{
		const auto q = files.find(path);
		if(q != files.end()) // found in "files" map
		{
			std::cerr << "\t found file \"" << q->second.fileName << "\", type=" << q->second.mimeType << ".\n";
			sendFile( req, q->second.mimeType, q->second.fileName);
			return;
		}
	}

	const std::string reply = std::string("=== Catch-All-Reply ===\nRequest URI: [") + uri_string + "]\n===\n"
		+ (path ? "NULL Path" : "Path: [ " + std::string(path) + "]" ) + "\n";
	std::cerr << "\t ERROR: " << reply ;
	sendReplyString(req, "text/plain", reply.c_str());
};


// generate a JavaScript file containing the definition of all registered callable functions, see above.
void OnGetFunctions(evhttp_request* req, void*)
{
	static const std::string preamble =
		"var Direction = { In:1, Out:2, InOut:3 };\n"
		"var Type = {\n"
		"		Blob    : 10, // binary strings or 'array of octet'\n"
		"		String  : 20, // human-readable, NUL-terminated utf8-encoded strings\n"
		"		StringList: 25,\n"
		"		Message : 30,\n"
		"		PEP_color : 50,\n"
		"		PEP_enc_format: 51,\n"
		"		PEP_STATUS: 59,\n"
		"		Uint16  : 80,\n"
		"		Session : 90 // opaque type. only a special encoded 'handle' is used in JavaScript code\n"
		"	};\n"
		"\n"
		"var server_version = \"" + server_version + "\";\n"
		"var pep_functions = ";
	
	js::Array jsonfunctions;
	for(const auto& f : functions)
	{
		js::Object o;
		o.emplace_back( "name", f.first );
		f.second->setJavaScriptSignature( o );
		jsonfunctions.push_back( o );
	}
	
	const std::string output = preamble + js::write( jsonfunctions, js::pretty_print | js::raw_utf8 | js::single_line_arrays )
		+ ";\n"
		"\n"
		"// End of generated file.\n";
		
	sendReplyString(req, "text/javascript", output.c_str());
}


void OnApiRequest(evhttp_request* req, void* obj)
{
	evbuffer* inbuf = evhttp_request_get_input_buffer(req);
	const size_t length = evbuffer_get_length(inbuf);

	int request_id = -42;
	js::Object answer;
	js::Value p;
	
	try
	{
	
	const JsonAdapter* ja = static_cast<const JsonAdapter*>(obj);
	
	std::vector<char> data(length);
	ssize_t nr = evbuffer_copyout(inbuf, data.data(), data.size());
	const std::string data_string(data.data(), data.data() + nr );
	if(nr>0)
	{
		std::cout << "\tData: «" << data_string  << "»\n";
		bool b = js::read( data_string, p);
		if(p.type() == js::obj_type)
		{
			const js::Object& request = p.get_obj();
			answer = call( functions, request, ja->sec_token() );
		}else{
			answer = make_error( JSON_RPC::PARSE_ERROR, "evbuffer_copyout does not return a JSON string. b=" + std::to_string(b), js::Value{data_string}, 42 );
		}
	}else{
		std::cout << "\tError: " << nr << ".\n";
		answer = make_error( JSON_RPC::INTERNAL_ERROR, "evbuffer_copyout returns negative value", p, request_id );
	}
	
	}
	catch(const std::exception& e)
	{
		std::cout << "\tException: \"" << e.what() << "\"\n";
		answer = make_error( JSON_RPC::INTERNAL_ERROR, "Got a std::exception: \"" + std::string(e.what()) + "\"", p, request_id );
	}

	sendReplyString(req, "text/plain", js::write(answer, js::raw_utf8));
};


} // end of anonymous namespace


std::string getSessions()
{
	js::Array a;
	a.reserve(session_registry.size());
	for(const auto& s : session_registry)
	{
		std::stringstream ss;
		js::Object o;
		ss << s.first;
		o.emplace_back("tid", ss.str() );
		ss.str("");
		ss << static_cast<void*>(s.second);
		o.emplace_back("session", ss.str() );
		if(s.first == std::this_thread::get_id())
		{
			o.emplace_back("mine", true);
		}
		a.push_back( std::move(o) );
	}
	
	return js::write( a, js::pretty_print | js::raw_utf8 | js::single_line_arrays );
}


template<>
PEP_SESSION from_json(const js::Value& v)
{
	const auto id = std::this_thread::get_id();
	const auto q = session_registry.find( id );
	if(q == session_registry.end())
	{
		std::stringstream ss;
		ss << "There is no SESSION for this thread (" << id << ")!"; 
		throw std::logic_error( ss.str() );
	}
	return q->second;
}


std::string JsonAdapter::version()
{
	return server_version;
}


unsigned JsonAdapter::apiVersion()
{
	return API_VERSION;
}


auto ThreadDeleter = [](std::thread *t)
{
	const auto id = t->get_id();
	const auto q = session_registry.find( id );
	if(q != session_registry.end())
	{
		release(q->second);
		session_registry.erase( q );
	}
	
	t->join();
	delete t;
};

typedef std::unique_ptr<std::thread, decltype(ThreadDeleter)> ThreadPtr;
typedef std::vector<ThreadPtr> ThreadPool;


struct JsonAdapter::Internal
{
	std::unique_ptr<event_base, decltype(&event_base_free)> eventBase = {nullptr, &event_base_free};
	std::unique_ptr<evhttp, decltype(&evhttp_free)> evHttp = {nullptr, &evhttp_free};
	std::string address;
	std::string token;
	
	unsigned    start_port    = 0;
	unsigned    end_port      = 0;
	unsigned    port          = 0;
	unsigned    request_count = 0;
	evutil_socket_t sock      = -1;
	bool        running = false;
	ThreadPool  threads;
};


JsonAdapter::JsonAdapter(const std::string& address, unsigned start_port, unsigned end_port)
: i(new Internal)
{
	i->eventBase.reset(event_base_new());
	if (!i->eventBase)
		throw std::runtime_error("Failed to create new base_event.");
	
	i->evHttp.reset( evhttp_new(i->eventBase.get()));
	if (!i->evHttp)
		throw std::runtime_error("Failed to create new evhttp.");
	
	i->address    = address;
	i->start_port = start_port;
	i->end_port   = end_port;
}


JsonAdapter::~JsonAdapter()
{
	delete i;
}


void JsonAdapter::run()
try
{
	std::cout << "I have " << session_registry.size() << " registered session(s).\n";
	
	std::exception_ptr initExcept;
	auto ThreadFunc = [&] ()
	{
		try
		{
			const auto id = std::this_thread::get_id();
			std::cerr << " +++ Thread starts: isRun=" << i->running << ", id=" << id << ". +++\n";
			const auto q=session_registry.find(id);
			if(q==session_registry.end())
			{
				PEP_SESSION session = nullptr;
				const PEP_STATUS status = init(&session); // release(status) in ThreadDeleter
				if(status != PEP_STATUS_OK || session==nullptr)
				{
					throw std::runtime_error("Cannot create session! status: " + status_to_string(status));
				}
				
				session_registry.emplace( id, session);
				std::cerr << "\tcreated new session for this thread: " << static_cast<void*>(session) << ".\n";
			}else{
				std::cerr << "\tsession for this thread: "  << static_cast<void*>(q->second) << ".\n";
			}
			
			evhttp_set_cb(i->evHttp.get(), ApiRequestUrl.c_str()    , OnApiRequest    , this);
			evhttp_set_cb(i->evHttp.get(), "/pep_functions.js"      , OnGetFunctions  , this);
			evhttp_set_gencb(i->evHttp.get(), OnOtherRequest, nullptr);
			
			if (i->sock == -1) // no port bound, yet
			{
				// initialize the pEp engine
				std::cout << "I have " << session_registry.size() << " registered session(s).\n";
				
				unsigned port_ofs = 0;
try_next_port:
				auto* boundSock = evhttp_bind_socket_with_handle(i->evHttp.get(), i->address.c_str(), i->start_port + port_ofs);
				if (!boundSock)
				{
					++port_ofs;
					if(i->start_port + port_ofs > i->end_port)
					{
						throw std::runtime_error("Failed to bind server socket: "
							"No free port between " + std::to_string(i->start_port) + " and " + std::to_string(i->end_port)
							);
					}
					goto try_next_port;
				}
				
				if ((i->sock = evhttp_bound_socket_get_fd(boundSock)) == -1)
					throw std::runtime_error("Failed to get server socket for next instance.");
				
				i->port = i->start_port + port_ofs;
				i->token = create_security_token(i->address, i->port, BaseUrl);
				
				std::cout << "Bound to port " << i->port << ", sec_token=\"" << i->token << "\"\n";
			}
			else
			{
				if (evhttp_accept_socket(i->evHttp.get(), i->sock) == -1)
					throw std::runtime_error("Failed to accept() on server socket for new instance.");
			}
			
			unsigned numnum = 1000000;
			while(i->running)
			{
				event_base_loop(i->eventBase.get(), EVLOOP_NONBLOCK);
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				std::cerr << "\r" << ++numnum << ".   ";
			}
		}
		catch (const std::exception& e)
		{
			std::cerr << " +++ std::exception in ThreadFunc: " << e.what() << "\n";
			initExcept = std::current_exception();
		}
		catch (...)
		{
			std::cerr << " +++ UNKNOWN EXCEPTION in ThreadFunc +++ ";
			initExcept = std::current_exception();
		}
		std::cerr << " +++ Thread exit? isRun=" << i->running << ", id=" << std::this_thread::get_id() << ". +++\n";
	};
	
	i->running = true;
	for(int t=0; t<SrvThreadCount; ++t)
	{
		std::cout << "Start Thread #" << t << "...\n";
		ThreadPtr thread(new std::thread(ThreadFunc), ThreadDeleter);
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		if (initExcept != std::exception_ptr())
		{
			i->running = false;
			std::rethrow_exception(initExcept);
		}
		i->threads.push_back(std::move(thread));
	}
	std::cout << "All " << SrvThreadCount << " thread(s) started.\n";
}
catch (std::exception const &e)
{
	std::cerr << "Exception catched in main(): \"" << e.what() << "\"" << std::endl;
}


void JsonAdapter::shutdown(timeval* t)
{
	i->running = false;
	const int ret = event_base_loopexit(i->eventBase.get(), t);
	if(ret!=0)
	{
		throw std::runtime_error("JsonAdapter::shutdown() failed.");
	}
}


const std::string& JsonAdapter::sec_token() const
{
	return i->token;
}


// returns 'true' if 's' is the security token created by the function above.
bool JsonAdapter::verify_security_token(const std::string& s) const
{
	if(s!=i->token)
	{
		std::cerr << "sec_token=\"" << i->token << "\" is unequal to \"" << s << "\"!\n";
	}
	return s == i->token;
}
