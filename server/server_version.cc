#include "server_version.hh"
#include "inout.hh"
#include <fstream>
#include <sstream>
#include "logger.hh"
#include "pEp-utils-json.hh"
#include <pEp/pEpEngine.h> // for PEP_VERSION and get_engine_version()
#include <boost/algorithm/string/trim.hpp>
#include "json_spirit/json_spirit_reader.h"

#include <pEp/slurp.hh>

namespace js = json_spirit;

namespace {

#ifdef PACKAGE_VERSION
	const char* PackageVersion = PACKAGE_VERSION;
#else
	const char* PackageVersion = nullptr;
#endif


// version names comes from here:
// https://de.wikipedia.org/wiki/Bundesautobahn_4
static const std::string VersionName =
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
//	"(31) Altenkleusheim";   // JSON-57: change location of server token file. breaking API change, so API_VERSION=0x0003.
//	"(32) Littfeld";         // JSON-72: add is_pep_user() to the API
//	"(33) Hilchenbach";      // JSON-71: Setup C++11 Multi-threading in libevent properly to avoid deadlocks in MT server code"
//	"(34) Erndtebrück";      // remove apiVersion(), change version() to return a semver-compatible version number in a JSON object.
//	"(35) Bad Berleburg";    // fix the fork() problem on MacOS. daemonize() now got a function parameter. \o/
//	"(36) Hatzfeld";         // JSON-81: add package_version, rename "version" into "api_version" in ServerVersion, add versions from the Engine, too
//	"(37) Battenberg";       // JSON-75: change debonize() behavior, especially on MS Windows
//	"(38) Frankenberg";      // JSON-92: API CHANGE: decrypt_message() has InOut src message, MIME_decrypt_message() returns changed src msg, too.
//	"(39) Gemünden";         // JSON-93: support for InLengt<> to calculate string lengths automatically.

//  Renumbering due to political decisions - the planned exits 31..39 through the Sauerland will never be build. :-(
//  So we got a new exit with the same number:
//	"(39) Eisenach"; // JSON-118: fix to_json() for KeySync callbacks to avoid crashes. Add attachment support in interactive.js \o/
//	"(40) Eisenach-Ost"; // remove all Enigmail leftovers. Bump API version to 0.17.0
	"(40b) Sättelstädt"; // JSON-139: support for NULL pointers in "const char*" parameters: In<c_string, NullOkay>

} // end of anonymous namespace
////////////////////////////////////////////////////////////////////////////

const ServerVersion& server_version()
{

//const ServerVersion sv{0, 10, 0, version_name};  // first version defined.
//const ServerVersion sv{0, 11, 0, version_name};  // add set_own_key()
//const ServerVersion sv{0, 12, 0, version_name};  // rename mis-spelled undo_last_mitrust() into undo_last_mistrust()
//const ServerVersion sv{0, 12, 1, version_name};  // add assert_utf8() for every string to/from the Engine (except blobdata)
//const ServerVersion sv{0, 12, 2, version_name};  // fix the fork() problem on MacOS. daemonize() now got a function parameter.
//const ServerVersion sv(0,13,0);  // add package_version, rename "version" into "api_version" in ServerVersion, add versions from the Engine, too
//const ServerVersion sv(0,13,1);  // JSON-91: add MIME_encrypt_message_for_self() and encrypt_message_for_self()
//const ServerVersion sv(0,14,0);  // JSON-75: incompatible behavior of daemonize() especially in MS Windows
//const ServerVersion sv(0,15,0);  // JSON-92: API CHANGE.
//const ServerVersion sv(0,15,1);  // JSON-92 again: Change "keylist" in (MIME_)decrypt_message() from Out to InOutP. Is a compatible API change for JSON/JavaScript due to the handling of output parameters. :-)

//const ServerVersion sv(0,15,2);  // JSON-93 InLength<> is a compatible API change, because length parameter is still there but ignored. :-)
//static const ServerVersion sv(0,15,3);  // JSON-110: add encrypt_message_and_add_priv_key()
//static const ServerVersion sv(0,15,4);  // JSON-117: add trust_own_key()
//static const ServerVersion sv(0,15,5);  // JSON-119: add get_key_rating_for_user()
//static const ServerVersion sv(0,16,0);  // Kick-out Enigmail 2.0 compat, remove MIME_*() methods, deliverHandshakeResult() changes parameter types
//static const ServerVersion sv(0,16,1);  // JSON-120: add support for key_reset_identity(), key_reset_user(), and key_reset_all_own_keys()
//static const ServerVersion sv(0,17,0);  // kick out getGpgEnvironment(). It was Enigmail-only (JSON-18) and breaks architecture. Kick-out hotfixer un-feature.
//static const ServerVersion sv(0,18,0);  // JSON-127: 'src' in encrypt_message() is InOut.
//static const ServerVersion sv(0,18,1);  // JSON-130: some data members in pEp_identity added
//static const ServerVersion sv(0,18,2);  // JSON-135: Add mime_encode_message() and mime_decode_message() to the JSON API
//static const ServerVersion sv(0,18,3);  // JSON-137: Add outgoing_message_rating_preview() to the JSON API
static const ServerVersion sv(0,18,4);  // JSON-141: fix handling of parameters of type PEP_rating

	return sv;
}

