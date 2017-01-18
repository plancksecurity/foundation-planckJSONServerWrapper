#include "gpg_environment.hh"
#include "function_map.hh"

#include <cstdlib>
#include <pEp/message_api.h> // for get_binary_path()

GpgEnvironment getGpgEnvironment()
{
	GpgEnvironment ge{};

	const char* gpg_path = nullptr;
	const auto status = get_binary_path( PEP_crypt_OpenPGP, &gpg_path);
	if(status == PEP_STATUS_OK && gpg_path)
	{
		ge.gnupg_path = std::string(gpg_path);
	}
	
	const char* home = std::getenv("GNUPGHOME");
	if(home)
	{
		ge.gnupg_home = std::string(home);
	}

	const char* ai = std::getenv("GPG_AGENT_INFO");
	if(home)
	{
		ge.gpg_agent_info = std::string(ai);
	}
	
	return ge;
}


template<>
js::Value to_json<GpgEnvironment>(const GpgEnvironment& ge)
{
	js::Object obj;
	obj.emplace_back("gnupg_path", (ge.gnupg_path ? ge.gnupg_path.get() : js::Value{}) );
	obj.emplace_back("gnupg_home", (ge.gnupg_home ? ge.gnupg_home.get() : js::Value{}) );
	obj.emplace_back("gpg_agent_info", (ge.gpg_agent_info ? ge.gpg_agent_info.get() : js::Value{}) );
	return obj;
}

template<>
js::Value Type2String<GpgEnvironment>::get()  { return "GpgEnvironment"; }
