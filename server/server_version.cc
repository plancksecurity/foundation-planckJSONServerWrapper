#include "server_version.hh"
#include "inout.hh"

namespace {

// version names comes from here:
// https://de.wikipedia.org/wiki/Bundesautobahn_4
static const std::string version_name =
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
	"(34) Erndtebrück";      // remove apiVersion(), change version() to return a semver-compatible version number in a JSON object.


const ServerVersion sv{0, 10, 0, version_name};  // first version defined.

} // end of anonymous namespace
////////////////////////////////////////////////////////////////////////////

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
	return o << sv.major_minor_patch() << " \"" << sv.name << '\"';
}


template<>
js::Value to_json<ServerVersion>(const ServerVersion& sv)
{
	js::Object o;
	o.emplace_back("major", uint64_t(sv.major));
	o.emplace_back("minor", uint64_t(sv.minor));
	o.emplace_back("patch", uint64_t(sv.patch));
	o.emplace_back("version", sv.major_minor_patch());
	o.emplace_back("name", sv.name);
	
	return o;
}

template<>
js::Value Type2String<ServerVersion>::get()  { return "ServerVersion"; }