ServerVersion::ServerVersion(unsigned maj, unsigned min, unsigned p)
: major{maj}
, minor{min}
, patch{p}
, name {VersionName}
, package_version{PackageVersion}
{
	Logger L("ServerVersion");
	if (!PackageVersion)
	{
		try{
			PackageVersion = "0.0.0";  /* break the loop */
			
			const std::string file_content =
				boost::algorithm::trim_copy(
					pEp::slurp("PackageVersion")
				);
			
			try{
				js::Value v;
				js::read_or_throw(file_content, v);
				const js::Object obj = v.get_obj();
				PackageVersion = pEp::utility::from_json_object<char*, js::str_type>(obj, "package_version");
			}
			catch(std::runtime_error& e)
			{
				L.warning(std::string("Cannot parse file \"PackageVersion\" as JSON object: ") + e.what() );
				PackageVersion = strdup(file_content.c_str());
			}
			
			this->package_version = PackageVersion;
		}
		catch(std::runtime_error& e)
		{
			// slurp() throws when it cannot read the file.
			L.info(std::string("Cannot read file \"PackageVersion\": ") + e.what() );
		}
	}
	L.debug("ServerVersion set as %u.%u.%u name=\"%s\" package_version=\"%s\".",
		major, minor, patch, name.c_str(), package_version
		);
}



std::string ServerVersion::major_minor_patch() const
{
	return std::to_string(major) + "."
	     + std::to_string(minor) + "."
	     + std::to_string(patch);
}


std::ostream& operator<<(std::ostream& o, const ServerVersion& sv)
{
	o << sv.major_minor_patch() << " \"" << sv.name << '\"';
	if(sv.package_version)
	{
		o << ". package_version=\"" << sv.package_version << "\" ";
	}
	return o;
}


template<>
js::Value to_json<ServerVersion>(const ServerVersion& sv)
{
	js::Object o;
	o.emplace_back("major", uint64_t(sv.major));
	o.emplace_back("minor", uint64_t(sv.minor));
	o.emplace_back("patch", uint64_t(sv.patch));
	o.emplace_back("api_version", sv.major_minor_patch());
	o.emplace_back("name", sv.name);
	o.emplace_back("package_version",
			(sv.package_version ? std::string(sv.package_version) : js::Value() )
		);
	o.emplace_back("engine_version", std::string(get_engine_version()));
	o.emplace_back("pep_protocol_version", std::string(PEP_VERSION));
	
	return o;
}

template<>
Out<ServerVersion>::~Out()
{
}


template<>
js::Value Type2String<ServerVersion>::get()  { return "ServerVersion"; }
