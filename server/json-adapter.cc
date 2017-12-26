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

#include <mutex>

#include "main.hh"
#include "json-adapter.hh"
#include "function_map.hh"
#include "pep-types.hh"
#include "json_rpc.hh"
#include "nulllogger.hh"
#include "security-token.hh"
#include "pep-utils.hh"
#include "gpg_environment.hh"

#include <pEp/message_api.h>
#include <pEp/blacklist.h>
#include <pEp/openpgp_compat.h>
#include <pEp/mime.h>

#include <boost/filesystem.hpp>
#include "json_spirit/json_spirit_writer.h"
#include "json_spirit/json_spirit_reader.h"
#include "json_spirit/json_spirit_utils.h"


template<>
In<Context*, false>::~In()
{
	// do nothing
}

template<>
In<Context*, false>::In(const js::Value&, Context* ctx)
: value( ctx )
{

}


namespace fs = boost::filesystem;


namespace {
    using namespace pEp::utility;

static const unsigned API_VERSION = 0x0003;

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
//	"(15) Dreieck Heumar";   // PEP_SESSIONs are now handled internally, so the adapter's users don't have to care about anymore. :-)
//	"(16) Kreuz Köln Ost";   // mime_encode_message(), mime_decode_message(), blob_t are base64-encoded.
//	"(17) Köln Mehrheim";    // MIME_encrypt_message() and MIME_decrypt_message() instead, because the other two were internal functions
//	"(18) Refrath";          // add trust_personal_key(), key_mistrusted(), key_reset_trust()
//	"(19) Bensberg";         // command-line parameters, daemonize(), silent all output in daemon mode
//	"(20) Moitzfeld";        // showHandshake() -> notifyHandshake() and other Engine's API changes
//	"(21) Untereschbach";    // JSON-11: replace trustwords() by get_trustwords()
//	"(22) Overath";          // add blacklist_retrieve(), rename identity_color() and outgoing_message_color() into ..._rating().
//	"(23) Engelskirchen";    // fix JSON-19. Support "Bool" and "Language" as separate data types in JavaScript.
//	"(24) Bielstein";        // add MIME_encrypt_message_ex() and MIME_decrypt_message_ex() as a HACK.
//	"(25) Gummersbach";      // JSON-22: add MIME_encrypt_message_for_self() and change API for encrypt_message_for_self().
//	"(26) Reichshof";        // change return type from JSON array into JSON object {"output":[...], "return":..., "errorstack":[...]}
//	"(27) Eckenhagen";       // add command line switch  "--sync false"  to disable automatic start of keysync at startup
//	"(28) Olpe-Süd";         // add re_evaluate_message_rating(). Jira: JSON-29
//	"(29) Wenden";           // {start|stop}{KeySync|KeyserverLookup}  JSON-28
//	"(30) Krombach";         // JSON-49, add call_with_lock() around init() and release().
	"(31) Altenkleusheim";   // JSON-57: change location of server token file. breaking API change, so API_VERSION=0x0003.


typedef std::map<std::thread::id, JsonAdapter*> SessionRegistry;

SessionRegistry session_registry;

auto ThreadDeleter = [](std::thread* t)
{
	const auto id = t->get_id();
	const auto q = session_registry.find( id );
	if(q != session_registry.end())
	{
		delete q->second;
	}
	
	delete t;
};


typedef std::unique_ptr<std::thread, decltype(ThreadDeleter)> ThreadPtr;
typedef std::vector<ThreadPtr> ThreadPool;

// keyserver lookup
locked_queue< pEp_identity*, &free_identity> keyserver_lookup_queue;
PEP_SESSION keyserver_lookup_session = nullptr; // FIXME: what if another adapter started it already?
ThreadPtr   keyserver_lookup_thread{nullptr, ThreadDeleter};


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


PEP_STATUS registerEventListener(Context* ctx, std::string address, unsigned port, std::string securityContext)
{
	JsonAdapter* ja = dynamic_cast<JsonAdapter*>(ctx);
	if(!ja)
	{
		return PEP_STATUS(-42);
	}
	
	ja->registerEventListener(address, port, securityContext);
	return PEP_STATUS_OK;
}


PEP_STATUS unregisterEventListener(Context* ctx, std::string address, unsigned port, std::string securityContext)
{
	JsonAdapter* ja = dynamic_cast<JsonAdapter*>(ctx);
	if(!ja)
	{
		return PEP_STATUS(-42);
	}
	
	ja->unregisterEventListener(address, port, securityContext);
	return PEP_STATUS_OK;
}


// HACK: because "auto sessions" are per TCP connections, add a parameter to set passive_mode each time again
PEP_STATUS MIME_encrypt_message_ex(
    PEP_SESSION session,
    const char *mimetext,
    size_t size,
    stringlist_t* extra,
    bool passive_mode,   // <-- guess what
    char** mime_ciphertext,
    PEP_enc_format enc_format,
    PEP_encrypt_flags_t flags
)
{
	config_passive_mode(session, passive_mode);
	return MIME_encrypt_message(session, mimetext, size, extra, mime_ciphertext, enc_format, flags);
}

PEP_STATUS MIME_decrypt_message_ex(
    PEP_SESSION session,
    const char *mimetext,
    size_t size,
    bool passive_mode, // <-- guess what
    char** mime_plaintext,
    stringlist_t **keylist,
    PEP_rating *rating,
    PEP_decrypt_flags_t *flags
)
{
	config_passive_mode(session, passive_mode);
	return MIME_decrypt_message(session, mimetext, size, mime_plaintext, keylist, rating, flags);
}


// these are the pEp functions that are callable by the client
const FunctionMap functions = {
		// from message_api.h
		FP( "Message API", new Separator ),
		FP( "MIME_encrypt_message", new Func<PEP_STATUS, In<PEP_SESSION, false>, In<c_string>, In<size_t>, In<stringlist_t*>,
			Out<char*>, In<PEP_enc_format>, In<PEP_encrypt_flags_t>>( &MIME_encrypt_message ) ),
		FP( "MIME_encrypt_message_for_self", new Func<PEP_STATUS, In<PEP_SESSION, false>, In<pEp_identity*>, In<c_string>, In<size_t>,
			Out<char*>, In<PEP_enc_format>, In<PEP_encrypt_flags_t>>( &MIME_encrypt_message_for_self ) ),
		FP( "MIME_decrypt_message", new Func<PEP_STATUS, In<PEP_SESSION, false>, In<c_string>, In<size_t>,
			Out<char*>, Out<stringlist_t*>, Out<PEP_rating>, Out<PEP_decrypt_flags_t>>( &MIME_decrypt_message ) ),
		
		// HACK: because "auto sessions" are per TCP connections, add a parameter to set passive_mode each time again
		FP( "MIME_encrypt_message_ex", new Func<PEP_STATUS, In<PEP_SESSION, false>, In<c_string>, In<size_t>, In<stringlist_t*>, In<bool>,
			Out<char*>, In<PEP_enc_format>, In<PEP_encrypt_flags_t>>( &MIME_encrypt_message_ex ) ),
		FP( "MIME_decrypt_message_ex", new Func<PEP_STATUS, In<PEP_SESSION, false>, In<c_string>, In<size_t>, In<bool>,
			Out<char*>, Out<stringlist_t*>, Out<PEP_rating>, Out<PEP_decrypt_flags_t>>( &MIME_decrypt_message_ex ) ),
		
		FP( "startKeySync", new Func<void, In<JsonAdapter*, false>>( &JsonAdapter::startSync) ),
		FP( "stopKeySync",  new Func<void, In<JsonAdapter*, false>>( &JsonAdapter::stopSync ) ),
		FP( "startKeyserverLookup", new Func<void>( &JsonAdapter::startKeyserverLookup) ),
		FP( "stopKeyserverLookup",  new Func<void>( &JsonAdapter::stopKeyserverLookup ) ),
		
		FP( "encrypt_message", new Func<PEP_STATUS, In<PEP_SESSION, false>, In<message*>, In<stringlist_t*>, Out<message*>, In<PEP_enc_format>, In<PEP_encrypt_flags_t>>( &encrypt_message ) ),
		FP( "encrypt_message_for_self", new Func<PEP_STATUS, In<PEP_SESSION, false>, In<pEp_identity*>, In<message*>, Out<message*>, In<PEP_enc_format>, In<PEP_encrypt_flags_t>>( &encrypt_message_for_self ) ),
		FP( "decrypt_message", new Func<PEP_STATUS, In<PEP_SESSION, false>, In<message*>, Out<message*>, Out<stringlist_t*>, Out<PEP_rating>, Out<PEP_decrypt_flags_t>>(  &decrypt_message ) ),
		FP( "outgoing_message_rating", new Func<PEP_STATUS, In<PEP_SESSION,false>, In<message*>, Out<PEP_rating>>( &outgoing_message_rating ) ),
		FP( "re_evaluate_message_rating", new Func<PEP_STATUS, In<PEP_SESSION,false>, In<message*>, In<stringlist_t*>, In<PEP_rating>, Out<PEP_rating>>( &re_evaluate_message_rating ) ),
		FP( "identity_rating" , new Func<PEP_STATUS, In<PEP_SESSION,false>, In<pEp_identity*>, Out<PEP_rating>>( &identity_rating) ),
		FP( "get_gpg_path",    new Func<PEP_STATUS, Out<const char*>>(&get_gpg_path) ),
		
		FP( "pEp Engine Core API", new Separator),
		FP( "log_event",  new Func<PEP_STATUS, In<PEP_SESSION,false>, In<c_string>, In<c_string>, In<c_string>, In<c_string>>( &log_event) ),
		FP( "get_trustwords", new Func<PEP_STATUS, In<PEP_SESSION,false>, In<const pEp_identity*>, In<const pEp_identity*>, In<Language>, Out<char*>, Out<size_t>, In<bool>>( &get_trustwords) ),
		FP( "get_languagelist", new Func<PEP_STATUS, In<PEP_SESSION,false>, Out<char*>>( &get_languagelist) ),
		FP( "get_phrase"      , new Func<PEP_STATUS, In<PEP_SESSION,false>, In<Language>, In<int>, Out<char*>> ( &get_phrase) ),
		FP( "get_engine_version", new Func<const char*> ( &get_engine_version) ),
		FP( "config_passive_mode", new Func<void, In<PEP_SESSION,false>, In<bool>>( &config_passive_mode) ),
		FP( "config_unencrypted_subject", new Func<void, In<PEP_SESSION,false>, In<bool>>( &config_unencrypted_subject) ),
		
		FP( "Identity Management API", new Separator),
		FP( "get_identity"       , new Func<PEP_STATUS, In<PEP_SESSION,false>, In<c_string>, In<c_string>, Out<pEp_identity*>>( &get_identity) ),
		FP( "set_identity"       , new Func<PEP_STATUS, In<PEP_SESSION,false>, In<pEp_identity*>> ( &set_identity) ),
		FP( "mark_as_comprimized", new Func<PEP_STATUS, In<PEP_SESSION,false>, In<c_string>> ( &mark_as_compromized) ),
		FP( "identity_rating"    , new Func<PEP_STATUS, In<PEP_SESSION,false>, In<pEp_identity*>, Out<PEP_rating>>( &identity_rating) ),
		FP( "outgoing_message_rating", new Func<PEP_STATUS, In<PEP_SESSION,false>, In<message*>, Out<PEP_rating>>( &outgoing_message_rating) ),
		FP( "set_identity_flags"     , new Func<PEP_STATUS, In<PEP_SESSION,false>, InOut<pEp_identity*>, In<identity_flags_t>>( &set_identity_flags) ),
		FP( "unset_identity_flags"   , new Func<PEP_STATUS, In<PEP_SESSION,false>, InOut<pEp_identity*>, In<identity_flags_t>>( &unset_identity_flags) ),
		
		FP( "Low level Key Management API", new Separator),
		FP( "generate_keypair", new Func<PEP_STATUS, In<PEP_SESSION,false>, InOut<pEp_identity*>> ( &generate_keypair) ),
		FP( "delete_keypair", new Func<PEP_STATUS, In<PEP_SESSION,false>, In<c_string>> ( &delete_keypair) ),
		FP( "import_key"    , new Func<PEP_STATUS, In<PEP_SESSION,false>, In<c_string>, In<std::size_t>, Out<identity_list*>> ( &import_key) ),
		FP( "export_key"    , new Func<PEP_STATUS, In<PEP_SESSION,false>, In<c_string>, Out<char*>, Out<std::size_t>> ( &export_key) ),
		FP( "find_keys"     , new Func<PEP_STATUS, In<PEP_SESSION,false>, In<c_string>, Out<stringlist_t*>> ( &find_keys) ),
		FP( "get_trust"     , new Func<PEP_STATUS, In<PEP_SESSION,false>, InOut<pEp_identity*>> ( &get_trust) ),
		FP( "own_key_is_listed", new Func<PEP_STATUS, In<PEP_SESSION,false>, In<c_string>, Out<bool>> ( &own_key_is_listed) ),
		FP( "own_identities_retrieve", new Func<PEP_STATUS, In<PEP_SESSION,false>, Out<identity_list*>>( &own_identities_retrieve ) ),
		FP( "undo_last_mitrust", new Func<PEP_STATUS, In<PEP_SESSION,false>>( &undo_last_mistrust ) ),
		
		FP( "myself"        , new Func<PEP_STATUS, In<PEP_SESSION,false>, InOut<pEp_identity*>> ( &myself) ),
		FP( "update_dentity", new Func<PEP_STATUS, In<PEP_SESSION,false>, InOut<pEp_identity*>> ( &update_identity) ),
		
		FP( "trust_personal_key", new Func<PEP_STATUS, In<PEP_SESSION,false>, In<pEp_identity*>>( &trust_personal_key) ),
		FP( "key_mistrusted",     new Func<PEP_STATUS, In<PEP_SESSION,false>, In<pEp_identity*>>( &key_mistrusted) ),
		FP( "key_reset_trust",    new Func<PEP_STATUS, In<PEP_SESSION,false>, In<pEp_identity*>>( &key_reset_trust) ),
		
		FP( "least_trust"   , new Func<PEP_STATUS, In<PEP_SESSION,false>, In<c_string>, Out<PEP_comm_type>> ( &least_trust) ),
		FP( "get_key_rating", new Func<PEP_STATUS, In<PEP_SESSION,false>, In<c_string>, Out<PEP_comm_type>> ( &get_key_rating) ),
		FP( "renew_key"     , new Func<PEP_STATUS, In<PEP_SESSION,false>, In<c_string>, In<const timestamp*>> ( &renew_key) ),
		FP( "revoke"        , new Func<PEP_STATUS, In<PEP_SESSION,false>, In<c_string>, In<c_string>> ( &revoke_key) ),
		FP( "key_expired"   , new Func<PEP_STATUS, In<PEP_SESSION,false>, In<c_string>, In<time_t>, Out<bool>> ( &key_expired) ),
		
		FP( "from blacklist.h & OpenPGP_compat.h", new Separator),
		FP( "blacklist_add"   , new Func<PEP_STATUS, In<PEP_SESSION,false>, In<c_string>> ( &blacklist_add) ),
		FP( "blacklist_delete", new Func<PEP_STATUS, In<PEP_SESSION,false>, In<c_string>> ( &blacklist_delete) ),
		FP( "blacklist_is_listed", new Func<PEP_STATUS, In<PEP_SESSION,false>, In<c_string>, Out<bool>> ( &blacklist_is_listed) ),
		FP( "blacklist_retrieve" , new Func<PEP_STATUS, In<PEP_SESSION,false>, Out<stringlist_t*>> ( &blacklist_retrieve) ),
		FP( "OpenPGP_list_keyinfo", new Func<PEP_STATUS, In<PEP_SESSION,false>, In<c_string>, Out<stringpair_list_t*>> ( &OpenPGP_list_keyinfo) ),
		
		FP( "Event Listener & Results", new Separator ),
		FP( "registerEventListener"  , new Func<PEP_STATUS, In<Context*, false>, In<std::string>, In<unsigned>, In<std::string>> ( &registerEventListener) ),
		FP( "unregisterEventListener", new Func<PEP_STATUS, In<Context*, false>, In<std::string>, In<unsigned>, In<std::string>> ( &unregisterEventListener) ),
		FP( "deliverHandshakeResult" , new Func<PEP_STATUS, In<PEP_SESSION,false>, In<pEp_identity*>, In<sync_handshake_result>> (&deliverHandshakeResult) ),
		
		// my own example function that does something useful. :-)
		FP( "Other", new Separator ),
		FP( "version",     new Func<std::string>( &JsonAdapter::version ) ),
		FP( "apiVersion",  new Func<unsigned>   ( &JsonAdapter::apiVersion ) ),
		FP( "getGpgEnvironment", new Func<GpgEnvironment>( &getGpgEnvironment ) ),
	};


std::mutex js_mutex;

// TODO: use && and std::forward<> to avoid copying of the arguments.
// It is not relevant, yet, because at the moment we use this function template only
// for init() and release() which have cheap-to-copy pointer parameters only
template<class R, class... Args>
R call_with_lock( R(*fn)(Args...), Args... args)
{
	std::lock_guard<std::mutex> L(js_mutex);
	return fn(args...);
}


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


void sendFile( evhttp_request* req, const std::string& mimeType, const fs::path& fileName)
{
	auto* outBuf = evhttp_request_get_output_buffer(req);
	if (!outBuf)
		return;
	
	// not the best for big files, but this server does not send big files. :-)
	const std::string fileContent = pEp::utility::slurp(fileName.string());
	evbuffer_add(outBuf, fileContent.data(), fileContent.size());
	evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", mimeType.c_str());
	evhttp_send_reply(req, HTTP_OK, "", outBuf);
}


struct FileRequest
{
	std::string mimeType;
	fs::path    fileName;
};

// catch-all callback
void OnOtherRequest(evhttp_request* req, void*)
{
	static const std::map<std::string, FileRequest > files =
		{
			{ "/"                , {"text/html"      , path_to_html / "index.html"            } },
			{ "/jquery.js"       , {"text/javascript", path_to_html / "jquery-2.2.0.min.js"   } },
			{ "/interactive.js"  , {"text/javascript", path_to_html / "interactive.js"        } },
			{ "/favicon.ico"     , {"image/vnd.microsoft.icon", path_to_html / "json-test.ico"} },
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
	
	JsonAdapter* ja = static_cast<JsonAdapter*>(obj);
	
	std::vector<char> data(length);
	ssize_t nr = evbuffer_copyout(inbuf, data.data(), data.size());
	const std::string data_string(data.data(), data.data() + nr );
	if(nr>0)
	{
		ja->Log() << "\tData: «" << data_string  << "»\n";
		bool b = js::read( data_string, p);
		if(p.type() == js::obj_type)
		{
			const js::Object& request = p.get_obj();
			answer = call( functions, request, ja );
		}else{
			answer = make_error( JSON_RPC::PARSE_ERROR, "evbuffer_copyout does not return a JSON string. b=" + std::to_string(b), js::Value{data_string}, 42 );
		}
	}else{
		ja->Log() << "\tError: " << nr << ".\n";
		answer = make_error( JSON_RPC::INTERNAL_ERROR, "evbuffer_copyout returns negative value", p, request_id );
	}
	
	}
	catch(const std::exception& e)
	{
		std::cerr << "\tException: \"" << e.what() << "\"\n";
		answer = make_error( JSON_RPC::INTERNAL_ERROR, "Got a std::exception: \"" + std::string(e.what()) + "\"", p, request_id );
	}

	sendReplyString(req, "text/plain", js::write(answer, js::raw_utf8));
};


} // end of anonymous namespace


