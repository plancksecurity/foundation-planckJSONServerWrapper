#include <pEp/webserver.hh>

#include "ev_server.hh"
#include "c_string.hh"
#include "prefix-config.hh"
#include "json-adapter.hh"
#include "function_map.hh"
#include "pEp-types.hh"
#include "json_rpc.hh"
#include "pEp-utils.hh"
#include "logger.hh"
#include "server_version.hh"

#include <pEp/message_api.h>
#include <pEp/blacklist.h>
#include <pEp/key_reset.h>
#include <pEp/openpgp_compat.h>
#include <pEp/message_api.h> // for get_binary_path()
#include <pEp/mime.h>

// libpEpAdapter:
#include <pEp/status_to_string.hh>
#include <pEp/slurp.hh>

#include <boost/filesystem.hpp>
#include "json_spirit/json_spirit_reader.h"

// HACK:
#include "mini-adapter-impl.hh"


namespace fs = boost::filesystem;

// compile-time default. might be overwritten in main() or before any ev_server function is called.
fs::path ev_server::path_to_html = fs::path(html_directory);


namespace {


std::string version_as_a_string()
{
	std::stringstream ss;
	ss << server_version();
	return ss.str();
}

std::string getBinaryPath()
{
	const char* gpg_path = nullptr;
	const auto status = get_binary_path( PEP_crypt_OpenPGP, &gpg_path);
	if(status == PEP_STATUS_OK && gpg_path)
	{
		return std::string(gpg_path);
	}
	
	throw std::runtime_error("getBinaryPath returns error: " + ::pEp::status_to_string(status) );
}


// these are the pEp functions that are callable by the client
const FunctionMap functions = {

		// from message_api.h
		FP( "Message API", new Separator ),
		FP( "encrypt_message", new Func<PEP_STATUS, In_Pep_Session, InOut<message*>, In<stringlist_t*>, Out<message*>, In<PEP_enc_format>, In<PEP_encrypt_flags_t>>( &encrypt_message ) ),
		FP( "encrypt_message_and_add_priv_key", new Func<PEP_STATUS, In_Pep_Session,
			In<message*>, Out<message*>, In<c_string>, In<PEP_enc_format>, In<PEP_encrypt_flags_t>>( &encrypt_message_and_add_priv_key) ),
		FP( "encrypt_message_for_self", new Func<PEP_STATUS, In_Pep_Session,
			In<pEp_identity*>, In<message*>, In<stringlist_t*>, Out<message*>, In<PEP_enc_format>, In<PEP_encrypt_flags_t>>( &encrypt_message_for_self ) ),

		FP( "decrypt_message", new Func<PEP_STATUS, In_Pep_Session, InOut<message*>, Out<message*>, InOutP<stringlist_t*>, Out<PEP_rating>, InOutP<PEP_decrypt_flags_t>>(  &decrypt_message ) ),
		FP( "get_key_rating_for_user", new Func<PEP_STATUS, In_Pep_Session, In<c_string>, In<c_string>, Out<PEP_rating>>( &get_key_rating_for_user) ),
		
		// from mime.h
		FP( "MIME handling API", new Separator),
		FP( "mime_encode_message", new Func<PEP_STATUS, In<const message*>, In<bool>, Out<char*>, In<bool, ParamFlag::NoInput>>( &mime_encode_message )),
		FP( "mime_decode_message", new Func<PEP_STATUS, In<c_string>, InLength<>, Out<message*>, In<bool*, ParamFlag::NoInput>>( &mime_decode_message )),
		
		// from pEpEngine.h
		FP( "pEp Engine Core API", new Separator),
//		FP( "log_event",  new Func<PEP_STATUS, In_Pep_Session, In<c_string>, In<c_string>, In<c_string>, In<c_string>>( &log_event) ),
		FP( "get_trustwords", new Func<PEP_STATUS, In_Pep_Session, In<const pEp_identity*>, In<const pEp_identity*>, In<Language>, Out<char*>, Out<size_t>, In<bool>>( &get_trustwords) ),
		FP( "re_evaluate_message_rating", new Func<PEP_STATUS, In_Pep_Session, In<message *>, In<stringlist_t *>, In<PEP_rating>, Out<PEP_rating>>( &re_evaluate_message_rating ) ),
		FP( "get_languagelist", new Func<PEP_STATUS, In_Pep_Session, Out<char*>>( &get_languagelist) ),
//		FP( "get_phrase"      , new Func<PEP_STATUS, In_Pep_Session, In<Language>, In<int>, Out<char*>> ( &get_phrase) ),
//		FP( "get_engine_version", new Func<const char*> ( &get_engine_version) ),
		FP( "is_pEp_user"     , new Func<PEP_STATUS, In_Pep_Session, In<pEp_identity*>, Out<bool>>( &is_pEp_user) ),
		FP( "config_passive_mode", new Func<void, In_Pep_Session, In<bool>>( &config_passive_mode) ),
		FP( "config_unencrypted_subject", new Func<void, In_Pep_Session, In<bool>>( &config_unencrypted_subject) ),
		
		FP( "Identity Management API", new Separator),
		FP( "get_identity"       , new Func<PEP_STATUS, In_Pep_Session, In<c_string>, In<c_string>, Out<pEp_identity*>>( &get_identity) ),
		FP( "set_identity"       , new Func<PEP_STATUS, In_Pep_Session, In<const pEp_identity*>> ( &set_identity) ),
		FP( "mark_as_comprimized", new Func<PEP_STATUS, In_Pep_Session, In<c_string>> ( &mark_as_compromized) ),
		FP( "identity_rating"    , new Func<PEP_STATUS, In_Pep_Session, In<pEp_identity*>, Out<PEP_rating>>( &identity_rating) ),
		FP( "outgoing_message_rating", new Func<PEP_STATUS, In_Pep_Session, In<message*>, Out<PEP_rating>>( &outgoing_message_rating) ),
		FP( "outgoing_message_rating_preview", new Func<PEP_STATUS, In_Pep_Session, In<message*>, Out<PEP_rating>>( &outgoing_message_rating_preview) ),
		FP( "set_identity_flags"     , new Func<PEP_STATUS, In_Pep_Session, InOut<pEp_identity*>, In<identity_flags_t>>( &set_identity_flags) ),
		FP( "unset_identity_flags"   , new Func<PEP_STATUS, In_Pep_Session, InOut<pEp_identity*>, In<identity_flags_t>>( &unset_identity_flags) ),
		
		FP( "Low level Key Management API", new Separator),
		FP( "generate_keypair", new Func<PEP_STATUS, In_Pep_Session, InOut<pEp_identity*>> ( &generate_keypair) ),
		FP( "delete_keypair", new Func<PEP_STATUS, In_Pep_Session, In<c_string>> ( &delete_keypair) ),
		FP( "import_key"    , new Func<PEP_STATUS, In_Pep_Session, In<c_string>, In<std::size_t>, Out<identity_list*>> ( &import_key) ),
		FP( "export_key"    , new Func<PEP_STATUS, In_Pep_Session, In<c_string>, Out<char*>, Out<std::size_t>> ( &export_key) ),
		FP( "find_keys"     , new Func<PEP_STATUS, In_Pep_Session, In<c_string>, Out<stringlist_t*>> ( &find_keys) ),
		FP( "get_trust"     , new Func<PEP_STATUS, In_Pep_Session, InOut<pEp_identity*>> ( &get_trust) ),
		FP( "own_key_is_listed", new Func<PEP_STATUS, In_Pep_Session, In<c_string>, Out<bool>> ( &own_key_is_listed) ),
		FP( "own_identities_retrieve", new Func<PEP_STATUS, In_Pep_Session, Out<identity_list*>>( &own_identities_retrieve ) ),
		FP( "set_own_key", new Func<PEP_STATUS, In_Pep_Session, InOut<pEp_identity*>, In<c_string>>( &set_own_key ) ),
		FP( "key_reset_identity", new Func<PEP_STATUS, In_Pep_Session, In<pEp_identity*>, In<c_string>>( &key_reset_identity) ),
		FP( "key_reset_user",     new Func<PEP_STATUS, In_Pep_Session, In<c_string>     , In<c_string, ParamFlag::NullOkay>>( &key_reset_user) ),
		FP( "key_reset_all_own_keys",  new Func<PEP_STATUS, In_Pep_Session>( &key_reset_all_own_keys) ),
		
		FP( "myself"        , new Func<PEP_STATUS, In_Pep_Session, InOut<pEp_identity*>> ( &myself) ),
		FP( "update_identity", new Func<PEP_STATUS, In_Pep_Session, InOut<pEp_identity*>> ( &update_identity) ),
		
		FP( "trust_personal_key", new Func<PEP_STATUS, In_Pep_Session, In<pEp_identity*>>( &trust_personal_key) ),
		FP( "trust_own_key",      new Func<PEP_STATUS, In_Pep_Session, In<pEp_identity*>>( &trust_own_key) ),
		FP( "key_mistrusted",     new Func<PEP_STATUS, In_Pep_Session, In<pEp_identity*>>( &key_mistrusted) ),
		FP( "key_reset_trust",    new Func<PEP_STATUS, In_Pep_Session, In<pEp_identity*>>( &key_reset_trust) ),
		
		FP( "least_trust"   , new Func<PEP_STATUS, In_Pep_Session, In<c_string>, Out<PEP_comm_type>> ( &least_trust) ),
		FP( "get_key_rating", new Func<PEP_STATUS, In_Pep_Session, In<c_string>, Out<PEP_comm_type>> ( &get_key_rating) ),
		FP( "renew_key"     , new Func<PEP_STATUS, In_Pep_Session, In<c_string>, In<const timestamp*>> ( &renew_key) ),
		FP( "revoke"        , new Func<PEP_STATUS, In_Pep_Session, In<c_string>, In<c_string>> ( &revoke_key) ),
		FP( "key_expired"   , new Func<PEP_STATUS, In_Pep_Session, In<c_string>, In<time_t>, Out<bool>> ( &key_expired) ),
		
		FP( "from blacklist.h & OpenPGP_compat.h", new Separator),
		FP( "blacklist_add"   , new Func<PEP_STATUS, In_Pep_Session, In<c_string>> ( &blacklist_add) ),
		FP( "blacklist_delete", new Func<PEP_STATUS, In_Pep_Session, In<c_string>> ( &blacklist_delete) ),
		FP( "blacklist_is_listed", new Func<PEP_STATUS, In_Pep_Session, In<c_string>, Out<bool>> ( &blacklist_is_listed) ),
		FP( "blacklist_retrieve" , new Func<PEP_STATUS, In_Pep_Session, Out<stringlist_t*>> ( &blacklist_retrieve) ),
		FP( "OpenPGP_list_keyinfo", new Func<PEP_STATUS, In_Pep_Session, In<c_string>, Out<stringpair_list_t*>> ( &OpenPGP_list_keyinfo) ),
		
		FP( "Event Listener & Results", new Separator ),
		FP( "deliverHandshakeResult" , new Func<PEP_STATUS, In_Pep_Session, In<sync_handshake_result>, In<const identity_list*> > (&deliverHandshakeResult) ),
		FP( "pollForEvents"          , new Func<js::Array, In<JsonAdapter*,ParamFlag::NoInput>, In<unsigned>> (&JsonAdapter::pollForEvents) ),
		
		FP( "Sync", new Separator ),
		FP( "leave_device_group"       , new Func<PEP_STATUS, In_Pep_Session> (&leave_device_group) ),
		FP( "enable_identity_for_sync" , new Func<PEP_STATUS, In_Pep_Session, InOut<pEp_identity*>> (&enable_identity_for_sync)),
		FP( "disable_identity_for_sync", new Func<PEP_STATUS, In_Pep_Session, InOut<pEp_identity*>> (&disable_identity_for_sync)),
		FP( "startSync", new Func<void> (&pEp::mini::startSync) ),
		FP( "stopSync" , new Func<void> (&pEp::mini::stopSync) ),
		
		// my own example function that does something useful. :-)
		FP( "Other", new Separator ),
		FP( "serverVersion", new Func<ServerVersion>( &server_version ) ),
		FP( "version",       new Func<std::string>( &version_as_a_string ) ),
		FP( "getBinaryPath", new Func<std::string>( &getBinaryPath ) ),
		FP( "testMessageToSend", new Func<PEP_STATUS, In<message*>> (&JsonAdapter::messageToSend) ),

		FP( "shutdown",  new Func<void, In<JsonAdapter*,ParamFlag::NoInput>>( &JsonAdapter::shutdown_now ) ),
	};
 

