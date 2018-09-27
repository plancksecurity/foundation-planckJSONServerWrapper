#ifndef HOT_FIXER_HH
#define HOT_FIXER_HH

#include <boost/process.hpp>

using namespace boost::process;

namespace pEp
{
    namespace utility
    {

bool hotfix_call_required();

int hotfix_call_execute();

    }
}

#endif
