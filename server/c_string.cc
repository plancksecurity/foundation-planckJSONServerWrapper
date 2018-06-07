#include "c_string.hh"
#include <pEp/pEpEngine.h>

template<>
Out<c_string, ParamFlag::Default>::~Out()
{
	pEp_free(value);
}

template<>
Out<c_string, ParamFlag::DontOwn>::~Out()
{
	// don't pEp_free() the value!
}

template<>
In<size_t, ParamFlag::NoInput>::~In()
{}
