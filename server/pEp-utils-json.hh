// some helper functions to deal with JSON data

#ifndef PEP_UTILS_JSON_HH
#define PEP_UTILS_JSON_HH

#include "json_spirit/json_spirit_utils.h"

namespace pEp {
namespace utility {

	// fetch a member from the given object, if set. (return a sensible NULL/default value if not)
	template<class T, js::Value_type VALUE_TYPE>
	T from_json_object(const js::Object& obj, const std::string& key)
	{
		const auto v = find_value(obj, key);
		if(v.type() == js::null_type) return T{};
		if(v.type() == VALUE_TYPE) return from_json<T>(v);
		
		throw std::runtime_error("JSON object has a member for key \"" + key + "\""
			" with incompatible type " + js::value_type_to_string( v.type())
			+ " instead of expected type " + js::value_type_to_string(VALUE_TYPE)
			);
	}


	template<class T>
	void to_json_object(js::Object& obj, const std::string& key, const T& value)
	{
		if(value!=T{})
		{
			obj.emplace_back( key, js::Value( to_json<T>(value) ));
		}
	}


} // end of namespace pEp::utility
} // end of namespace pEp

#endif // PEP_UTILS_JSON_HH
