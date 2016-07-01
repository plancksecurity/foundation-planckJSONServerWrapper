#ifndef FUNCTION_MAP_HH
#define FUNCTION_MAP_HH

#include "json_spirit/json_spirit_value.h"
#include <type_traits>

// Just for debugging:
#include <iostream>
#include <pEp/message_api.h>


namespace js = json_spirit;

template<class T> struct In;
template<class T> struct Out;

// "params" and "position" might be used to fetch additional parameters from the array.
template<class T>
T from_json(const js::Value& v);


template<class T>
js::Value to_json(const T& t);



// helper classes to specify in- and out-parameters
template<class T>
struct In
{
	typedef T c_type; // the according type in C function parameter
	enum { is_output = false };
	
	explicit In(const T& t) : value(t) {}
	~In();
	
	In(const In<T>& other) = delete;
	In(In<T>&& victim);
	
	In<T>& operator=(const In<T>&) = delete;
	
	// default implementation:
	In(const js::Value& v, const js::Array& params, unsigned position)
	: In( from_json<T>(v) )
	{ }
	
	T value;
};


// to call functions that operate directly on the JSON data type
template<class T>
struct InRaw
{
	typedef js::Value c_type; // do not unwrap JSON data type
	enum { is_output = false };
	
	explicit InRaw(const js::Value& t) : value(t) {}
	~InRaw() = default;
	
	InRaw(const InRaw<T>& other) = delete;
	InRaw(InRaw<T>&& victim);
	InRaw<T>& operator=(const InRaw<T>&) = delete;
	
	// default implementation:
	InRaw(const js::Value& v, const js::Array& params, unsigned position)
	: InRaw(v)
	{ }
	
	js::Value value;
};


// helper classes to specify in- and out-parameters
template<class T>
struct InOut : public In<T>
{
	typedef In<T> Base;
	enum { is_output = true };

	explicit InOut(const T& t) : Base(t) {}
	~InOut() = default;
	
	InOut<T>& operator=(const InOut<T>&) = delete;
	
	// default implementation:
	InOut(const js::Value& v, const js::Array& params, unsigned position)
	: Base( from_json<T>(v) )
	{ }
	
	js::Value to_json() const
	{
		return ::to_json<T>(Base::value);
	}
};


template<class T>
struct Out
{
	typedef T* c_type; // the according type in C function parameter
	enum { is_output = true };
	
	explicit Out() : value{ new T{} }
	{
		if(typeid(T)==typeid(_message*))
		{
			std::cerr << "|$ Out<message*>(): this=" << *this << "\n";
		}
	}
	
	~Out();
	
	Out(const Out<T>& other) = delete;
	Out(Out<T>&& victim);
	
	// just to be sure they are not implicitly defined:
	Out<T>& operator=(const Out<T>& other) = delete;
	Out<T>& operator=(Out<T>&& victim) = delete;
	
	Out(const js::Value& v, const js::Array& params, unsigned position)
	: Out()
	{ }
	
	js::Value to_json() const
	{
		return ::to_json<T>(*value);
	}
	
	T* value = nullptr;
	
