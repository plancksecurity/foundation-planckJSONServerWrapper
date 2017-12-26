#include "inout.hh"
#include <stdexcept>
#include <pEp/pEpEngine.h>


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

SIMPLE_TYPE( std::string )


template<>
Out<char const*>::~Out()
{
	if(value)
	{
		pEp_free(const_cast<char*>(*value));
	}
	delete value;
}


template<>
Out<char*>::~Out()
{
	if(value)
	{
		pEp_free(*value);
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
		const auto u = v.get_uint64();         \
		if(u>std::numeric_limits<TYPE>::max()) \
		{                                      \
			throw std::runtime_error("Cannot convert " + std::to_string(u) + " into type " + typeid(TYPE).name() ); \
		}                                    \
		                                     \
		return static_cast<TYPE>(u);         \
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
		const auto u = v.get_int64();        \
		if( u > std::numeric_limits<TYPE>::max() || u < std::numeric_limits<TYPE>::min() ) \
		{                                    \
			throw std::runtime_error("Cannot convert " + std::to_string(u) + " into type " + typeid(TYPE).name() ); \
		}                                    \
		                                     \
		return static_cast<TYPE>(u);         \
	}                                        \
	                                         \
	template<>                               \
	js::Value to_json< TYPE >(const TYPE& t) \
	{                                        \
		return js::Value(int64_t(t));        \
	}

FROM_TO_JSON_INT64( long )
FROM_TO_JSON_INT64( long long )


template<>
std::string from_json<std::string>(const js::Value& v)
{
	return v.get_str();
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

