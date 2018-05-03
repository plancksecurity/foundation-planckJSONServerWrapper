#include "c_string.hh"
#include <pEp/pEpEngine.h>

Out<c_string, true>::~Out()
{
	pEp_free(value);
}
