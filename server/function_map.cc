#include "function_map.hh"
#include <stdexcept>

#include <cstring> // for memcpy()

const Concat concat;

namespace
{
	char* make_c_string(const std::string& src)
	{
		char* dest = (char*)malloc(src.size() + 1);
		memcpy(dest, src.c_str(), src.size() + 1 );
		return dest;
	}
}


template<>
Out<char const*>::~Out()
{
	if(value) free(const_cast<char*>(*value));
	delete value;
}

template<>
Out<char const*>::Out(const Out<const char*>& other)
: value( new const char* )
{
	*value = *other.value ? strdup(*other.value) : nullptr;
}


template<>
int from_json<int>(const js::Value& v)
{
	return v.get_int();
}

template<>
unsigned from_json<unsigned>(const js::Value& v)
{
	return v.get_uint64();
}

template<>
std::string from_json<std::string>(const js::Value& v)
{
	return v.get_str();
}


template<>
char* from_json<char*>(const js::Value& v)
{
	return make_c_string( v.get_str() );
}


template<class T>
js::Value to_json(const T& t)
{
	return js::Value(t);
}

template js::Value to_json<bool>(const bool&);
template js::Value to_json<int>(const int&);
template js::Value to_json<std::string>(const std::string&);


template<>
js::Value to_json<unsigned>(const unsigned& t)
{
	return js::Value(uint64_t(t));
}

template<>
js::Value to_json<char*>(char* const & s)
{
	return s ? js::Value(std::string(s)) : js::Value{};
}

template<>
js::Value to_json<const char*>(const char* const & s)
{
	return s ? js::Value(std::string(s)) : js::Value{};
}


template<>
js::Value Type2String<std::string>::get()  { return "String"; }

template<>
js::Value Type2String<const char*>::get()  { return "String"; }
