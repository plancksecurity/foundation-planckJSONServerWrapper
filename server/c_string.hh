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
	
	In(const js::Value& v, Context* ctx, unsigned param_nr)
	: value( from_json<std::string>(v) )
	{
		ctx->store(param_nr, value.length());
	}
	
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
	
	Out(const js::Value&, Context*, unsigned) // ignore dummy value, ignore context, ignore param_nr
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


// Holds the length of the string in the previous c_string parameter
template<ParamFlag PF = ParamFlag::Default>
struct InLength : In<size_t, PF>
{
	typedef In<size_t, PF> Base;
	
	InLength(const js::Value& v, Context* ctx, unsigned param_nr)
	: Base( ctx->retrieve(param_nr-1) )
	{}
};


template<ParamFlag PF>
struct Type2String<InLength<PF>>
{
	static js::Value get() { js::Object ret; ret.emplace_back("direction", "In"); ret.emplace_back("type", Type2String<size_t>::get() ); return ret; }
};

#endif // PEP_JSON_ADAPTER_C_STRING_HH
