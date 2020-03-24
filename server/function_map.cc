#include "function_map.hh"
#include "c_string.hh"


FunctionMap::const_iterator FunctionMap::find(const std::string& key) const noexcept
{
	return std::find_if(v.begin(), v.end(), [&key](const FunctionMap::value_type& elem){ return elem.first == key; });
}


FunctionMap::FunctionMap(std::initializer_list<FunctionMap::value_type> il)
{
	v.reserve(il.size());
	for(const auto& elem : il)
	{
		// only add if function name is not already in FunctionMap
		if(elem.second->isSeparator() || this->find(elem.first)==this->end())
		{
			v.push_back(elem);
		}else{
			throw std::runtime_error("Duplicate function name \"" + elem.first + "\" in FunctionMap!");
		}
	}
}

template<>
js::Value Type2String<std::string>::get()  { return "String"; }

template<>
js::Value Type2String<c_string>::get()  { return "String"; }

template<>
js::Value Type2String<const char*>::get()  { return "String"; }

template<>
js::Value Type2String<char*>::get()  { return "String"; }


template<>
js::Value Type2String<int>::get()  { return "Integer"; }

template<>
js::Value Type2String<long>::get()  { return "Integer"; }

template<>
js::Value Type2String<long long>::get()  { return "Integer"; }


template<>
js::Value Type2String<unsigned>::get()  { return "Integer"; }

template<>
js::Value Type2String<unsigned long>::get()  { return "Integer"; }

template<>
js::Value Type2String<unsigned long long>::get()  { return "Integer"; }

template<>
js::Value Type2String<bool>::get()  { return "Bool"; }

template<>
js::Value Type2String<void>::get()  { return "Void"; }
