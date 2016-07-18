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
	"(13) Köln-Poll";        // refactoring to avoid copying of parameters. Fixes the memory corruption. Some other clean-ups


PEP_SESSION createSession()
{
	PEP_SESSION session = nullptr;
	auto ret = init(&session);
	if(ret != PEP_STATUS_OK)
	{
		throw std::runtime_error("createSession() failed because pEp's init() returns " + status_to_string(ret) );
	}
	return session;
}


typedef Registry<PEP_SESSION, PEP_SESSION(*)(), void(*)(PEP_SESSION)>  SessionRegistry;

template<> const uint64_t SessionRegistry::Identifier = 0x44B0310A;

SessionRegistry session_registry(&createSession, &release);


std::string registerSession()
{
	const std::uint64_t idx = session_registry.emplace();
	return base57(idx);
}


PEP_STATUS releaseSession(const js::Value& session_handle)
{
	try{
		const std::string s = session_handle.get_str();
		session_registry.erase(s);
		return PEP_STATUS_OK;
	}
	catch(...)
	{
		return PEP_ILLEGAL_VALUE; // There is no "PEP_INVALID_SESSION" or the like. :-(
	}
}


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
		FP( "encrypt_and_sign", new Func<PEP_STATUS, In<PEP_SESSION>, In<stringlist_t*>, In<const char*>, In<size_t>, Out<char*>, Out<size_t>> ( &encrypt_and_sign) ),
		
		FP( "version", new Func<std::string>( &getVersion ) ),
		FP( "registerSession", new Func<std::string>(&registerSession) ),
		FP( "releaseSession", new Func<PEP_STATUS, InRaw<PEP_SESSION>>(&releaseSession) ),
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


