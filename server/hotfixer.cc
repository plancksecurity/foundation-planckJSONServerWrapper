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


namespace fs = boost::filesystem;
namespace bp = boost::process;


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
            return pephome;
        }


        // fs::path get_adapter_share_dir()
        // {
        //  return fs::path(".");
        // }


        fs::path get_adapter_bin_dir()
        {
            return fs::path("../../bin");
        }


        int is_error_logged(std::error_code ec)
        {
            int ret = 0;
            if ((ret = ec.value())) {
                Logger l("hotfix");
                l.info("%s error (%d): %s", ec.category().name(), ret, ec.message().c_str());
                ec.clear();
                return ret;
            }
            return 0;
        }


        bool hotfix_call_required()
        {
            Logger L("hotfix");
            fs::path pepdir = get_pep_dir();
            if (fs::exists(pepdir / HOTFIX_SENTINEL_FILE))
                return false;
            L.info("hotfix required to run");
            return true;
        }


        int hotfix_call_execute()
        {
            int ret;
            Logger L("hotfix");
            fs::path sent_path = get_pep_dir() / HOTFIX_SENTINEL_FILE;
            fs::path hotfix_bin = get_adapter_bin_dir() / HOTFIX_BINARY;
            std::error_code ec;

            bp::ipstream is;  // reading spipe-stream
            std::string line;
            bp::child c(hotfix_bin, "argv1", "argv2", bp::std_out > is, ec);
            if ((ret = is_error_logged(ec)))
                return ret;

            while (c.running(ec) && std::getline(is, line) && !line.empty())
                L.info(line);
            if ((ret = is_error_logged(ec))) {
                c.wait(ec);
                return ret;
            }
            c.wait(ec);
            if ((ret = is_error_logged(ec)))
                return ret;

            if ((ret = c.exit_code())) {
                L.info("hotfix returned exit code %d, exiting...", ret);
                return ret;
            }

            fs::ofstream sent_file(sent_path);
            sent_file << "# HOTFIX SENTINEL FILE, DO NOT REMOVE" << std::endl;
            sent_file.close();

            L.info("sentinel file created: %s", sent_path.c_str());

            return 0;
        }

    }
}