	friend
	std::ostream& operator<<(std::ostream& o, const Out<T>& out)
	{
		o << (const void*)&out;
		
		if(&out)
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


template<class T>
js::Value to_json(const Out<T>& o)
{
	return ::to_json(*o.value);
}

template<class T>
js::Value to_json(const InOut<T>& o)
{
	return ::to_json(o.value);
}


// heloer class for generic calls:
// R : return type of the called function
// U : number of the parameter which is being extracted
// MAX: maximum number of parameters. if U==MAX the function call is executed
// Args... the function's parameter types
template<class R, unsigned U, unsigned MAX, class... Args>
class helper;


// specialization for U==MAX: do the function call here
template<class R, unsigned U, class... Args>
class helper<R, U, U, Args...>
{
public:
	static js::Value call( const std::function<R(typename Args::c_type...)>& fn, js::Array& out_parameters, const js::Array& parameters, const Args&... args)
	{
		return to_json( fn(args.value...) );
	}
};


// specialization for Return type == void
template<unsigned U, class... Args>
class helper<void, U, U, Args...>
{
public:
	static js::Value call( const std::function<void(typename Args::c_type...)>& fn, js::Array& out_parameters, const js::Array& parameters, const Args&... args)
	{
		fn(args.value...);
		return js::Value{};
	}
};


// recursive helper class:
// It is used with U==0 in Func<>::call() and calls itself recursively until U==MAX, where the real function calls occurs,
// and the output parameters are collected during unwinding of the recursion
template<class R, unsigned U, unsigned MAX, class... Args>
class helper
{
public:
	typedef std::tuple<Args...> Tuple;
	typedef typename std::tuple_element<U, Tuple>::type Element; // The type of the U'th parameter
	typedef helper<R, U+1, MAX, Args...> NextHelper;
	
public:

	// A2... a2 are the alredy pealed-off paremeters
	template<class... A2>
	static js::Value call( const std::function<R(typename Args::c_type...)>& fn, js::Array& out_parameters, const js::Array& parameters, const A2&... a2)
	{
		// extract the U'th element of the parameter list
		const Element element(parameters[U], parameters, U);
		
		const js::Value ret = NextHelper::call(fn, out_parameters, parameters, a2..., element );
		if(Element::is_output)
		{
			out_parameters.push_back( to_json(element) );
		}
		return ret;
	}
};


template<class T>
struct Type2String
{
	static js::Value get();
};

template<class T>
struct Type2String<In<T>>
{
	static js::Value get() { js::Object ret; ret.emplace_back("direction", "In"); ret.emplace_back("type", Type2String<T>::get() ); return ret; }
};

template<class T>
struct Type2String<InRaw<T>>
{
	static js::Value get() { js::Object ret; ret.emplace_back("direction", "In"); ret.emplace_back("type", Type2String<T>::get() ); return ret; }
};

template<class T>
struct Type2String<Out<T>>
{
	static js::Value get() { js::Object ret; ret.emplace_back("direction", "Out"); ret.emplace_back("type", Type2String<T>::get() ); return ret; }
};

template<class T>
struct Type2String<InOut<T>>
{
	static js::Value get() { js::Object ret; ret.emplace_back("direction", "InOut"); ret.emplace_back("type", Type2String<T>::get() ); return ret; }
};

template<class... Args> struct Type2Json;

template<class T, class... Args>
struct Type2Json<T, Args...>
{
	static js::Array& get(js::Array& a)
	{
		a.push_back( Type2String<T>::get() );
		Type2Json<Args...>::get(a);
		return a;
	}
};


template<> struct Type2Json<>
{
	static js::Array& get(js::Array& a) { return a; }
};



// abstract base class for all Func<...> types below
class FuncBase
{
public:
	virtual ~FuncBase() = default;
	virtual bool  isSeparator() const = 0;
	virtual void  setJavaScriptSignature(js::Object& o) const = 0;
	virtual js::Value     call(const js::Array& params) const = 0;
};


template<class R, class... Args>
class Func : public FuncBase
{
public:
//	typedef std::tuple<Args...> arg_t;
	enum { Size = sizeof...(Args) };

	virtual ~Func() = default;
	virtual bool isSeparator() const override
	{
		return false;
	}
	
	Func() : fn() {}

	Func( const std::function<R(typename Args::c_type ...)>& _f )
	: fn(_f)
	{}

	std::function<R(typename Args::c_type ...)> fn;

	js::Value call(const js::Array& parameters) const override
	{
		if(parameters.size() != sizeof...(Args))
			throw std::runtime_error("Size mismatch: "
				"Array has "    + std::to_string( parameters.size() ) + " element(s), "
				"but I expect " + std::to_string( sizeof...(Args) )   + " element(s)! "
			);
		
		// recursive template magic breaks loose:
		// recursively extract the JSON parameters, call 'fn' and collect its return value
		// and all output parameters into a tuple<> and return it as JSON array
		js::Array out_params;
		out_params.reserve( 1 + sizeof...(Args) ); // too big, but who cares?
		
		js::Value ret = helper<R, 0, sizeof...(Args), Args...>::call(fn, out_params, parameters);
		out_params.push_back( ret );
		return ret;
	}

	void setJavaScriptSignature(js::Object& o) const override
	{
		js::Array params;
		Type2Json<Args...>::get(params);
		
		o.emplace_back( "return", Type2String<R>::get() );
		o.emplace_back( "params", params );
		o.emplace_back( "separator", false );
	}
};


// Just a separating placeholder in the drop-down list. Does not calls anything.
class Separator : public FuncBase
{
public:
	Separator() = default;
	virtual bool isSeparator() const override { return true; }
	virtual void setJavaScriptSignature(js::Object& o) const override { o.emplace_back("separator", true); }
	virtual js::Value     call(const js::Array& params) const override { return js::Value{}; }
};

//typedef std::map< std::string, FuncBase* > FunctionMap;
typedef std::vector< std::pair< std::string, FuncBase*> > FunctionMap;
typedef FunctionMap::value_type FP;

#endif // FUNCTION_MAP_HH
