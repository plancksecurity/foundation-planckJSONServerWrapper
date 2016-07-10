#include "function_map.hh"
#include <stdexcept>

#include <cstring> // for memcpy()


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


#define SIMPLE_TYPE(TYPE)     \
	template<>                \
	In< TYPE >::~In()         \
	{ }                       \
	                          \
	template<>                \
	Out< TYPE >::~Out()       \
	{                         \
		delete value;         \
	}                         \


SIMPLE_TYPE( bool )
SIMPLE_TYPE( unsigned short )
SIMPLE_TYPE( unsigned )
SIMPLE_TYPE( unsigned long )
SIMPLE_TYPE( unsigned long long )

SIMPLE_TYPE( short )
SIMPLE_TYPE( int )
SIMPLE_TYPE( long )
SIMPLE_TYPE( long long )


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
int from_json<int>(const js::Value& v)
{
	return v.get_int();
}

template<>
bool from_json<bool>(const js::Value& v)
{
	return v.get_bool();
}


#define FROM_TO_JSON_UINT64( TYPE )          \
	template<>                               \
	TYPE from_json<TYPE>(const js::Value& v) \
	{                                        \
		return v.get_uint64();               \
	}                                        \
	                                         \
	template<>                               \
	js::Value to_json< TYPE >(const TYPE& t) \
	{                                        \
		return js::Value(uint64_t(t));       \
	}

FROM_TO_JSON_UINT64( unsigned )
FROM_TO_JSON_UINT64( unsigned long )
FROM_TO_JSON_UINT64( unsigned long long )


#define FROM_TO_JSON_INT64( TYPE )           \
	template<>                               \
	TYPE from_json<TYPE>(const js::Value& v) \
	{                                        \
		return v.get_int64();                \
	}                                        \
	                                         \
	template<>                               \
	js::Value to_json< TYPE >(const TYPE& t) \
	{                                        \
		return js::Value(int64_t(t));        \
	}

FROM_TO_JSON_UINT64( long )
FROM_TO_JSON_UINT64( long long )


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
js::Value to_json<struct tm*>(struct tm* const& t)
{
	if(t==nullptr)
	{
		return js::Value{};
	}
	
	char s[32] = "YYYY-MM-DDThh:mm:ss";
	snprintf(s,31, "%04u-%02u-%02uT%02u:%02u:%02u", t->tm_year+1900, t->tm_mon+1, t->tm_mday,  t->tm_hour, t->tm_min, t->tm_sec);
	return js::Value{s};
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
