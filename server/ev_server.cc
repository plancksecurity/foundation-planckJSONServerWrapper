#include <evhttp.h>

#include "ev_server.hh"
#include "json-adapter.hh"
#include "function_map.hh"
#include "pep-types.hh"
#include "json_rpc.hh"
#include "pep-utils.hh"
#include "gpg_environment.hh"
#include "prefix-config.hh"
#include "server_version.hh"

#include <pEp/message_api.h>
#include <pEp/blacklist.h>
#include <pEp/openpgp_compat.h>
#include <pEp/mime.h>

#include <boost/filesystem.hpp>
#include "json_spirit/json_spirit_reader.h"


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


namespace ev_server {

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
		FP( "MIME_encrypt_message", new Func<PEP_STATUS, In<PEP_SESSION, false>, In<const char*>, In<size_t>, In<stringlist_t*>,
			Out<char*>, In<PEP_enc_format>, In<PEP_encrypt_flags_t>>( &MIME_encrypt_message ) ),
		FP( "MIME_encrypt_message_for_self", new Func<PEP_STATUS, In<PEP_SESSION, false>, In<pEp_identity*>, In<const char*>, In<size_t>,
			Out<char*>, In<PEP_enc_format>, In<PEP_encrypt_flags_t>>( &MIME_encrypt_message_for_self ) ),
		FP( "MIME_decrypt_message", new Func<PEP_STATUS, In<PEP_SESSION, false>, In<const char*>, In<size_t>,
			Out<char*>, Out<stringlist_t*>, Out<PEP_rating>, Out<PEP_decrypt_flags_t>>( &MIME_decrypt_message ) ),
		
