#include <gtest/gtest.h>

#include "inout.hh" // for to_json() and from_json()
#include "nfc.hh"   // for illegal_utf8 exception
#include "json_spirit/json_spirit_writer.h"
#include "json_spirit/json_spirit_reader.h"
#include <vector>

namespace js = json_spirit;

namespace {

// for JSON input
struct TestPair
{
	std::string input;
	std::string output;
};

// for JSON output
struct TestTriple
{
	std::string input;
	std::string output_esc; // with \uXXXX escapes
	std::string output_raw; // raw_utf8
};


std::ostream& operator<<(std::ostream& o, const TestPair& tp)
{
	return o << "input=«" << tp.input << "», output=«" << tp.output << "». ";
}

std::ostream& operator<<(std::ostream& o, const TestTriple& tt)
{
	return o << "input=«" << tt.input << "», esc=«" << tt.output_esc << "», raw=«" << tt.output_raw << "». ";
}


const char nullo[4] = {0,0,0,0};
const char null_x[4] = { '\0', 'X', '\0', '\n' };

const std::vector<TestPair> testValuesInput =
	{
		{ R"("")"                   , ""       },  // always start with the simple case ;-)
		{ R"("123")"                , "123"    },  // some ASCII digits. Still easy.
		{ R"("\n\\\b")"             , "\n\\\b" },  // backslash escapes for ASCII and control chars
		{ R"("\u001F")"             , "\x1f"   },  // C compiler knows \x##, but JSON does not
		{ R"("\u007F")"             , "\x7f"   },  // C compiler knows \x##, but JSON does not
		
		{ R"("\u00E4\u00F6\u00FC")" , "äöü"    },  // German umlauts from Unicode block "Latin-1 Supplement"
		{ R"("äöü")"                , "äöü"    },  // German umlauts from Unicode block "Latin-1 Supplement"
		
		{ R"("\u041C\u043E\u0441\u043A\u0432\u0430")" , "Москва" },  // some Cyrillic
		{ R"("Москва")"                               , "Москва" },  // some Cyrillic
		
		{ R"("\uD83D\uDCA3")"   , "\xF0\x9f\x92\xA3"  }, // Unicode Bomb <U+1F4A3>, an example for char outside of BMP
		{ "\"\xF0\x9f\x92\xA3\"", "\xF0\x9f\x92\xA3"  }, // Unicode Bomb <U+1F4A3>, an example for char outside of BMP
		
		{ R"("\u0000")"         , std::string(nullo, nullo+1)   },  // Yeah, 1 NUL byte
		{ R"("\u0000\u0000")"   , std::string(nullo, nullo+2)   },  // Yeah, 2 NUL bytes
		{ R"("\u0000X\u0000\n")", std::string(null_x, null_x+4) },  // guess what...
		
		{ "\"EOF\"", "EOF" }
	};


const std::vector<TestTriple> testValuesOutput =
	{
		{ ""      , R"("")"                   , R"("")"        },  // always start with the simple case ;-)
		{ "123"   , R"("123")"                , R"("123")"     },  // some ASCII digits. Still easy.
		{ "\n\\\b", R"("\n\\\b")"             , R"("\n\\\b")"  },  // backslash escapes for ASCII and control chars
		{ "\x1f"  , R"("\u001F")"             , R"("\u001F")"  },  // C compiler knows \x##, but JSON does not
		{ "\x7f"  , R"("\u007F")"             , R"("\u007F")"  },  // C compiler knows \x##, but JSON does not
		{ "äöü"  , R"("\u00E4\u00F6\u00FC")" , R"("äöü")"     },  // German umlauts from Unicode block "Latin-1 Supplement"
		{ "Москва", R"("\u041C\u043E\u0441\u043A\u0432\u0430")" , R"("Москва")" },  // some Cyrillic
		{ "\xf0\x9f\x92\xa3", R"("\uD83D\uDCA3")" , "\"\xF0\x9f\x92\xA3\""        }, // Unicode Bomb <U+1F4A3>, an example for char outside of BMP
		
		{ std::string(nullo, nullo+1), R"("\u0000")"             , R"("\u0000")"  },  // Yeah, 1 NUL byte
		{ std::string(nullo, nullo+2), R"("\u0000\u0000")"       , R"("\u0000\u0000")"  },  // Yeah, 2 NUL bytes
		{ std::string(null_x, null_x+4), R"("\u0000X\u0000\n")" , R"("\u0000X\u0000\n")"  },  // guess what...
		
// a nasty and controversal example:
// <U+2028> (LINE SEPARATOR) and <U+2029> (PARAGRAPH SEPARATOR) are legal in JSON but not in JavaScript.
// A JSON encoder should _always_ escape them to avoid trouble in JavaScript:
// See http://timelessrepo.com/json-isnt-a-javascript-subset
//
//		{ "\xe2\x80\xa8 \xe2\x80\xa9", R"("\u2028 \u2029")", R"("\u2028 \u2029")" },

		{ "EOF", "\"EOF\"", "\"EOF\"" }
	};

} // end of anonymous namespace

class FromJsonTest : public ::testing::TestWithParam<TestPair>
{
	// intentionally left blank for now.
};

