#ifndef JSON_GPG_ENVIRONMENT_HH
#define JSON_GPG_ENVIRONMENT_HH

#include <boost/optional.hpp>
#include <string>

struct GpgEnvironment
{
	boost::optional<std::string> gnupg_path;     // filled by pEpEngine's gnu_gpg_path()
	boost::optional<std::string> gnupg_home;     // filled by getenv("GNUPGHOME")
	boost::optional<std::string> gpg_agent_info; // filled by getenv("GPG_AGENT_INFO")
};

GpgEnvironment getGpgEnvironment();

#endif
