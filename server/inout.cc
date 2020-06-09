#include "inout.hh"
#include "nfc.hh"
#include <stdexcept>
#include <pEp/pEpEngine.h>
#include <pEp/platform.h> // for timegm() replacement on Windows


#define SIMPLE_TYPE(TYPE)     \
	template<>                \
	In< TYPE >::~In()         \
	{ }                       \
	                          \
	template<>                \
	Out< TYPE >::~Out()       \
	{ }                       \


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
SIMPLE_TYPE( js::Array )


template<>
Out<char const*>::~Out()
{
	pEp_free(const_cast<char*>(value));

}


template<>
Out<char*>::~Out()
{
	pEp_free(value);
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
	const std::string& s = v.get_str();
	assert_utf8(s); // whould throw an exception if s is not valid UTF-8
	return s;
}



template<class T>
js::Value to_json(const T& t)
{
	return js::Value(t);
}

template js::Value to_json<bool>(const bool&);
template js::Value to_json<int>(const int&);


template<>
js::Value to_json<std::string>(const std::string& s)
{
	assert_utf8(s);
	return js::Value(s);
}


template<>
js::Value to_json<const char*>(const char* const & s)
{
	if(s)
	{
		const std::string ss{s};
		assert_utf8(s);
		return js::Value(ss);
	}else{
		return js::Value{};
	}
}


template<>
js::Value to_json<char*>(char* const & s)
{
	return to_json<const char*>( const_cast<const char*>(s) );
}


template<>
js::Value to_json<js::Array>(const js::Array& a)
{
    return a;
}


template<>
js::Value to_json<timestamp *>(timestamp * const& t)
{
	if(t==nullptr)
	{
		return js::Value{};
	}
	
	// neither timegm() nor mktime() respect t->tm_gmtoff for their conversions. What a mess!
	// But t->tm_gmtoff is non-standard, though, and doesn't exist on MS Windows. In ENGINE-735
	// we switched to `timestamp` to cater for Windows and now convert to UTC in Windows' `timegm`.
	const int64_t u = timegm(t);
	return js::Value{u};
}

