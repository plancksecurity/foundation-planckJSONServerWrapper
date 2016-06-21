#include "function_map.hh"
#include <stdexcept>

#include <cstring> // for memcpy()

const Concat concat{};

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
In<char const*>::~In()
{
	if(value) free(const_cast<char*>(value));
}

template<>
In<int>::~In()
{ }

template<>
In<time_t>::~In()
{ }

template<>
In<std::size_t>::~In()
{ }


#define SIMPLE_TYPE_OUT(TYPE) \
	template<>                \
	Out< TYPE >::~Out()       \
	{                         \
		delete value;         \
	}                         \
	                          \
	template<>                                \
	Out< TYPE >::Out(const Out<TYPE>& other)  \
	: value ( new TYPE {*other.value} )       \
	{ }

SIMPLE_TYPE_OUT( bool )
SIMPLE_TYPE_OUT( unsigned )
SIMPLE_TYPE_OUT( unsigned long)
SIMPLE_TYPE_OUT( unsigned long long)


template<>
Out<char const*>::~Out()
{
	if(value)
	{
		free(const_cast<char*>(*value));
	}
	delete value;
}


template<>
Out<char*>::~Out()
{
	if(value)
	{
		free(*value);
		*value = nullptr;
	}
	delete value;
}



template<>
Out<char const*>::Out(const Out<const char*>& other)
: value( new const char* )
{
	*value = *other.value ? strdup(*other.value) : nullptr;
}

template<>
Out<char*>::Out(const Out<char*>& other)
: value( new char* )
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
time_t from_json<time_t>(const js::Value& v)
{
	return static_cast<time_t>(v.get_int64());
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

template<>
const char* from_json<const char*>(const js::Value& v)
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
js::Value to_json<std::size_t>(const std::size_t& s)
{
	return js::Value{uint64_t(s)};
}

template<>
std::size_t from_json<std::size_t>(const js::Value& v)
{
	return v.get_uint64();
}

template<>
bool from_json<bool>(const js::Value& v)
{
	return v.get_bool();
}


template<>
js::Value Type2String<std::string>::get()  { return "String"; }

template<>
js::Value Type2String<const char*>::get()  { return "String"; }

template<>
js::Value Type2String<char*>::get()  { return "String"; }


template<>
js::Value Type2String<int>::get()  { return "Integer"; }

template<>
js::Value Type2String<long>::get()  { return "Integer"; }

template<>
js::Value Type2String<long long>::get()  { return "Integer"; }


template<>
js::Value Type2String<unsigned>::get()  { return "Integer"; }

template<>
js::Value Type2String<unsigned long>::get()  { return "Integer"; }

template<>
js::Value Type2String<unsigned long long>::get()  { return "Integer"; }

template<>
js::Value Type2String<bool>::get()  { return "Bool"; }
