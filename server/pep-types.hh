// overloads, specializations & instantiations for pEp-sepcific data types.

#ifndef PEP_TYPES_HH
#define PEP_TYPES_HH

#include "function_map.hh"
#include <pEp/message_api.h>
#include <pEp/sync.h>

// in pEpEngine.h positive values are hex, negative are decimal. :-o
std::string status_to_string(PEP_STATUS status);

// just a tag type:
struct Language{};

template<>
struct In<Language>
{
	typedef const char* c_type; // the according type in C function parameter
	enum { is_output = false, need_input = true };
	
	~In() = default;
	
	In(const In<Language>& other) = delete;
	In(In<Language>&& victim) = delete;
	void operator=(const In<Language>&) = delete;
	
	// default implementation:
	In<Language>(const js::Value& v, Context*, unsigned)
	{
		const std::string vs = v.get_str();
		if(vs.size() != 2)
		{
			throw std::runtime_error("Language must be a string of length 2, but I got \"" + vs + "\".");
		}
		
		array_value[0]=vs[0];
		array_value[1]=vs[1];
		array_value[2]='\0';
	}
	
	js::Value to_json() const
	{
		return js::Value( std::string(get_value(), get_value()+2) );
	}
	
	c_type get_value() const { return array_value; }
	
	char array_value[3];
};



#endif // PEP_TYPES_HH
