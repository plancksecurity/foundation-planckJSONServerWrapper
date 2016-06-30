#ifndef FUNCTION_MAP_HH
#define FUNCTION_MAP_HH

#include "json_spirit/json_spirit_value.h"
#include <type_traits>


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
	
	template<class... Args>
	using pack = std::tuple<Args...>;
	
	explicit In(const T& t) : value(t) {}
	~In();
	
	In(const In<T>& other);
	In(In<T>&& victim);
	
	In<T>& operator=(const In<T>&) = delete;
	
	// default implementation:
	static In<T> from_json(const js::Value& v, const js::Array& params, unsigned position)
	{
		T t = ::from_json<T>(v);
		return In<T>{t};
	}
	
	T value;
};


// to call functions that operate directly on the JSON data type
template<class T>
struct InRaw
{
	typedef js::Value c_type; // do not unwrap JSON data type
	
	template<class... Args>
	using pack = std::tuple<Args...>;
	
	explicit InRaw(const js::Value& t) : value(t) {}
	~InRaw() = default;
	
	InRaw(const InRaw<T>& other) = default;
	InRaw(InRaw<T>&& victim) = default;

	InRaw<T>& operator=(const InRaw<T>&) = delete;
	
	// default implementation:
	static InRaw<T> from_json(const js::Value& v, const js::Array& params, unsigned position)
	{
		return InRaw<T>{v};
	}
	
	js::Value value;
};


// helper classes to specify in- and out-parameters
template<class T>
struct InOut : public In<T>
{
	typedef In<T> Base;

	explicit InOut(const T& t) : Base(t) {}
	~InOut() = default;
	
	InOut<T>& operator=(const InOut<T>&) = delete;
	
	// default implementation:
	static InOut<T> from_json(const js::Value& v, const js::Array& params, unsigned position)
	{
		T t = ::from_json<T>(v);
		return InOut<T>{t};
	}
	
	js::Value to_json() const
	{
		return ::to_json<T>(Base::value);
	}
};


template<class T>
struct Out
{
	typedef T* c_type; // the according type in C function parameter
	
	template<class... Args>
	using pack = std::tuple<Out<T>, Args...>;
	
//	explicit Out(const T& v );
	explicit Out() : value{ new T{} } {}
	~Out();
	
	Out(const Out<T>& other);
	Out(Out<T>&& victim)
	: value(victim.value)
	{
		victim.value = nullptr;
	}
	
	// just to be sure they are not implicitly defined:
	Out<T>& operator=(const Out<T>& other) = delete;
	Out<T>& operator=(Out<T>&& victim) = delete;
	
	static Out<T> from_json(const js::Value& v, const js::Array& params, unsigned position)
	{
		return Out<T>{};
	}
	
	js::Value to_json() const
	{
		return ::to_json<T>(*value);
	}
	
	T* value = nullptr;
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


struct Concat
{
	template<class T, class... Args>
	std::tuple<Args...> operator()(const In<T>& in, const std::tuple<Args...>& rest) const
	{
		return rest;
	}

	template<class T, class... Args>
	std::tuple<Args...> operator()(const InRaw<T>& in, const std::tuple<Args...>& rest) const
	{
		return rest;
	}

	template<class T, class... Args>
	std::tuple<Out<T>, Args...> operator()(const Out<T>& out, const std::tuple<Args...>& rest) const
	{
		return std::tuple_cat(std::tuple<Out<T>>{out}, rest);
	}

	template<class T, class... Args>
	std::tuple<InOut<T>, Args...> operator()(const InOut<T>& out, const std::tuple<Args...>& rest) const
	{
		return std::tuple_cat(std::tuple<InOut<T>>{out}, rest);
	}
};

extern const Concat concat;



// helper functors for to_json(tuple<...>):
namespace
{
	template<unsigned U, unsigned V, class... Args>
	struct copy_value
	{
		static void to(js::Array& a, const std::tuple<Args...>& t)
		{
			a[U] = to_json(std::get<U>(t));
			copy_value<U+1, sizeof...(Args), Args...>::to(a, t);
		}
	};
	
	template<unsigned U, class... Args>
	struct copy_value<U,U,Args...>
	{
		static void to(js::Array& a, const std::tuple<Args...>& t) {}
	};
}


template<class... Args>
js::Value to_json(const std::tuple<Args...>& t)
{
	js::Array ret; ret.resize(sizeof...(Args));
	copy_value<0, sizeof...(Args), Args...>::to(ret, t);
	return ret;
}


template<class R, unsigned U, unsigned MAX, class... Args>
class helper;

template<class R, unsigned U, class... Args>
class helper<R, U, U, Args...>
{
public:
	typedef std::tuple<R> RetType;

	static RetType call( const std::function<R(typename Args::c_type...)>& fn, const js::Array& parameters, const Args&... args)
	{
		return RetType( fn(args.value...) );
	}
};


template<unsigned U, class... Args>
class helper<void, U, U, Args...>
{
public:
	typedef std::tuple<> RetType;

	static RetType call( const std::function<void(typename Args::c_type...)>& fn, const js::Array& parameters, const Args&... args)
	{
		fn(args.value...);
		return RetType{};
	}
};


template<class R, unsigned U, unsigned MAX, class... Args>
class helper
{
public:
	typedef std::tuple<Args...> Tuple;
	typedef typename std::tuple_element<U, Tuple>::type Element;
	typedef helper<R, U+1, MAX, Args...> NextHelper;
	
	typedef typename std::result_of<Concat(Element, typename NextHelper::RetType )>::type  RetType;
	
public:
	template<class... A2>
	static RetType call( const std::function<R(typename Args::c_type...)>& fn, const js::Array& parameters, const A2&... a2)
	{
		const Element element{ Element::from_json(parameters[U], parameters, U) };

		return concat( element, NextHelper::call(fn, parameters, a2..., element ) );
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
		
		return to_json( helper<R, 0, sizeof...(Args), Args...>::call(fn, parameters) );
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
