#include "json_rpc.hh"
#include "json_spirit/json_spirit_utils.h"
#include "json_spirit/json_spirit_writer.h"
#include "json-adapter.hh"
#include "security-token.hh"

// Server side:

	js::Object make_result(const js::Value& result, int id)
	{
		js::Object ret;
		ret.emplace_back( "jsonrpc", "2.0" );
		ret.emplace_back( "result" , result );
		ret.emplace_back( "id"     , id );
		return ret;
	}
	
	
	js::Object make_error(JSON_RPC error_code, const std::string& error_message, const js::Value& data, int id)
	{
		js::Object err_obj;
		err_obj.emplace_back( "code", int(error_code) );
		err_obj.emplace_back( "message", error_message );
		if( !data.is_null() )
		{
			err_obj.emplace_back( "data", data );
		}
		
		js::Object ret;
		ret.emplace_back( "jsonrpc", "2.0" );
		ret.emplace_back( "error"  , err_obj );
		ret.emplace_back( "id"     , id );
		
		return ret;
	}

// Client side:

js::Object make_request(const std::string& functionName, const js::Array& parameters, const std::string& securityContext)
{
	static int request_id = 2000;
	
	js::Object request;
	request.emplace_back( "jsonrpc", "2.0" );
	request.emplace_back( "id"     , ++request_id );
	request.emplace_back( "security_token", securityContext );
	request.emplace_back( "method", functionName );
	request.emplace_back( "params", parameters );
	
	return request;
}


namespace
{
	FunctionMap::const_iterator find_in_vector(const FunctionMap& fm, const std::string& key)
	{
		return std::find_if(fm.begin(), fm.end(), [&key](const FunctionMap::value_type& v){ return v.first == key; });
	}
}

using json_spirit::find_value;


js::Object call(const FunctionMap& fm, const js::Object& request, Context* context)
{
	int request_id = -1;
	try
	{
		const auto rpc = find_value(request, "jsonrpc");
		if(rpc.type()!=js::str_type || rpc.get_str() != "2.0")
		{
			return make_error(JSON_RPC::INVALID_REQUEST, "Invalid request: no valid member \"jsonrpc\" found.", request, request_id);
		}
		
		const auto sec_token = find_value(request, "security_token");
		if(sec_token.type()!=js::str_type || context->verify_security_token(sec_token.get_str())==false )
		{
			return make_error(JSON_RPC::INVALID_REQUEST, "Invalid request: Wrong security token.", request, request_id);
		}
		
		const auto method = find_value(request, "method");
		if(method.type()!=js::str_type)
		{
			return make_error(JSON_RPC::INVALID_REQUEST, "Invalid request: no valid member \"method\" found.", request, request_id);
		}
		
		const std::string method_name = method.get_str();
		//const auto fn = fm.find(method_name);
		const auto fn = find_in_vector(fm,method_name);
		if(fn == fm.end() || fn->second->isSeparator())
		{
			return make_error(JSON_RPC::METHOD_NOT_FOUND, "Method \"" + method_name + "\" is unknown to me.", request, request_id);
		}
		
		const auto id = find_value(request, "id");
		if(!id.is_null() && id.type()!=js::int_type)
		{
			return make_error(JSON_RPC::INVALID_REQUEST, "Invalid request: no valid member \"id\" found.", request, request_id);
		}
		
		if(id.type()==js::int_type)
		{
			request_id = id.get_int();
		}
		
		const auto params = find_value(request, "params");
		if(!params.is_null() && params.type()!=js::array_type)
		{
			return make_error(JSON_RPC::INVALID_REQUEST, "Invalid request: no valid member \"params\" found.", request, request_id);
		}
		
		const js::Array p = ( params.type()==js::array_type ? params.get_array() : js::Array{} );
		
		std::cerr << "=== Now I do the call!\n"
			"\tmethod_name=\"" << method_name << "\","
			"\tparams=" << js::write(params) << ". ===\n";
		
		const js::Value result = fn->second->call(p, context);
		std::cerr << "=== Result of call: " << js::write(result, js::raw_utf8) << ". ===\n";
		std::cerr << "\tSessions: " << getSessions() << "\n";
		
		return make_result(result, request_id);
	}
	catch(const std::exception& e)
	{
		// JSON-RPC "internal error"
		return make_error(JSON_RPC::INTERNAL_ERROR, std::string("std::exception catched: \"") + e.what() + "\".", request, request_id );
	}
	catch(...)
	{
		// JSON-RPC "internal error"
		return make_error(JSON_RPC::INTERNAL_ERROR, "Unknown exception occured. :-(", request, request_id );
	}
}