typedef std::pair<std::string, unsigned> EventListenerKey;

struct EventListenerValue
{
	std::string securityContext;
	std::unique_ptr<evhttp_connection, decltype(&evhttp_connection_free)> connection = { nullptr, &evhttp_connection_free};
};




struct JsonAdapter::Internal
{
	std::unique_ptr<event_base, decltype(&event_base_free)> eventBase = {nullptr, &event_base_free};
	std::unique_ptr<evhttp, decltype(&evhttp_free)> evHttp = {nullptr, &evhttp_free};
	std::string address;
	std::string token;
	std::map<EventListenerKey, EventListenerValue> eventListener;
	
	std::ostream& Log;
	unsigned    start_port    = 0;
	unsigned    end_port      = 0;
	unsigned    port          = 0;
	unsigned    request_count = 0;
	evutil_socket_t sock      = -1;
	bool        shall_sync    = false; // just hold the value from config/command line.
	bool        running = false;
	bool        silent  = false;
	ThreadPool  threads;
	PEP_SESSION session = nullptr;
	
	// Sync
	locked_queue< sync_msg_t*, &free_sync_msg>  sync_queue;
	PEP_SESSION sync_session = nullptr;
	ThreadPtr   sync_thread{nullptr, ThreadDeleter};
	
	
	explicit Internal(std::ostream& logger, bool _shall_sync)
	: Log(logger)
	, shall_sync(_shall_sync)
	{}
	