	bool add_sharks = false;

} // end of anonymous namespace


pEp::Webserver::response ev_server::sendReplyString(const pEp::Webserver::request& req, const char* contentType, std::string&& outputText)
{
	Log() << Logger::Debug << "sendReplyString(): "
		<< ", contentType=" << (contentType ? "«" + std::string(contentType)+ "»" : "NULL")
		<< ", output.size()=«" << outputText.size() << "».";
	
	pEp::Webserver::response res{pEp::http::status::ok, req.version()};
	res.set(pEp::http::field::content_type, contentType);
	res.keep_alive(req.keep_alive());
	res.content_length(outputText.size());
	res.body() = std::move(outputText);

	return res;
}


pEp::Webserver::response ev_server::sendFile(const pEp::Webserver::request& req, const char* mimeType, const fs::path& fileName)
{
	// not the best for big files, but this server does not send big files. :-)
	std::string fileContent = pEp::slurp(fileName.string());
	return sendReplyString(req, mimeType, std::move(fileContent));
}


struct FileRequest
{
	const char* mimeType;
	fs::path    fileName;
};

// catch-all callback
pEp::Webserver::response ev_server::OnOtherRequest(boost::cmatch match, const pEp::Webserver::request& req)
{
	static const std::map<std::string, FileRequest > files =
		{
			{ "/"                , {"text/html"      , path_to_html / "index.html"            } },
			{ "/jquery.js"       , {"text/javascript", path_to_html / "jquery-2.2.0.min.js"   } },
			{ "/interactive.js"  , {"text/javascript", path_to_html / "interactive.js"        } },
			{ "/favicon.ico"     , {"image/vnd.microsoft.icon", path_to_html / "json-test.ico"} },
		};
	
	const std::string path = req.target().to_string(); // NB: is percent-encoded! does not relevant for the supported paths above.
	
	Log() << Logger::Debug << "** Request: [" << req.method_string().to_string() << "] " << "Path: [" + path + "]";
	
	try{
		const auto q = files.find(path);
		if(q != files.end()) // found in "files" map
		{
			Log() << Logger::Debug << "\t found file \"" << q->second.fileName.string() << "\", type=" << q->second.mimeType << ".\n";
			return sendFile( req, q->second.mimeType, q->second.fileName);
		}
		
		return pEp::Webserver::create_status_response(req, pEp::http::status::not_found);
	}
	catch(const std::runtime_error& e)
	{
		const std::string error_msg = "Internal error caused by path \"" + path + "\"";
		// Log e.what() to log file, but DON'T send it in HTTP error message
		// because it might contain sensitive information, e.g. local file paths etc.!
		Log() << Logger::Error << "OnOtherRequest: " << error_msg << ".  what:" << e.what();
		return pEp::Webserver::create_status_response(req, pEp::http::status::internal_server_error );
	}
};