INSTANTIATE_TEST_CASE_P(FromJsonTestInstance, FromJsonTest, testing::ValuesIn(testValuesInput) );


class ToJsonTest : public ::testing::TestWithParam<TestTriple>
{
	// intentionally left blank for now.
};

INSTANTIATE_TEST_CASE_P(ToJsonTestInstance, ToJsonTest, testing::ValuesIn(testValuesOutput) );


TEST_P( FromJsonTest, Meh )
{
	const auto param = GetParam();
	js::Value v;
	js::read_or_throw( param.input, v);
	EXPECT_EQ( param.output,  from_json<std::string>(v) );
}

TEST_P( ToJsonTest, Meh )
{
	const auto v = GetParam();
	EXPECT_EQ( v.output_esc, simple_write( to_json<std::string>( v.input )) );
	EXPECT_EQ( v.output_raw, js::write( to_json<std::string>( v.input ), js::raw_utf8) );
}


TEST( ToJsonTest, Arrays )
{
	js::Array arr;
	EXPECT_EQ( js::write(arr), "[]" );
	js::Value varr{arr};
	EXPECT_EQ( js::write(varr), "[]" );

//	that does not work on gcc (Linux) & MSVC, for whatever reasons.
//	js::Array aarr{arr};
//	EXPECT_EQ( js::write(aarr), "[]" );

// but that works. Bizarre...
	js::Array aarr(arr);
	EXPECT_EQ( js::write(aarr), "[]" );
	
	Out<js::Array> oarr{arr};
	EXPECT_EQ( js::write(oarr.to_json()), "[]" );
	
	arr.push_back(42);
	arr.push_back("Meh");
	EXPECT_EQ( js::write(arr), "[42,\"Meh\"]" );
}


TEST( ToJsonTest, IllegalUtf8 )
{
	// examples from UTF-8 stress test:
	EXPECT_THROW( to_json<std::string>( "\x80" ), illegal_utf8 );
	EXPECT_THROW( to_json<std::string>( "\xbf" ), illegal_utf8 );
	EXPECT_THROW( to_json<std::string>( "\xc0" ), illegal_utf8 );
	EXPECT_THROW( to_json<std::string>( "\xc1" ), illegal_utf8 );
	EXPECT_THROW( to_json<std::string>( "\xc2" ), illegal_utf8 );
	EXPECT_THROW( to_json<std::string>( "\xfe" ), illegal_utf8 );
	EXPECT_THROW( to_json<std::string>( "\xff" ), illegal_utf8 );
	EXPECT_THROW( to_json<std::string>( "\xC0\xAF" ), illegal_utf8 ); // overlong "/"
	EXPECT_THROW( to_json<std::string>( "\xE0\x80\xAF" ), illegal_utf8 ); // overlong "/"
	EXPECT_THROW( to_json<std::string>( "\xF0\x80\x80\xAF" ), illegal_utf8 ); // overlong "/"
	
	EXPECT_THROW( to_json<std::string>( "\xF4\x90\x80\x80" ), illegal_utf8 ); // bigger than U+10FFFF
	EXPECT_THROW( to_json<std::string>( "\xED\xA0\x81\xED\xB0\x90" ), illegal_utf8 ); // CESU-8. Correct UTF-8 whoild be F0 90 90 80.
}


TEST( FromJsonTest, IllegalSequences )
{
	js::Value v;
	
	// too short \u escape sequences
	EXPECT_ANY_THROW( js::read_or_throw( R"("\")", v) );
	EXPECT_THROW( js::read_or_throw( R"("\q")", v), std::runtime_error );
	EXPECT_THROW( js::read_or_throw( R"("\u")", v), std::runtime_error );
	EXPECT_THROW( js::read_or_throw( R"("\u1")", v), std::runtime_error );
	EXPECT_THROW( js::read_or_throw( R"("\u12")", v), std::runtime_error );
	EXPECT_THROW( js::read_or_throw( R"("\u123")", v), std::runtime_error );
	
	// high surrogate without following legal low surrogate:
	EXPECT_THROW( js::read_or_throw( R"("\uD801")", v), std::runtime_error );
	EXPECT_THROW( js::read_or_throw( R"("\uD801D")", v), std::runtime_error );
	
	EXPECT_ANY_THROW( js::read_or_throw( R"("\uD801\")", v) );
	EXPECT_THROW( js::read_or_throw( R"("\uD801\u")", v), std::runtime_error );
	EXPECT_THROW( js::read_or_throw( R"("\uD801\uD")", v), std::runtime_error );
	EXPECT_THROW( js::read_or_throw( R"("\uD801\uDC")", v), std::runtime_error );
	EXPECT_THROW( js::read_or_throw( R"("\uD801\uDC0")", v), std::runtime_error );
	EXPECT_THROW( js::read_or_throw( R"("\uD801\u1234")", v), std::runtime_error );

	EXPECT_NO_THROW( js::read_or_throw( R"("\uD801\uDC02")", v) ); // legal UTF-16 sequence
	
	EXPECT_THROW( js::read_or_throw( R"("\uD801\uD801")", v), std::runtime_error ); // two high surrogates
	EXPECT_THROW( js::read_or_throw( R"("\uDC01\uDC01")", v), std::runtime_error ); // low surrogate without high surrogate before
}