	Internal(const Internal&) = delete;
	void operator=(const Internal&) = delete;
	
	~Internal()
	{
		stopSync();
		call_with_lock(&release, session);
		session=nullptr;
	}
	
	void stopSync();
	
	static
	void requestDone(evhttp_request* req, void* userdata)
	{
		// Hum, what is to do here?
	}

	PEP_STATUS deliverRequest(std::pair<const EventListenerKey,EventListenerValue>& e, const js::Object& request)
	{
		const std::string uri = "http://" + e.first.first + ":" + std::to_string(e.first.second) + "/";
		const std::string request_s = js::write(request, js::raw_utf8);
		evhttp_request* ereq = evhttp_request_new( &requestDone, &e ); // ownership goes to the connection in evhttp_make_request() below.
		evhttp_add_header(ereq->output_headers, "Host", e.first.first.c_str());
		evhttp_add_header(ereq->output_headers, "Content-Length", std::to_string(request_s.length()).c_str());
		auto output_buffer = evhttp_request_get_output_buffer(ereq);
		evbuffer_add(output_buffer, request_s.data(), request_s.size());
		
		const int ret = evhttp_make_request(e.second.connection.get(), ereq, EVHTTP_REQ_POST, uri.c_str() );
		
		return (ret == 0) ? PEP_STATUS_OK : PEP_UNKNOWN_ERROR;
	}
	
