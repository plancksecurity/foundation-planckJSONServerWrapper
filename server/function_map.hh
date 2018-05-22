#ifndef FUNCTION_MAP_HH
#define FUNCTION_MAP_HH

#include "inout.hh"

#include "json_spirit/json_spirit_value.h"
#include "json_spirit/json_spirit_writer.h"

#include "context.hh"
#include <type_traits>

// Just for debugging:
#include <iostream>
#include <pEp/message_api.h>


template<class R>
struct Return
{
	typedef R return_type;
	typedef Out<R> out_type;
};

template<class R, ParamFlag PF>
struct Return< Out<R, PF> >
{
	typedef R return_type;
	typedef Out<R, PF> out_type;
};

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
	typedef typename Return<R>::return_type ReturnType;
	
	enum { nr_of_output_params = 0 };
	enum { nr_of_input_params = 0 };

	static void copyParam( js::Array& dest, const js::Array& src, unsigned index )
	{
		// do nothing. :-)
	}

	static js::Value call( const std::function< ReturnType(typename Args::c_type...)>& fn, Context*, js::Array& out_parameters, const js::Array& parameters, const Args&... args)
	{
		typename Return<R>::out_type o{ fn(args.get_value()...) };
		return to_json( o );
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
	typedef typename Return<R>::return_type ReturnType;
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
	static js::Value call( const std::function< ReturnType(typename Args::c_type...)>& fn, Context* ctx, js::Array& out_parameters, const js::Array& parameters, const A2&... a2)
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
	typedef typename Return<R>::return_type ReturnType;
	
	virtual ~Func() = default;
	virtual bool isSeparator() const override
	{
		return false;
	}
	
	Func() : fn() {}

	Func( const std::function<ReturnType(typename Args::c_type ...)>& _f )
	: fn(_f)
	{}

	std::function<ReturnType(typename Args::c_type ...)> fn;

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
		out_params.reserve( Helper::nr_of_output_params );
		
		js::Value ret = Helper::call(fn, context, out_params, *p_params);
		
		js::Object rs;
		rs.emplace_back("outParams", std::move(out_params));
		rs.emplace_back("return", std::move(ret));
		
		if(context)
		{
			context->augment(rs);  // used e.g. add some debug infos to the status return value
		}
		return rs;
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
