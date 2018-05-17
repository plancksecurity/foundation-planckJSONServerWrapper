// simple string wrapper that can be used in my template magic to call
// a C function expecting a "const char*" directly.

#ifndef PEP_JSON_ADAPTER_C_STRING_HH
#define PEP_JSON_ADAPTER_C_STRING_HH

#include <string>
#include "inout.hh"

// just an empty tag type
struct c_string
{ };


template<ParamFlag PF>
struct In<c_string, PF>
{
	typedef In<c_string, PF> Self;
	
	typedef const char* c_type;
	enum { is_output = false, need_input = !(PF & ParamFlag::NoInput) };
	
	~In() = default;
	
	In(const Self& other) = delete;
	In(Self&& victim) = delete;
	Self& operator=(const Self&) = delete;
	
	In(const js::Value& v, Context*)
	: value( from_json<std::string>(v) )
	{ }
	
	js::Value to_json() const
	{
		return ::to_json<std::string>(value);
	}
	
	c_type get_value() const { return value.c_str(); }
	
	const std::string value;
};


template<ParamFlag PF>
struct Out<c_string, PF>
{
	typedef Out<c_string, PF> Self;
	
	typedef char** c_type;
	enum { is_output = true, need_input = !(PF & ParamFlag::NoInput) };
	
	Out(const js::Value&, Context*) // ignore dummy value, ignore context
	{ }
	
	~Out();
	
	Out(const Self& other) = delete;
	Out(Self&& victim) = delete;
	Self& operator=(const Self&) = delete;
	
	js::Value to_json() const
	{
		return value ? ::to_json<std::string>(value) : js::Value();
	}
	
	c_type get_value() const { return &value; }
	
	mutable char* value = nullptr;
};


// forward declare specializations (to avoid selecting of the default implementation),
// but don't implement them, because not needed, yet.
template<ParamFlag PF>
struct InOut<c_string, PF>;

template<ParamFlag PF>
struct InOutP<c_string, PF>;


#endif // PEP_JSON_ADAPTER_C_STRING_HH