	PEP_STATUS makeAndDeliverRequest(const char* function_name, const js::Array& params)
	{
		PEP_STATUS status = PEP_STATUS_OK;
		for(auto& e : eventListener)
		{
			js::Object request = make_request( function_name, params, e.second.securityContext );
			const PEP_STATUS s2 = deliverRequest( e, request );
			if(s2!=PEP_STATUS_OK)
			{
				status = s2;
			}
		}
		return status;
	}
	
	
	static void addToArray(js::Array&) { /* do nothing */ }
	
	template<class T, class... Rest>
	static void addToArray(js::Array& a, const In<T>& in, Rest&&... rest)
	{
		a.push_back( in.to_json() );
		addToArray( a, rest... );
	}
	
	template<class... Params>
	PEP_STATUS makeAndDeliverRequest2(const char* msg_name, Params&&... params)
	{
		js::Array param_array;
		addToArray( param_array, params...);
		return makeAndDeliverRequest(msg_name, param_array);
	}
	
	int injectSyncMsg(void* msg)
	{
		sync_queue.push_back( static_cast<sync_msg_t*>(msg) );
		return 0;
	}
	
	int injectIdentity(pEp_identity* idy)
	{
		keyserver_lookup_queue.push_back(idy);
		return 0;
	}
	
	void* retrieveNextSyncMsg(time_t* timeout)
	{
        sync_msg_t* msg = nullptr;
        if(timeout && *timeout) {
            std::chrono::steady_clock::time_point end_time = std::chrono::steady_clock::now()
                + std::chrono::seconds(*timeout);

            const bool success = sync_queue.try_pop_front(msg, end_time);
            if(!success)
            {
                // this is timeout occurrence
                return nullptr;
            }

            // we got a message while waiting for timeout -> compute remaining time
            std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
            if (now < end_time) 
            {
                *timeout = std::chrono::duration_cast<std::chrono::seconds>(end_time - now).count();
            } 
            else 
            {
                *timeout = 0;
            }

        }else{
            msg = sync_queue.pop_front();
        }
        return msg;
	}
	
