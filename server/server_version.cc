#include "server_version.hh"
#include "inout.hh"
#include <fstream>
#include <sstream>

#include <pEp/pEpEngine.h> // for PEP_VERSION and get_engine_version()

namespace {

#ifdef PACKAGE_VERSION
	char* PackageVersion = PACKAGE_VERSION;
#else
	char* PackageVersion = nullptr;
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
	"(39) Gemünden";         // JSON-93: support for InLengt<> to calculate string lengths automatically.

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
const ServerVersion sv(0,15,2);  // JSON-93 InLength<> is a compatible API change, because length parameter is still there but ignored. :-)

} // end of anonymous namespace
////////////////////////////////////////////////////////////////////////////

ServerVersion::ServerVersion(unsigned maj, unsigned min, unsigned p)
: major{maj}
, minor{min}
, patch{p}
, name {VersionName}
, package_version{PackageVersion}
{
	if (!PackageVersion) {
		std::ifstream packver("PackageVersion");
		if (packver.is_open() && packver.good()) {
			std::stringstream sstr;
			sstr << packver.rdbuf();
			std::string contents = sstr.str();
			contents.erase(std::remove(contents.begin(), contents.end(), '\n'), contents.end());
			contents.erase(std::remove(contents.begin(), contents.end(), '\r'), contents.end());
			PackageVersion = new char[contents.length() + 1];
			strcpy(PackageVersion, contents.c_str());
			this->package_version = PackageVersion;
		}
	}
}

const ServerVersion& server_version()
{
	return sv;
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