void OnApiRequest(evhttp_request* req, void*)
{
	evbuffer* inbuf = evhttp_request_get_input_buffer(req);
	const size_t length = evbuffer_get_length(inbuf);

	int request_id = -42;
	js::Object answer;
	js::Value p;
	
	try
	{
	
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
			answer = call( functions, request );
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


void OnCreateSession(evhttp_request* req, void*)
{
	std::cout << "Create session: " << std::flush;
	
	evbuffer* inbuf = evhttp_request_get_input_buffer(req);
	const size_t length = evbuffer_get_length(inbuf);
	
	std::vector<char> data(length);
	ssize_t nr = evbuffer_copyout(inbuf, data.data(), data.size());
	if(nr>0)
	{
		std::cout << "\tData: «" << std::string( data.data(), data.data() + nr )  << "»\n";
	}else{
		std::cout << "\tError: " << nr << ".\n";
	}
	const auto sn = registerSession();
	const std::string a = "{\"session\":\"" + sn + "\"}\n";
	std::cout << "\tResult: " << a << "\n";
	
	sendReplyString(req, "text/plain", a );
}


void OnCloseSession(evhttp_request* req, void*)
{
	std::cout << "Close session: " << std::flush;
	
	evbuffer* inbuf = evhttp_request_get_input_buffer(req);
	const size_t length = evbuffer_get_length(inbuf);
	
	js::Object answer;
	std::vector<char> data(length);
	ssize_t nr = evbuffer_copyout(inbuf, data.data(), data.size());
	const std::string data_string(data.data(), data.data() + nr );
	if(nr>0)
	{
		std::cout << "\tData: «" << data_string  << "»\n";
		js::Value p;
		bool b = js::read( data_string, p);
		if(p.type() == js::str_type)
		{
			session_registry.erase( p.get_str() );
			answer = make_result( std::string( "\"" + p.get_str() + "\" successfully erased."), 44 );
		}else{
			answer = make_error( JSON_RPC::PARSE_ERROR, "evbuffer_copyout does not return a JSON string. b=" + std::to_string(b), js::Value{data_string}, 42 );
		}
	}else{
		std::cout << "\tError: " << nr << ".\n";
		answer = make_error( JSON_RPC::PARSE_ERROR, "evbuffer_copyout returns negative value " + std::to_string(nr), js::Value{}, 42 );
	}
	
	sendReplyString(req, "text/plain", js::write(answer) );
}


void OnGetAllSessions(evhttp_request* req, void*)
{
	const auto sessions = session_registry.keys();
	std::string result;
	for(const auto k : sessions)
	{
		result += std::string(result.empty() ? "" : ",") + "{\"session\":\"" + base57(k) + "\"}\n";
	}
	
	result = "[" + result + "]\n";
	std::cout << "Get All Sessions: " << result;
	sendReplyString(req, "text/plain", result );
}

} // end of anonymous namespace


template<>
PEP_SESSION from_json(const js::Value& v)
{
	return session_registry.get(v.get_str());
}


const std::string& JsonAdapter::version()
{
	return server_version;
}



unsigned JsonAdapter::apiVersion()
{
	return API_VERSION;
}


bool volatile isRun = true;

auto ThreadDeleter = [](std::thread *t) { isRun = false; t->join(); delete t; };

typedef std::unique_ptr<std::thread, decltype(ThreadDeleter)> ThreadPtr;
typedef std::vector<ThreadPtr> ThreadPool;


int main()
try
{
	std::cout << "I have " << session_registry.size() << " registered session(s).\n";
	
	std::exception_ptr initExcept;
	evutil_socket_t s = -1;
	auto ThreadFunc = [&] ()
	{
		try
		{
			std::cerr << " +++ Thread starts: isRun=" << isRun << ", id=" << std::this_thread::get_id() << ". +++\n";
			
			std::unique_ptr<event_base, decltype(&event_base_free)> eventBase(event_base_new(), &event_base_free);
			if (!eventBase)
				throw std::runtime_error("Failed to create new base_event.");
			
			std::unique_ptr<evhttp, decltype(&evhttp_free)> evHttp(evhttp_new(eventBase.get()), &evhttp_free);
			
			if (!evHttp)
				throw std::runtime_error("Failed to create new evhttp.");
			
			evhttp_set_cb(evHttp.get(), ApiRequestUrl.c_str()    , OnApiRequest    , nullptr);
			evhttp_set_cb(evHttp.get(), CreateSessionUrl.c_str() , OnCreateSession , nullptr);
			evhttp_set_cb(evHttp.get(), GetAllSessionsUrl.c_str(), OnGetAllSessions, nullptr);
			evhttp_set_cb(evHttp.get(), "/pep_functions.js"      , OnGetFunctions  , nullptr);
			evhttp_set_gencb(evHttp.get(), OnOtherRequest, nullptr);
			
			if (s == -1)
			{
				// initialize the pEp engine
				registerSession();
				std::cout << "I have " << session_registry.size() << " registered session(s).\n";
				
				unsigned port_ofs = 0;
try_next_port:
				auto* boundSock = evhttp_bind_socket_with_handle(evHttp.get(), SrvAddress.c_str(), SrvPort + port_ofs);
				if (!boundSock)
				{
					++port_ofs;
					if(port_ofs > 9999)
					{
						throw std::runtime_error("Failed to bind server socket.");
					}
					goto try_next_port;
				}
				
				create_security_token(SrvAddress, SrvPort + port_ofs, BaseUrl);
				
				if ((s = evhttp_bound_socket_get_fd(boundSock)) == -1)
					throw std::runtime_error("Failed to get server socket for next instance.");
			}
			else
			{
				if (evhttp_accept_socket(evHttp.get(), s) == -1)
					throw std::runtime_error("Failed to accept() on server socket for new instance.");
			}
			
			while(isRun)
			{
				event_base_loop(eventBase.get(), EVLOOP_NONBLOCK);
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
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
		std::cerr << " +++ Thread exit? isRun=" << isRun << ", id=" << std::this_thread::get_id() << ". +++\n";
	};
	
	
	ThreadPool threads;
	for (int i=0; i<SrvThreadCount; ++i)
	{
		std::cout << "Start Thread #" << i << "...\n";
		ThreadPtr thread(new std::thread(ThreadFunc), ThreadDeleter);
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		if (initExcept != std::exception_ptr())
		{
			isRun = false;
			std::rethrow_exception(initExcept);
		}
		threads.push_back(std::move(thread));
	}
	
	int input = 0;
	do{
		std::cout << "Press <Q> <Enter> to quit." << std::endl;
		input = std::cin.get();
		std::cout << "Oh, I got a '" << input << "'. \n";
	}while(input != 'q' && input != 'Q');
	
	isRun = false;
}
catch (std::exception const &e)
{
	std::cerr << "Exception catched in main(): \"" << e.what() << "\"" << std::endl;
}