	pEp_identity* retrieveNextIdentity()
	{
		return keyserver_lookup_queue.pop_front();
	}
	
	void* syncThreadRoutine(void* arg)
	{
		PEP_STATUS status = do_sync_protocol(sync_session, arg); // does the whole work
		sync_queue.clear(); // remove remaining messages
		return (void*) status;
	}
};


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
PEP_SESSION from_json(const js::Value& /* not used */)
{
	const auto id = std::this_thread::get_id();
	const auto q = session_registry.find( id );
	if(q == session_registry.end())
	{
		std::stringstream ss;
		ss << "There is no SESSION for this thread (" << id << ")!"; 
		throw std::logic_error( ss.str() );
	}
	return q->second->i->session;
}


template<>
JsonAdapter* from_json(const js::Value& /* not used */)
{
	const auto id = std::this_thread::get_id();
	const auto q = session_registry.find( id );
	if(q == session_registry.end())
	{
		std::stringstream ss;
		ss << "There is no JsonAdapter registered for this thread (" << id << ")!"; 
		throw std::logic_error( ss.str() );
	}
	return q->second;
}


template<>
In<JsonAdapter*, false>::~In()
{
	// nothing to do here. :-D
}


std::string JsonAdapter::version()
{
	return server_version;
}


unsigned JsonAdapter::apiVersion()
{
	return API_VERSION;
}