// generate a JavaScript file containing the definition of all registered callable functions, see above.
pEp::Webserver::response ev_server::OnGetFunctions(boost::cmatch match, const pEp::Webserver::request& req)
{
	static const auto& version = server_version();
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
		"var server_version_name = \"" + version.name + "\";\n"
		"var server_version = \"" + version.major_minor_patch() + "\";\n"
		"var add_sharks = " + (add_sharks?"true":"false") + ";\n"
		"var pEp_functions = ";
	
	js::Array jsonfunctions;
	for(const auto& f : functions)
	{
		js::Object o;
		o.emplace_back( "name", f.first );
		f.second->setJavaScriptSignature( o );
		jsonfunctions.push_back( o );
	}
	
	std::string output = preamble + js::write( jsonfunctions, js::pretty_print | js::raw_utf8 | js::single_line_arrays )
		+ ";\n"
		"\n"
		"// End of generated file.\n";
		
	return sendReplyString(req, "text/javascript", std::move(output));
}


pEp::Webserver::response ev_server::OnApiRequest(boost::cmatch match, const pEp::Webserver::request& req)
{
	Logger L( Log(), "OnApiReq");

	int request_id = -42;
	js::Object answer;
	js::Value p;
	
	try
	{
	
	JsonAdapter& ja = JsonAdapter::getInstance();
	
	const std::string data_string = req.body();
	if(!data_string.empty())
	{
		L << Logger::Debug << "Data: «" << data_string  << "»";
		bool b = js::read( data_string, p);
		if(p.type() == js::obj_type)
		{
			const js::Object& request = p.get_obj();
			answer = call( functions, request, &ja );
		}else{
			const std::string error_msg = "evbuffer_copyout does not return a JSON string. b=" + std::to_string(b);
			L << Logger::Error << error_msg;
			answer = make_error( JSON_RPC::PARSE_ERROR, error_msg, js::Value{data_string}, 42 );
		}
	}else{
		answer = make_error( JSON_RPC::INTERNAL_ERROR, "ZERO size request", p, request_id );
	}
	
	}
	catch(const std::exception& e)
	{
		L << Logger::Error << "Exception: \"" << e.what() << "\"";
		answer = make_error( JSON_RPC::INTERNAL_ERROR, "Got a std::exception: \"" + std::string(e.what()) + "\"", p, request_id );
	}

	return sendReplyString(req, "text/plain", js::write(answer, js::raw_utf8));
};


Logger& ev_server::Log()
{
	static Logger L("evs");
	return L;
}


void ev_server::addSharks()
{
	add_sharks = true;
}
