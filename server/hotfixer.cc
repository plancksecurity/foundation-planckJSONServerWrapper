#include "hotfixer.hh"
#include "logger.hh"

#include <boost/process.hpp>
#include <boost/filesystem.hpp>


#ifdef _WIN32
# ifndef  HOTFIX_SENTINEL_FILE
#  define HOTFIX_SENTINEL_FILE "json-hotfix-1.dat"
# endif
# define HOTFIX_BINARY "pep-hotfix.exe"
#else
# ifndef  HOTFIX_SENTINEL_FILE
#  define HOTFIX_SENTINEL_FILE "json-hotfix-1.dat"
# endif
# define HOTFIX_BINARY "pep-hotfix"
#endif

#define IS_ERROR_LOGGED(e) ( is_error_logged((e), "hotfixer.cc", __LINE__) )

namespace fs = boost::filesystem;
namespace bp = boost::process;
namespace sys = boost::system;


namespace pEp
{
    namespace utility
    {

        fs::path get_pep_dir()
        {
            const char *env_pephome = getenv("PEPHOME");
#ifndef _WIN32
            const char *env_usrhome = getenv("HOME");
            const char *env_pepsub = ".pEp";
#else
            const char *env_usrhome = getenv("APPDATA");
            const char *env_pepsub = "pEp";
#endif
            fs::path pephome;
            if (env_pephome)
                pephome = fs::path(env_pephome);
            else
            {
                if (!env_usrhome || !env_pepsub)
                    return fs::path();   // .empty() == true
                fs::path p1 = env_usrhome;
                fs::path p2 = env_pepsub;
                pephome = p1 / p2;
            }
            if(!fs::create_directory(pephome))
                if (!fs::exists(pephome))
                    return fs::path();
            return pephome;
        }

        int is_error_logged(sys::error_code& sec, const char* const src, const int line)
        {
            int ret = 0;
            if ((ret = sec.value()))
            {
                Logger l("hotfix");
                l.error("%s error (%d): %s (%s:%d)", sec.category().name(), ret, sec.message().c_str(), src, line);
                sec.clear();
                return ret;
            }
            return 0;
        }


        int is_error_logged(std::error_code& ec, const char* const src, const int line)
        {
            int ret = 0;
            if ((ret = ec.value()))
            {
                Logger l("hotfix");
                l.error("%s error (%d): %s (%s:%d)", ec.category().name(), ret, ec.message().c_str(), src, line);
                ec.clear();
                return ret;
            }
            return 0;
        }


        fs::path get_adapter_share_dir(sys::error_code& sec)
        {
            return fs::path(".");
        }


        fs::path get_adapter_bin_dir(sys::error_code& sec)
        {
            fs::path p = "../../bin";
            return p;
        }


        bool hotfix_call_required()
        {
            sys::error_code sec;
            int ret;
            Logger L("hotfix");

            fs::path pepdir = get_adapter_share_dir(sec);
            if ((ret = IS_ERROR_LOGGED(sec)))
                return ret;

            if (fs::exists(pepdir / HOTFIX_SENTINEL_FILE), sec)
                return false;

            L.info("hotfix required to run");
            return true;
        }


        int hotfix_call_execute()
        {
            std::error_code ec;
            sys::error_code sec;
            int ret;
            Logger L("hotfix");

            fs::path sent_path = get_adapter_share_dir(sec);
            if ((ret = IS_ERROR_LOGGED(ec)))
                return ret;
            sent_path /= HOTFIX_SENTINEL_FILE;

            fs::path hotfix_bin = get_adapter_bin_dir(sec);
            if ((ret = IS_ERROR_LOGGED(ec)))
                return ret;
            hotfix_bin /= HOTFIX_BINARY;

            if (!(fs::exists(hotfix_bin, sec)))
            {
                IS_ERROR_LOGGED(sec);
                L.debug("file: '%s'", hotfix_bin.c_str());
                L.info("error locating hotfix binary, ignoring.");
                return 0;
            }

            bp::ipstream is;  // reading spipe-stream
            std::string line;
            bp::child c(hotfix_bin, bp::std_out > is, ec);
            if ((ret = IS_ERROR_LOGGED(ec)))
                return ret;

            while (c.running(ec) && std::getline(is, line) && !line.empty())
                L.info(line);
            if ((ret = IS_ERROR_LOGGED(ec)))
            {
                c.wait(ec);
                return ret;
            }
            c.wait(ec);
            if ((ret = IS_ERROR_LOGGED(ec)))
                return ret;

            if ((ret = c.exit_code()))
            {
                L.error("hotfix returned exit code %d, exiting...", ret);
                return ret;
            }

            fs::ofstream sent_file(sent_path);
            sent_file << "# HOTFIX SENTINEL FILE, DO NOT REMOVE" << std::endl;
            sent_file.close();

            L.debug("sentinel file created: '%s'", sent_path.c_str());

            return 0;
        }

    }
}
