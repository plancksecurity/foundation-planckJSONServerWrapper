#ifndef FUNCTION_MAP_HH
#define FUNCTION_MAP_HH

#include "json_spirit/json_spirit_value.h"
#include "json_spirit/json_spirit_writer.h"

#include "context.hh"
#include <type_traits>

// Just for debugging:
#include <iostream>
#include <pEp/message_api.h>


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
		return ::to_json<T>(value);
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


// helper classes to specify in- and out-parameters
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
	
	Out() : value{ new T{} }
	{
		if(typeid(T)==typeid(_message*))
		{
			std::cerr << "|$ Out<message*>(): this=" << *this << "\n";
		}
	}
	
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
	enum { nr_of_output_params = 0 };
	enum { nr_of_input_params = 0 };

	static void copyParam( js::Array& dest, const js::Array& src, unsigned index )
	{
		// do nothing. :-)
	}

	static js::Value call( const std::function<R(typename Args::c_type...)>& fn, Context*, js::Array& out_parameters, const js::Array& parameters, const Args&... args)
	{
		return to_json( fn(args.get_value()...) );
	}
};


// specialization for Return type == void
template<unsigned U, class... Args>
class helper<void, U, U, Args...>
{
public:
	enum { nr_of_output_params = 0 };
	enum { nr_of_input_params = 0 };

	static void copyParam( js::Array& dest, const js::Array& src, unsigned index )
	{
		// do nothing. :-)
	}

	static js::Value call( const std::function<void(typename Args::c_type...)>& fn, Context*, js::Array& out_parameters, const js::Array& parameters, const Args&... args)
	{
		fn(args.get_value()...);
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
	
	enum { nr_of_output_params = int(Element::is_output) + NextHelper::nr_of_output_params };
	enum { nr_of_input_params = int(Element::need_input) + NextHelper::nr_of_input_params };
	
	static void copyParam( js::Array& dest, const js::Array& src, unsigned index )
	{
		if(Element::need_input)
		{
			dest.push_back( src.at(index) );
			++index;
		}else{
			dest.push_back( js::Value{} ); // insert dummy parameter
		}
		NextHelper::copyParam( dest, src, index );
	}
	
	// A2... a2 are the alredy pealed-off paremeters
	template<class... A2>
	static js::Value call( const std::function<R(typename Args::c_type...)>& fn, Context* ctx, js::Array& out_parameters, const js::Array& parameters, const A2&... a2)
	{
		// extract the U'th element of the parameter list
		const Element element(parameters[U], ctx);
		
		const js::Value ret = NextHelper::call(fn, ctx, out_parameters, parameters, a2..., element );
		if(Element::is_output)
		{
			js::Value out = element.to_json();
			std::cerr << "|$ Out #" << U << " : " << js::write(out) << "\n";
			out_parameters.push_back( std::move(out) );
		}else{
			std::cerr << "|$ Param #" << U << " is not for output.\n";
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
struct Type2String<In<T, true>>
{
	static js::Value get() { js::Object ret; ret.emplace_back("direction", "In"); ret.emplace_back("type", Type2String<T>::get() ); return ret; }
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



// abstract base class for all Func<...> types below
class FuncBase
{
public:
	virtual ~FuncBase() = default;
	virtual bool  isSeparator() const = 0;
	virtual void  setJavaScriptSignature(js::Object& o) const = 0;
	virtual js::Value  call(const js::Array& params, Context* context) const = 0;
};


template<class R, class... Args>
class Func : public FuncBase
{
public:
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

	js::Value call(const js::Array& parameters, Context* context) const override
	{
		typedef helper<R, 0, sizeof...(Args), Args...> Helper;
		
		if(parameters.size() != Helper::nr_of_input_params)
			throw std::runtime_error("Size mismatch: "
				"Array has "    + std::to_string( parameters.size() ) + " element(s), "
				"but I expect " + std::to_string( Helper::nr_of_input_params) + " element(s)! "
			);
		
		const js::Array* p_params = &parameters;
		
		// create a copy of the parameters only if necessary
		js::Array param_copy;
		if( Helper::nr_of_input_params != sizeof...(Args) )
		{
			param_copy.reserve( Helper::nr_of_input_params );
			Helper::copyParam( param_copy, parameters, 0u );
			p_params = &param_copy; // use the copy instead of 'parameters'
		}
		
		// recursive template magic breaks loose:
		// recursively extract the JSON parameters, call 'fn' and collect its return value
		// and all output parameters into a tuple<> and return it as JSON array
		js::Array out_params;
		out_params.reserve( 1 + Helper::nr_of_output_params );
		
		js::Value ret = Helper::call(fn, context, out_params, *p_params);
		
		context->augment(ret);  // used e.g. add some debug infos to the status return value
		
		out_params.push_back( ret );
		return out_params;
	}

	void setJavaScriptSignature(js::Object& o) const override
	{
		js::Array params;
		Type2Json<Args...>::get(params);
		
		o.emplace_back( "return", Type2String<R>::get() );
		o.emplace_back( "params", std::move(params) );
		o.emplace_back( "separator", false );
	}
};


// Just a separating placeholder in the drop-down list. Does not calls anything.
class Separator : public FuncBase
{
public:
	Separator() = default;
	virtual bool isSeparator()                          const override { return true; }
	virtual void setJavaScriptSignature(js::Object& o)  const override { o.emplace_back("separator", true); }
	virtual js::Value  call(const js::Array&, Context*) const override { return js::Value{}; }
};

//typedef std::map< std::string, FuncBase* > FunctionMap;
typedef std::vector< std::pair< std::string, FuncBase*> > FunctionMap;
typedef FunctionMap::value_type FP;

#endif // FUNCTION_MAP_HH
