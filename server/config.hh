#ifndef JSON_ADAPTER_CONFIG
#define JSON_ADAPTER_CONFIG

#if (__cplusplus >= 201606)  // std::string_view is C++17.
#   include <string_view>
    typedef std::string_view sv;

#else // in C++11 / C++14 use boost::string_view instead.
#   include <boost/utility/string_view.hpp>
    typedef boost::string_view sv;

    // if sv==boost::string_view these operations are not provided, neither by boost nor by the stdlib. :-(
    inline
    std::string& operator+=(std::string& s, sv v)
    {
        s.append(v.data(), v.size());
        return s;
    }

    inline
    std::string operator+(std::string s, sv v)
    {
        return s += v;
    }
    
#endif // C++17 switch


#endif // JSON_ADAPTER_CONFIG
