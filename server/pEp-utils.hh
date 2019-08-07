#pragma once

#include <memory>

namespace pEp
{
	namespace utility
	{
		template<class T>
		std::unique_ptr<T, void(*)(T*)> make_c_ptr(T* data, void(*Deleter)(T*))
		{
			return { data, Deleter };
		}

	} // end of namespace pEp::util
} // end of namespace pEp

