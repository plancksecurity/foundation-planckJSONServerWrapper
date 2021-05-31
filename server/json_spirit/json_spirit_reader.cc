//          Copyright John W. Wilkinson 2007 - 2014
// Distributed under the MIT License, see accompanying file LICENSE.txt

// json spirit version 4.08

#include "json_spirit_reader.h"
#include "json_spirit_reader_template.h"

namespace json_spirit {

#ifdef JSON_SPIRIT_VALUE_ENABLED
    bool read( const std::string& s, Value& value )
    {
        return read_string( s, value );
    }
    
    void read_or_throw( const std::string& s, Value& value )
    {
        read_string_or_throw( s, value );
    }

    bool read( std::istream& is, Value& value )
    {
        return read_stream( is, value );
    }

    void read_or_throw( std::istream& is, Value& value )
    {
        read_stream_or_throw( is, value );
    }

    bool read( std::string::const_iterator& begin, std::string::const_iterator end, Value& value )
    {
        return read_range( begin, end, value );
    }

    void read_or_throw( std::string::const_iterator& begin, std::string::const_iterator end, Value& value )
    {
        begin = read_range_or_throw( begin, end, value );
    }
    
    // UTF-8 encoding:
    template<>
    std::string encode_utf<std::string>(const unsigned u)
    {
        if(u <= 0x7F)
        {
            return std::string( 1, char(u) );
        }else if( u<= 0x7FF )
        {
            char buf[2] = { char( 0xC0 + (u>>6) ),
                            char( 0x80 + (u & 63) )
                          };
            return std::string( buf, buf+2 );
        }else if( u<= 0xFFFF )
        {
            char buf[3] = { char( 0xE0 + ( u>>12     ) ),
                            char( 0x80 + ((u>>6) & 63) ),
                            char( 0x80 + ( u     & 63) )
                          };
            return std::string( buf, buf+3 );

        }else if( u<= 0x10FFFF )
        {
            char buf[4] = { char( 0xF0 + ( u>>18      ) ),
                            char( 0x80 + ((u>>12) & 63) ),
                            char( 0x80 + ((u>> 6) & 63) ),
                            char( 0x80 + ( u      & 63) )
                          };
            return std::string( buf, buf+4 );
        }
        
        throw std::runtime_error( "Unicode codepoint " + std::to_string(u) + " is too big!");
    }
#endif

#if defined( JSON_SPIRIT_WVALUE_ENABLED ) && !defined( BOOST_NO_STD_WSTRING )
    bool read( const std::wstring& s, wValue& value )
    {
        return read_string( s, value );
    }

    void read_or_throw( const std::wstring& s, wValue& value )
    {
        read_string_or_throw( s, value );
    }

    bool read( std::wistream& is, wValue& value )
    {
        return read_stream( is, value );
    }

    void read_or_throw( std::wistream& is, wValue& value )
    {
        read_stream_or_throw( is, value );
    }

    bool read( std::wstring::const_iterator& begin, std::wstring::const_iterator end, wValue& value )
    {
        return read_range( begin, end, value );
    }

    void read_or_throw( std::wstring::const_iterator& begin, std::wstring::const_iterator end, wValue& value )
    {
        begin = read_range_or_throw( begin, end, value );
    }

    // implement UTF-16 encoding id wchar_t == 16 bit?  I don't know, and don't care at the moment...
    // template<>
    // std::wstring encode_utf<std::wstring>(const unsigned u)
    // {
    //    TODO... if ever necessary
    // }

#endif

#ifdef JSON_SPIRIT_MVALUE_ENABLED
    bool read( const std::string& s, mValue& value )
    {
        return read_string( s, value );
    }

    void read_or_throw( const std::string& s, mValue& value )
    {
        read_string_or_throw( s, value );
    }
    
    bool read( std::istream& is, mValue& value )
    {
        return read_stream( is, value );
    }

    void read_or_throw( std::istream& is, mValue& value )
    {
        read_stream_or_throw( is, value );
    }

    bool read( std::string::const_iterator& begin, std::string::const_iterator end, mValue& value )
    {
        return read_range( begin, end, value );
    }

    void read_or_throw( std::string::const_iterator& begin, std::string::const_iterator end, mValue& value )
    {
        begin = read_range_or_throw( begin, end, value );
    }
#endif

#if defined( JSON_SPIRIT_WMVALUE_ENABLED ) && !defined( BOOST_NO_STD_WSTRING )
    bool read( const std::wstring& s, wmValue& value )
    {
        return read_string( s, value );
    }

    void read_or_throw( const std::wstring& s, wmValue& value )
    {
        read_string_or_throw( s, value );
    }

    bool read( std::wistream& is, wmValue& value )
    {
        return read_stream( is, value );
    }

    void read_or_throw( std::wistream& is, wmValue& value )
    {
        read_stream_or_throw( is, value );
    }

    bool read( std::wstring::const_iterator& begin, std::wstring::const_iterator end, wmValue& value )
    {
        return read_range( begin, end, value );
    }

    void read_or_throw( std::wstring::const_iterator& begin, std::wstring::const_iterator end, wmValue& value )
    {
        begin = read_range_or_throw( begin, end, value );
    }
#endif

} // end of namespace json_spirit
