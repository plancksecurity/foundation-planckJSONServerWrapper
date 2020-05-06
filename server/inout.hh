#ifndef PEP_JSON_SERVER_INOUT_HH
#define PEP_JSON_SERVER_INOUT_HH

#include "json_spirit/json_spirit_value.h"

#include "context.hh"
#include <type_traits>
#include <stdexcept>

// Just for debugging:
#include <iostream>

// is a bitfield that controls the In<> / Out<> parameter types
enum class ParamFlag : unsigned
{
	Default = 0,
	NoInput = 1,        // has no input parameter in the JSON API. Value comes from Context.
	DontOwn = 2,        // parameter holds a "shared resource". Don't free() it in destructor.
	NullOkay = 4,       // JSON 'null' value accepted, will become a C NULL pointer.
};

inline constexpr
ParamFlag operator|(ParamFlag a, ParamFlag b)
{
	return ParamFlag( unsigned(a) | unsigned(b) );
}

inline constexpr
ParamFlag operator&(ParamFlag a, ParamFlag b)
{
	return ParamFlag( unsigned(a) & unsigned(b) );
}

inline constexpr
bool operator!(ParamFlag pf)
{
	return !unsigned(pf);
}


namespace js = json_spirit;

template<class T, ParamFlag PF> struct In;
template<class T, ParamFlag PF> struct Out;

// "params" and "position" might be used to fetch additional parameters from the array.
template<class T>
T from_json(const js::Value& v);


template<class T>
js::Value to_json(const T& t);


// helper classes to specify in- and out-parameters
template<class T, ParamFlag PF=ParamFlag::Default>
struct In
{
	typedef T c_type; // the according type in C function parameter
	enum { is_output = false, need_input = !(PF & ParamFlag::NoInput) };
	
	explicit In(const T& t) : value(t) {}
	~In();
	
	In(const In<T,PF>& other) = delete;
	In(In<T,PF>&& victim) = delete;
	In<T,PF>& operator=(const In<T,PF>&) = delete;
	
	// default implementation:
	In(const js::Value& v, Context*, unsigned param_nr)
	: In( from_json<T>(v) )
	{ }
	
	js::Value to_json() const
	{
		throw std::logic_error( std::string("Never call to_json() for a In<") + typeid(T).name() + "> data type!  fn=" + __PRETTY_FUNCTION__ );
	}
	
	c_type get_value() const { return value; }
	
	T value;
};


// to call functions that operate directly on the JSON data type
template<class T, ParamFlag PF=ParamFlag::Default>
struct InRaw
{
	typedef js::Value c_type; // do not unwrap JSON data type
	enum { is_output = false, need_input = !(PF & ParamFlag::NoInput) };
	
	explicit InRaw(const js::Value& t) : value(t) {}
	~InRaw() = default;
	
	InRaw(const InRaw<T,PF>& other) = delete;
	InRaw(InRaw<T,PF>&& victim) = delete;
	InRaw<T,PF>& operator=(const InRaw<T,PF>&) = delete;
	
	// default implementation:
	InRaw(const js::Value& v, Context*, unsigned param_nr)
	: InRaw(v)
	{ }

	js::Value to_json() const
	{
		throw std::logic_error( std::string(typeid(T).name()) + " is not for output!" );
	}
	
	c_type get_value() const { return value; }
	
	js::Value value;
};


// helper classes to specify in- and out-parameters whose output is in-place.
// Use InOutP<T> for in/out parameters where the function might change the object and expects a pointer
template<class T, ParamFlag PF=ParamFlag::Default>
struct InOut : public In<T,PF>
{
	typedef In<T,PF> Base;
	enum { is_output = true, need_input = !(PF & ParamFlag::NoInput) };

	explicit InOut(const T& t) : Base(t) {}
	~InOut() = default;
	
	InOut<T,PF>& operator=(const InOut<T,PF>&) = delete;
	
	// default implementation:
	InOut(const js::Value& v, Context*, unsigned param_nr)
	: Base( from_json<T>(v) )
	{ }
	
	js::Value to_json() const
	{
		return ::to_json<T>(Base::value);
	}
};


