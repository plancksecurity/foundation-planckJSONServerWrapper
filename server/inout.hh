#ifndef PEP_JSON_SERVER_INOUT_HH
#define PEP_JSON_SERVER_INOUT_HH

#include "json_spirit/json_spirit_value.h"

#include "context.hh"
#include <type_traits>
#include <stdexcept>

// Just for debugging:
#include <iostream>


namespace js = json_spirit;

template<class T, bool NeedInput> struct In;
template<class T, bool NeedInput> struct Out;

// "params" and "position" might be used to fetch additional parameters from the array.
template<class T>
T from_json(const js::Value& v);


template<class T>
js::Value to_json(const T& t);


// helper classes to specify in- and out-parameters
template<class T, bool NeedInput=true>
struct In
{
	typedef T c_type; // the according type in C function parameter
	enum { is_output = false, need_input = NeedInput };
	
	explicit In(const T& t) : value(t) {}
	~In();
	
	In(const In<T,NeedInput>& other) = delete;
	In(In<T,NeedInput>&& victim) = delete;
	In<T,NeedInput>& operator=(const In<T,NeedInput>&) = delete;
	
	// default implementation:
	In(const js::Value& v, Context*)
	: In( from_json<T>(v) )
	{ }
	
	js::Value to_json() const
	{
		throw "Something went wrong here!";
	}
	
	c_type get_value() const { return value; }
	
	T value;
};


// to call functions that operate directly on the JSON data type
template<class T, bool NeedInput=true>
struct InRaw
{
	typedef js::Value c_type; // do not unwrap JSON data type
	enum { is_output = false, need_input = NeedInput };
	
	explicit InRaw(const js::Value& t) : value(t) {}
	~InRaw() = default;
	
	InRaw(const InRaw<T,NeedInput>& other) = delete;
	InRaw(InRaw<T,NeedInput>&& victim) = delete;
	InRaw<T,NeedInput>& operator=(const InRaw<T,NeedInput>&) = delete;
	
	// default implementation:
	InRaw(const js::Value& v, Context*)
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
template<class T, bool NeedInput=true>
struct InOut : public In<T,NeedInput>
{
	typedef In<T,NeedInput> Base;
	enum { is_output = true, need_input = NeedInput };

	explicit InOut(const T& t) : Base(t) {}
	~InOut() = default;
	
	InOut<T,NeedInput>& operator=(const InOut<T,NeedInput>&) = delete;
	
	// default implementation:
	InOut(const js::Value& v, Context*)
	: Base( from_json<T>(v) )
	{ }
	
	js::Value to_json() const
	{
		return ::to_json<T>(Base::value);
	}
};


template<class T, bool NeedInput = true>
struct Out
{
	typedef T* c_type; // the according type in C function parameter
	enum { is_output = true, need_input = NeedInput }; // if need_input=false it would no longer consume an element in the input parameter array.
	
	explicit Out() : value{ new T{} }
	{ }

	explicit Out(const T& t) : value{ new T{t} }
	{ }

	~Out();
	
	Out(const Out<T,NeedInput>& other) = delete;
	Out(Out<T,NeedInput>&& victim) = delete;
	
	// just to be sure they are not implicitly defined:
	Out<T,NeedInput>& operator=(const Out<T,NeedInput>& other) = delete;
	Out<T,NeedInput>& operator=(Out<T,NeedInput>&& victim) = delete;
	
	Out(const js::Value& v, Context*)
	: Out()
	{ }
	
	js::Value to_json() const
	{
		return ::to_json<T>(*value);
	}
	
	c_type get_value() const { return value; }
	
	T* value = nullptr;
	
	friend
	std::ostream& operator<<(std::ostream& o, const Out<T,NeedInput>& out)
	{
		o << (const void*)&out;
		
// the if() was added to avoid crashes on memory corruptuon. But clang++ warns, that this check is always true on "well-formed" programs, and he is right. In an ideal world there are no memory corruptions. ;-(
//		if(&out)
		{
			o << ", value=" << (const void*)out.value;
			if(out.value)
			{
				o << ", *value=" << *(out.value);
			}
		}
		
		return o;
	}
	
};


// helper classes to specify in- and out-parameters whose output might change by the called function.
// Use InOut<T> for in/out parameters where the function only makes in-place changes.
template<class T, bool NeedInput=true>
struct InOutP : public Out<T,NeedInput>
{
	typedef Out<T,NeedInput> Base;
	enum { is_output = true, need_input = NeedInput };

	explicit InOutP(const T& t) : Base(t) {}
	
	InOutP<T,NeedInput>& operator=(const InOutP<T,NeedInput>&) = delete;
	
	// default implementation:
	InOutP(const js::Value& v, Context*)
	: Base( from_json<T>(v) )
	{ }
};


template<class T, bool NeedInput>
js::Value to_json(const Out<T,NeedInput>& o)
{
	return ::to_json(*o.value);
}

template<class T, bool NeedInput>
js::Value to_json(const InOut<T,NeedInput>& o)
{
	return ::to_json(o.value);
}


// Type2String

template<class T>
struct Type2String
{
	static js::Value get();
};


template<class T>
struct Type2String<In<T, true>>
{
	static js::Value get() { js::Object ret; ret.emplace_back("direction", "In"); ret.emplace_back("type", Type2String<T>::get() ); return ret; }
};

template<class T>
struct Type2String<In<T, false>>
{
	static js::Value get() { throw "MSVC is b0rken"; }
};



template<class T>
struct Type2String<InRaw<T, true>>
{
	static js::Value get() { js::Object ret; ret.emplace_back("direction", "In"); ret.emplace_back("type", Type2String<T>::get() ); return ret; }
};

template<class T>
struct Type2String<Out<T, true>>
{
	static js::Value get() { js::Object ret; ret.emplace_back("direction", "Out"); ret.emplace_back("type", Type2String<T>::get() ); return ret; }
};

template<class T>
struct Type2String<InOut<T, true>>
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