		// HACK: because "auto sessions" are per TCP connections, add a parameter to set passive_mode each time again
		FP( "MIME_encrypt_message_ex", new Func<PEP_STATUS, In<PEP_SESSION, false>, In<const char*>, In<size_t>, In<stringlist_t*>, In<bool>,
			Out<char*>, In<PEP_enc_format>, In<PEP_encrypt_flags_t>>( &MIME_encrypt_message_ex ) ),
		FP( "MIME_decrypt_message_ex", new Func<PEP_STATUS, In<PEP_SESSION, false>, In<const char*>, In<size_t>, In<bool>,
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
		FP( "log_event",  new Func<PEP_STATUS, In<PEP_SESSION,false>, In<const char*>, In<const char*>, In<const char*>, In<const char*>>( &log_event) ),
		FP( "get_trustwords", new Func<PEP_STATUS, In<PEP_SESSION,false>, In<const pEp_identity*>, In<const pEp_identity*>, In<Language>, Out<char*>, Out<size_t>, In<bool>>( &get_trustwords) ),
		FP( "get_languagelist", new Func<PEP_STATUS, In<PEP_SESSION,false>, Out<char*>>( &get_languagelist) ),
		FP( "get_phrase"      , new Func<PEP_STATUS, In<PEP_SESSION,false>, In<Language>, In<int>, Out<char*>> ( &get_phrase) ),
		FP( "get_engine_version", new Func<const char*> ( &get_engine_version) ),
		FP( "config_passive_mode", new Func<void, In<PEP_SESSION,false>, In<bool>>( &config_passive_mode) ),
		FP( "config_unencrypted_subject", new Func<void, In<PEP_SESSION,false>, In<bool>>( &config_unencrypted_subject) ),
		
		FP( "Identity Management API", new Separator),
		FP( "get_identity"       , new Func<PEP_STATUS, In<PEP_SESSION,false>, In<const char*>, In<const char*>, Out<pEp_identity*>>( &get_identity) ),
		FP( "set_identity"       , new Func<PEP_STATUS, In<PEP_SESSION,false>, In<pEp_identity*>> ( &set_identity) ),
		FP( "mark_as_comprimized", new Func<PEP_STATUS, In<PEP_SESSION,false>, In<const char*>> ( &mark_as_compromized) ),
		FP( "identity_rating"    , new Func<PEP_STATUS, In<PEP_SESSION,false>, In<pEp_identity*>, Out<PEP_rating>>( &identity_rating) ),
		FP( "outgoing_message_rating", new Func<PEP_STATUS, In<PEP_SESSION,false>, In<message*>, Out<PEP_rating>>( &outgoing_message_rating) ),
		FP( "set_identity_flags"     , new Func<PEP_STATUS, In<PEP_SESSION,false>, InOut<pEp_identity*>, In<identity_flags_t>>( &set_identity_flags) ),
		FP( "unset_identity_flags"   , new Func<PEP_STATUS, In<PEP_SESSION,false>, InOut<pEp_identity*>, In<identity_flags_t>>( &unset_identity_flags) ),
		
		FP( "Low level Key Management API", new Separator),
		FP( "generate_keypair", new Func<PEP_STATUS, In<PEP_SESSION,false>, InOut<pEp_identity*>> ( &generate_keypair) ),
		FP( "delete_keypair", new Func<PEP_STATUS, In<PEP_SESSION,false>, In<const char*>> ( &delete_keypair) ),
		FP( "import_key"    , new Func<PEP_STATUS, In<PEP_SESSION,false>, In<const char*>, In<std::size_t>, Out<identity_list*>> ( &import_key) ),
		FP( "export_key"    , new Func<PEP_STATUS, In<PEP_SESSION,false>, In<const char*>, Out<char*>, Out<std::size_t>> ( &export_key) ),
		FP( "find_keys"     , new Func<PEP_STATUS, In<PEP_SESSION,false>, In<const char*>, Out<stringlist_t*>> ( &find_keys) ),
		FP( "get_trust"     , new Func<PEP_STATUS, In<PEP_SESSION,false>, InOut<pEp_identity*>> ( &get_trust) ),
		FP( "own_key_is_listed", new Func<PEP_STATUS, In<PEP_SESSION,false>, In<const char*>, Out<bool>> ( &own_key_is_listed) ),
		FP( "own_identities_retrieve", new Func<PEP_STATUS, In<PEP_SESSION,false>, Out<identity_list*>>( &own_identities_retrieve ) ),
		FP( "undo_last_mitrust", new Func<PEP_STATUS, In<PEP_SESSION,false>>( &undo_last_mistrust ) ),
		
		FP( "myself"        , new Func<PEP_STATUS, In<PEP_SESSION,false>, InOut<pEp_identity*>> ( &myself) ),
		FP( "update_dentity", new Func<PEP_STATUS, In<PEP_SESSION,false>, InOut<pEp_identity*>> ( &update_identity) ),
		
		FP( "trust_personal_key", new Func<PEP_STATUS, In<PEP_SESSION,false>, In<pEp_identity*>>( &trust_personal_key) ),
		FP( "key_mistrusted",     new Func<PEP_STATUS, In<PEP_SESSION,false>, In<pEp_identity*>>( &key_mistrusted) ),
		FP( "key_reset_trust",    new Func<PEP_STATUS, In<PEP_SESSION,false>, In<pEp_identity*>>( &key_reset_trust) ),
		
		FP( "least_trust"   , new Func<PEP_STATUS, In<PEP_SESSION,false>, In<const char*>, Out<PEP_comm_type>> ( &least_trust) ),
		FP( "get_key_rating", new Func<PEP_STATUS, In<PEP_SESSION,false>, In<const char*>, Out<PEP_comm_type>> ( &get_key_rating) ),
		FP( "renew_key"     , new Func<PEP_STATUS, In<PEP_SESSION,false>, In<const char*>, In<const timestamp*>> ( &renew_key) ),
		FP( "revoke"        , new Func<PEP_STATUS, In<PEP_SESSION,false>, In<const char*>, In<const char*>> ( &revoke_key) ),
		FP( "key_expired"   , new Func<PEP_STATUS, In<PEP_SESSION,false>, In<const char*>, In<time_t>, Out<bool>> ( &key_expired) ),
		
		FP( "from blacklist.h & OpenPGP_compat.h", new Separator),
		FP( "blacklist_add"   , new Func<PEP_STATUS, In<PEP_SESSION,false>, In<const char*>> ( &blacklist_add) ),
		FP( "blacklist_delete", new Func<PEP_STATUS, In<PEP_SESSION,false>, In<const char*>> ( &blacklist_delete) ),
		FP( "blacklist_is_listed", new Func<PEP_STATUS, In<PEP_SESSION,false>, In<const char*>, Out<bool>> ( &blacklist_is_listed) ),
		FP( "blacklist_retrieve" , new Func<PEP_STATUS, In<PEP_SESSION,false>, Out<stringlist_t*>> ( &blacklist_retrieve) ),
		FP( "OpenPGP_list_keyinfo", new Func<PEP_STATUS, In<PEP_SESSION,false>, In<const char*>, Out<stringpair_list_t*>> ( &OpenPGP_list_keyinfo) ),
		
		FP( "Event Listener & Results", new Separator ),
		FP( "registerEventListener"  , new Func<void, In<JsonAdapter*, false>, In<std::string>, In<unsigned>, In<std::string>> ( &JsonAdapter::registerEventListener) ),
		FP( "unregisterEventListener", new Func<void, In<JsonAdapter*, false>, In<std::string>, In<unsigned>, In<std::string>> ( &JsonAdapter::unregisterEventListener) ),
		FP( "deliverHandshakeResult" , new Func<PEP_STATUS, In<PEP_SESSION,false>, In<pEp_identity*>, In<sync_handshake_result>> (&deliverHandshakeResult) ),
		
		// my own example function that does something useful. :-)
		FP( "Other", new Separator ),
		FP( "version",     new Func<std::string>( &JsonAdapter::version ) ),
		FP( "apiVersion",  new Func<unsigned>   ( &JsonAdapter::apiVersion ) ),
		FP( "getGpgEnvironment", new Func<GpgEnvironment>( &getGpgEnvironment ) ),
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


static const boost::filesystem::path  path_to_html = fs::path(html_directory);

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


} // end of namespace ev_server