template<class T, ParamFlag PF = ParamFlag::Default>
struct Out
{
	typedef T* c_type; // the according type in C function parameter
	enum { is_output = true, need_input = !(PF & ParamFlag::NoInput) }; // if need_input=false it would no longer consume an element in the input parameter array.
	
	explicit Out() : value{}
	{ }

	explicit Out(const T& t) : value{t}
	{ }

	~Out();
	
	Out(const Out<T,PF>& other) = delete;
	Out(Out<T,PF>&& victim) = delete;
	
	// just to be sure they are not implicitly defined:
	Out<T,PF>& operator=(const Out<T,PF>& other) = delete;
	Out<T,PF>& operator=(Out<T,PF>&& victim) = delete;
	
	// dummy Value v is ignored for output parameters
	Out(const js::Value& v, Context*, unsigned param_nr)
	: Out()
	{ }
	
	js::Value to_json() const
	{
		return ::to_json<T>(value);
	}
	
	c_type get_value() const { return &value; }
	
	mutable T value;
	
	/*
	friend
	std::ostream& operator<<(std::ostream& o, const Out<T,PF>& out)
	{
		o << (const void*)out;
		
		o << ", value=" << (const void*)out.value;
		return o;
	}
	*/
};


// helper classes to specify in- and out-parameters whose output might change by the called function.
// Use InOut<T> for in/out parameters where the function only makes in-place changes.
template<class T, ParamFlag PF=ParamFlag::Default>
struct InOutP : public Out<T,PF>
{
	typedef Out<T,PF> Base;
	enum { is_output = true, need_input = !(PF & ParamFlag::NoInput) };

	explicit InOutP(const T& t) : Base(t) {}
	
	InOutP<T,PF>& operator=(const InOutP<T,PF>&) = delete;
	
	// default implementation:
	InOutP(const js::Value& v, Context*, unsigned param_nr)
	: Base( from_json<T>(v) )
	{ }
};


template<class T, ParamFlag PF>
js::Value to_json(const Out<T,PF>& o)
{
	return ::to_json(o.value);
}

template<class T, ParamFlag PF>
js::Value to_json(const InOut<T,PF>& o)
{
	return ::to_json(o.value);
}


// Type2String

template<class T>
struct Type2String
{
	static js::Value get();
};


template<class T, ParamFlag PF>
struct Type2String<In<T, PF>>
{
	static js::Value get() { js::Object ret; ret.emplace_back("direction", "In"); ret.emplace_back("type", Type2String<T>::get() ); return ret; }
};


template<class T>
struct Type2String<In<T, ParamFlag::NoInput>>
{
    static js::Value get() { throw std::logic_error("Stupid MSVC compiler requiring this!"); }
};


template<class T>
struct Type2String<InRaw<T, ParamFlag::Default>>
{
	static js::Value get() { js::Object ret; ret.emplace_back("direction", "In"); ret.emplace_back("type", Type2String<T>::get() ); return ret; }
};

template<class T, ParamFlag PF>
struct Type2String<Out<T, PF>>
{
	static js::Value get() { js::Object ret; ret.emplace_back("direction", "Out"); ret.emplace_back("type", Type2String<T>::get() ); return ret; }
};

template<class T, ParamFlag PF>
struct Type2String<InOut<T, PF>>
{
	static js::Value get() { js::Object ret; ret.emplace_back("direction", "InOut"); ret.emplace_back("type", Type2String<T>::get() ); return ret; }
};

template<class T, ParamFlag PF>
struct Type2String<InOutP<T, PF>>
{
	static js::Value get() { js::Object ret; ret.emplace_back("direction", "InOut"); ret.emplace_back("type", Type2String<T>::get() ); return ret; }
};


template<class... Args> struct Type2Json;

template<class T, class... Args>
struct Type2Json<T, Args...>
{
	static js::Array& get(js::Array& a)
	{
		if(T::need_input)
		{
			a.push_back( Type2String<T>::get() );
		}
		Type2Json<Args...>::get(a);
		return a;
	}
};


template<> struct Type2Json<>
{
	static js::Array& get(js::Array& a) { return a; }
};


#endif // PEP_JSON_SERVER_INOUT_HH
