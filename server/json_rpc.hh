#ifndef JSON_RPC_HH
#define JSON_RPC_HH

#include "json_spirit/json_spirit_value.h"
#include "context.hh"
#include "function_map.hh"

namespace js = json_spirit;

enum class JSON_RPC
{
	PARSE_ERROR      = -32700,
	INVALID_REQUEST  = -32600,
	METHOD_NOT_FOUND = -32601,
	INVALID_PARAMS   = -32602,
	
	INTERNAL_ERROR   = -32603,
};

// Server side:

// parse the JSON-RPC 2.0 compatible "request", call the C function
// and create an appropiate "response" object (containing a result or an error)
js::Object call(const FunctionMap& fm, const js::Object& request, Context* context);

// create a JSON-RPC 2.0 compatible result response object
//js::Object make_result(const js::Value& result, int id);

// create a JSON-RPC 2.0 compatible error response object
js::Object make_error(JSON_RPC error_code, const std::string& error_message, const js::Value& data, int id);


// Client side:
js::Object make_request(const std::string& functionName, const js::Array& parameters, const std::string& securityContext);

#endif // JSON_RPC_HH
