#include "function_map.hh"


template<>
js::Value Type2String<std::string>::get()  { return "String"; }

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