PEP_STATUS JsonAdapter::messageToSend(void* obj, message* msg)
{
	JsonAdapter* ja = static_cast<JsonAdapter*>(obj);
	return ja->i->makeAndDeliverRequest2("messageToSend", In<message*>(msg) );
}


PEP_STATUS JsonAdapter::notifyHandshake(void* obj, pEp_identity* self, pEp_identity* partner, sync_handshake_signal sig)
{
	JsonAdapter* ja = static_cast<JsonAdapter*>(obj);
	return ja->i->makeAndDeliverRequest2("notifyHandshake", In<pEp_identity*>(self), In<pEp_identity*>(partner), In<sync_handshake_signal>(sig) );
}


// BEWARE: msg is 1st parameter, obj is second!!!
int JsonAdapter::injectSyncMsg(void* msg, void* obj)
{
	JsonAdapter* ja = static_cast<JsonAdapter*>(obj);
	return ja->i->injectSyncMsg(msg);
}


void* JsonAdapter::retrieveNextSyncMsg(void* obj, time_t* timeout)
{
	JsonAdapter* ja = static_cast<JsonAdapter*>(obj);
	return ja->i->retrieveNextSyncMsg(timeout);
}


void* JsonAdapter::syncThreadRoutine(void* arg)
{
	JsonAdapter* ja = static_cast<JsonAdapter*>(arg);
	return ja->i->syncThreadRoutine(arg);
}


void JsonAdapter::startSync()
{
	if(i->sync_session)
	{
		throw std::runtime_error("sync session already started!");
	}
	
	PEP_STATUS status = call_with_lock(&init, &i->sync_session);
	if(status != PEP_STATUS_OK || i->sync_session==nullptr)
	{
		throw std::runtime_error("Cannot create sync session! status: " + status_to_string(status));
	}
	
	i->sync_queue.clear();
	
	status = register_sync_callbacks(i->sync_session,
	                                 (void*) this,
	                                 JsonAdapter::messageToSend,
	                                 JsonAdapter::notifyHandshake,
	                                 JsonAdapter::injectSyncMsg,
	                                 JsonAdapter::retrieveNextSyncMsg);
	if (status != PEP_STATUS_OK)
		throw std::runtime_error("Cannot register sync callbacks! status: " + status_to_string(status));
	
	status = attach_sync_session(i->session, i->sync_session);
	if(status != PEP_STATUS_OK)
		throw std::runtime_error("Cannot attach to sync session! status: " + status_to_string(status));
	
	i->sync_thread.reset( new std::thread( JsonAdapter::syncThreadRoutine, (void*)this ) );
}


void JsonAdapter::stopSync()
{
	i->stopSync();
}


