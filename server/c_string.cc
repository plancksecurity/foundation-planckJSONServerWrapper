#include "c_string.hh"
#include <pEp/pEpEngine.h>

template<>
Out<c_string, true>::~Out()
{
	pEp_free(value);
}
