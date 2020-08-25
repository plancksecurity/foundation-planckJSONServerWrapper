#include "context.hh"
#include "json_rpc.hh"
#include "json_spirit/json_spirit_utils.h"
#include "json_spirit/json_spirit_writer.h"
#include "json-adapter.hh"
#include "security-token.hh"
#include "logger.hh"
#include <atomic>

namespace
{
	std::atomic<std::uint64_t> request_nr{0};
}


Logger& Log()
{
	static Logger L("jrpc");
	return L;
}



// Server side:

	js::Object make_result(const js::Value& result, int id)
	{
		js::Object ret;
		ret.emplace_back( "jsonrpc", "2.0" );
		ret.emplace_back( "id"     , id );
		ret.emplace_back( "thread_id", Logger::thread_id() );
		ret.emplace_back( "result" , result );
		
		DEBUG_OUT(Log(),  "make_result(): result: " + js::write(result) );
		return ret;
	}
	
	
	js::Object make_error(JSON_RPC error_code, const std::string& error_message, const js::Value& data, int id)
	{
		Log().error("make_error(): \"" + error_message + "\" data: " + js::write(data) );
		
		js::Object err_obj;
		err_obj.emplace_back( "code", int(error_code) );
		err_obj.emplace_back( "message", error_message );
		if( !data.is_null() )
		{
			err_obj.emplace_back( "data", data );
		}
		
		js::Object ret;
		ret.emplace_back( "jsonrpc", "2.0" );
		ret.emplace_back( "id"     , id );
		ret.emplace_back( "thread_id", Logger::thread_id() );
		ret.emplace_back( "error"  , err_obj );
		
		return ret;
	}

// for event delivery to clients:
js::Object make_request(const std::string& functionName, const js::Array& parameters)
{
	js::Object request;
	request.emplace_back( "method", functionName );
	request.emplace_back( "request_nr", ++request_nr );
	request.emplace_back( "thread_id", Logger::thread_id() );
	request.emplace_back( "params", parameters );
	
	return request;
}


using json_spirit::find_value;


js::Object call(const FunctionMap& fm, const js::Object& request, JsonAdapterBase* ja)
{
	Logger L("jrpc:call");
	int request_id = -1;
	try
	{
		const auto rpc = find_value(request, "jsonrpc");
		if(rpc.type()!=js::str_type || rpc.get_str() != "2.0")
		{
			return make_error(JSON_RPC::INVALID_REQUEST, "Invalid request: no valid member \"jsonrpc\" found.", request, request_id);
		}
		
		const auto sec_token = find_value(request, "security_token");
		const std::string sec_token_s = (sec_token.type()==js::str_type ? sec_token.get_str() : std::string() ); // missing or non-string "security_token" --> empty string.
		if( ja->verify_security_token(sec_token_s)==false )
		{
			return make_error(JSON_RPC::INVALID_REQUEST, "Invalid request: Wrong security token.", request, request_id);
		}
		
		const auto client_id = find_value(request, "client_id");
		const std::string client_id_s = (client_id.type()==js::str_type ? client_id.get_str() : std::string() ); // missing or non-string "client_id" --> empty string.
		
		const auto method = find_value(request, "method");
		if(method.type()!=js::str_type)
		{
			return make_error(JSON_RPC::INVALID_REQUEST, "Invalid request: no valid member \"method\" found.", request, request_id);
		}
		
		const std::string method_name = method.get_str();
		const auto fn = fm.find(method_name);
		//const auto fn = find_in_vector(fm,method_name);
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
		
		DEBUG_OUT(L, "method_name=\"" + method_name + "\"\n"
					"params=" + js::write(params) );
		
		Context context{ja};
		const js::Value result = fn->second->call(p, &context);
		DEBUG_OUT(L, "result=" + js::write(result, js::raw_utf8) );
		
		return make_result(result, request_id);
	}
	catch(const std::exception& e)
	{
		// JSON-RPC "internal error"
		return make_error(JSON_RPC::INTERNAL_ERROR, std::string("std::exception caught: \"") + e.what() + "\".", request, request_id );
	}
	catch(...)
	{
		// JSON-RPC "internal error"
		return make_error(JSON_RPC::INTERNAL_ERROR, "Unknown exception occured. :-(", request, request_id );
	}
}