void JsonAdapter::Internal::stopSync()
{
	// No sync session active
	if(sync_session == nullptr)
		return;
	
	sync_queue.push_front(NULL);
	sync_thread->join();
	
	unregister_sync_callbacks(sync_session);
	sync_queue.clear();
	
	call_with_lock(&release, sync_session);
	sync_session = nullptr;
}


void JsonAdapter::startKeyserverLookup()
{
	if(keyserver_lookup_session)
		throw std::runtime_error("KeyserverLookup already started.");

	PEP_STATUS status = call_with_lock(&init, &keyserver_lookup_session);
	if(status != PEP_STATUS_OK || keyserver_lookup_session==nullptr)
	{
		throw std::runtime_error("Cannot create keyserver lookup session! status: " + status_to_string(status));
	}
	
	keyserver_lookup_queue.clear();
	status = register_examine_function(keyserver_lookup_session,
			JsonAdapter::examineIdentity,
			&keyserver_lookup_session // nullptr is not accepted, so any dummy ptr is used here
			);
	if (status != PEP_STATUS_OK)
		throw std::runtime_error("Cannot register keyserver lookup callbacks! status: " + status_to_string(status));
	
	keyserver_lookup_thread.reset( new std::thread( JsonAdapter::keyserverLookupThreadRoutine, &keyserver_lookup_session /* just a dummy */ ) );
}


void JsonAdapter::stopKeyserverLookup()
{
	// No keyserver lookup session active
	if(keyserver_lookup_session == nullptr)
		return;
	
	keyserver_lookup_queue.push_front(NULL);
	keyserver_lookup_thread->join();

	// there is no unregister_examine_callback() function. hum...
	keyserver_lookup_queue.clear();
	call_with_lock(&release, keyserver_lookup_session);
	keyserver_lookup_session = nullptr;
}


int JsonAdapter::examineIdentity(pEp_identity* idy, void* obj)
{
	JsonAdapter* ja = static_cast<JsonAdapter*>(obj);
	return ja->i->injectIdentity(idy);
}


pEp_identity* JsonAdapter::retrieveNextIdentity(void* obj)
{
	JsonAdapter* ja = static_cast<JsonAdapter*>(obj);
	return ja->i->retrieveNextIdentity();
}


void* JsonAdapter::keyserverLookupThreadRoutine(void* arg)
{
	PEP_STATUS status = do_keymanagement(&JsonAdapter::retrieveNextIdentity, arg); // does the whole work
	keyserver_lookup_queue.clear();
	return (void*) status;
}


JsonAdapter::JsonAdapter(const std::string& address, unsigned start_port, unsigned end_port, bool silent, bool do_sync)
: i(new Internal( silent ? nulllogger : std::cerr, do_sync ))
{
	i->eventBase.reset(event_base_new());
	if (!i->eventBase)
		throw std::runtime_error("Failed to create new base_event.");
	
	i->evHttp.reset( evhttp_new(i->eventBase.get()) );
	if (!i->evHttp)
		throw std::runtime_error("Failed to create new evhttp.");
	
	i->address    = address;
	i->start_port = start_port;
	i->end_port   = end_port;
	i->silent     = silent;
}


JsonAdapter::~JsonAdapter()
{
	Log() << "~JsonAdapter(): " << session_registry.size() << " sessions registered.\n";
	stopSync();
	this->shutdown(nullptr);
	delete i;
	i=nullptr;
	Log() << "\t After stopSync() and shutdown() there are " << session_registry.size() << " sessions registered.\n";
}


void JsonAdapter::run()
try
{
	Log() << "I have " << session_registry.size() << " registered session(s).\n";
	JsonAdapter* ja = this;
	
	std::exception_ptr initExcept;
	auto ThreadFunc = [&] ()
	{
		try
		{
			const auto id = std::this_thread::get_id();
			Log() << " +++ Thread starts: isRun=" << i->running << ", id=" << id << ". +++\n";
			const auto q=session_registry.find(id);
			if(q==session_registry.end())
			{
				i->session = nullptr;
				PEP_STATUS status = call_with_lock(&init, &i->session); // release(session) in ThreadDeleter
				if(status != PEP_STATUS_OK || i->session==nullptr)
				{
					throw std::runtime_error("Cannot create session! status: " + status_to_string(status));
				}
				
				session_registry.emplace(id, ja);
				Log() << "\tcreated new session for this thread: " << static_cast<void*>(i->session) << ".\n";
				if(i->shall_sync)
				{
					Log() << "\tstartSync()...\n";
					startSync();
				}
			}else{
				Log() << "\tsession for this thread: "  << static_cast<void*>(q->second) << ".\n";
			}
			
			evhttp_set_cb(i->evHttp.get(), ApiRequestUrl.c_str()    , OnApiRequest    , this);
			evhttp_set_cb(i->evHttp.get(), "/pep_functions.js"      , OnGetFunctions  , this);
			evhttp_set_gencb(i->evHttp.get(), OnOtherRequest, nullptr);
			
			if (i->sock == -1) // no port bound, yet
			{
				// initialize the pEp engine
				Log() << "I have " << session_registry.size() << " registered session(s).\n";
				
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
				
				Log() << "Bound to port " << i->port << ", sec_token=\"" << i->token << "\"\n";
			}
			else
			{
				if (evhttp_accept_socket(i->evHttp.get(), i->sock) == -1)
					throw std::runtime_error("Failed to accept() on server socket for new instance.");
			}
			
			unsigned numnum = 1000000;
			while(i->running)
			{
				// once we have libevent 2.1:
				//event_base_loop(i->eventBase.get(), EVLOOP_NO_EXIT_ON_EMPTY);
				
				// for libevent 2.0:
				event_base_loop(i->eventBase.get(), EVLOOP_NONBLOCK);
				std::this_thread::sleep_for(std::chrono::milliseconds(333));
				Log() << "\r" << ++numnum << ".   ";
			}
		}
		catch (const std::exception& e)
		{
			Log() << " +++ std::exception in ThreadFunc: " << e.what() << "\n";
			initExcept = std::current_exception();
		}
		catch (...)
		{
			Log() << " +++ UNKNOWN EXCEPTION in ThreadFunc +++ ";
			initExcept = std::current_exception();
		}
		Log() << " +++ Thread exit? isRun=" << i->running << ", id=" << std::this_thread::get_id() << ". +++\n";
	};
	
	i->running = true;
	for(int t=0; t<SrvThreadCount; ++t)
	{
		Log() << "Start Thread #" << t << "...\n";
		ThreadPtr thread(new std::thread(ThreadFunc), ThreadDeleter);
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		if (initExcept != std::exception_ptr())
		{
			i->running = false;
			std::rethrow_exception(initExcept);
		}
		i->threads.push_back(std::move(thread));
	}
	Log() << "All " << SrvThreadCount << " thread(s) started:\n";
	for(const auto& t:i->threads)
	{
		Log() << "\tthread_id()=" << t->get_id() << ".\n";
	}
}
catch (std::exception const &e)
{
	Log() << "Exception catched in main(): \"" << e.what() << "\"" << std::endl;
	throw;
}


void JsonAdapter::shutdown(timeval* t)
{
	Log() << "JS::shutdown() was called.\n";
	i->running = false;
	const int ret = event_base_loopexit(i->eventBase.get(), t);
	if(ret!=0)
	{
		throw std::runtime_error("JsonAdapter::shutdown() failed.");
	}
	Log() << "JS::shutdown(): event_base loop is finished.\n";
	Log() << "\t there are " << i->threads.size() << " threads remaining in the threadpool.\n";
	for(const auto& t : i->threads)
	{
		Log() << "\t\tjoin() on id=" << t->get_id() << "....\n";
		t->join();
	}
	i->threads.clear();
}


// returns 'true' if 's' is the security token created by the function above.
bool JsonAdapter::verify_security_token(const std::string& s) const
{
	if(s!=i->token)
	{
		Log() << "sec_token=\"" << i->token << "\" (len=" << i->token.size() << ") is unequal to \"" << s << "\" (len=" << s.size() << ")!\n";
	}
	return s == i->token;
}


void JsonAdapter::augment(json_spirit::Object& returnObject)
{
	PEP_SESSION session = from_json<PEP_SESSION>(returnObject); // the parameter is not used :-D
	auto errorstack = get_errorstack(session);
	returnObject.emplace_back( "errorstack", to_json(errorstack) );
	clear_errorstack(session);
}


void JsonAdapter::registerEventListener(const std::string& address, unsigned port, const std::string& securityContext)
{
	const auto key = std::make_pair(address, port);
	const auto q = i->eventListener.find(key);
	if( q != i->eventListener.end() && q->second.securityContext != securityContext)
	{
		throw std::runtime_error("EventListener at host \"" + address + "\":" + std::to_string(port) + " is already registered with different securityContext." );
	}
	
	EventListenerValue v;
	v.securityContext = securityContext;
	v.connection.reset( evhttp_connection_base_new( i->eventBase.get(), nullptr, address.c_str(), port ) );
	i->eventListener[key] = std::move(v);
}


void JsonAdapter::unregisterEventListener(const std::string& address, unsigned port, const std::string& securityContext)
{
	const auto key = std::make_pair(address, port);
	const auto q = i->eventListener.find(key);
	if( q == i->eventListener.end() || q->second.securityContext != securityContext)
	{
		throw std::runtime_error("Cannot unregister EventListener at host \"" + address + "\":" + std::to_string(port) + ". Not registered or wrong securityContext." );
	}
	
	i->eventListener.erase(q);
}


std::ostream& JsonAdapter::Log() const
{
	return i->Log;
}


bool JsonAdapter::running() const
{
	return i->running;
}
