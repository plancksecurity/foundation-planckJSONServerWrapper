#include <gtest/gtest.h>

#include "inout.hh" // for to_json() and from_json()
#include "nfc.hh"   // for illegal_utf8 exception
#include "json_spirit/json_spirit_writer.h"
#include <vector>

namespace js = json_spirit;

struct TestTriple
{
	std::string input;
	std::string output_esc; // with \uXXXX escapes
	std::string output_raw; // raw_utf8
};


std::ostream& operator<<(std::ostream& o, const TestTriple& tt)
{
	return o << "input=«" << tt.input << "», esc=«" << tt.output_esc << "», raw=«" << tt.output_raw << "». ";
}


const std::vector<TestTriple> testValues =
	{
		{ ""      , R"("")"                   , R"("")"        },  // always start with the simple case ;-)
		{ "123"   , R"("123")"                , R"("123")"     },  // some ASCII digits. Still easy.
		{ "\n\\\b", R"("\n\\\b")"             , R"("\n\\\b")"  },  // backslash escapes for ASCII and control chars
		{ "\x1f"  , R"("\u001F")"             , R"("\u001F")"  },  // C compiler knows \x##, but JSON does not
		{ "\x7f"  , R"("\u007F")"             , R"("\u007F")"  },  // C compiler knows \x##, but JSON does not
		{ "äöü"  , R"("\u00E4\u00F6\u00FC")" , R"("äöü")"     },  // German umlauts from Unicode block "Latin-1 Supplement"
		{ "Москва", R"("\u041C\u043E\u0441\u043A\u0432\u0430")" , R"("Москва")" },  // some Cyrillic
		{ "\xf0\x9f\x92\xa3", R"("\uD83D\uDCA3")" , "\"\xF0\x9f\x92\xA3\""        }, // Unicode Bomb <U+1F4A3>, an example for char outside of BMP

// a nasty and controversal example:
// <U+2028> (LINE SEPARATOR) and <U+2029> (PARAGRAPH SEPARATOR) are legal in JSON but not in JavaScript.
// A JSON encoder should _always_ escape them to avoid trouble in JavaScript:
// See http://timelessrepo.com/json-isnt-a-javascript-subset
//
//		{ "\xe2\x80\xa8 \xe2\x80\xa9", R"("\u2028 \u2029")", R"("\u2028 \u2029")" },

		{ "EOF", "\"EOF\"", "\"EOF\"" }
	};


class ToJsonTest : public ::testing::TestWithParam<TestTriple>
{
	// intentionally left blank for now.
};

INSTANTIATE_TEST_CASE_P(ToJsonTestInstance, ToJsonTest, testing::ValuesIn(testValues) );

TEST_P( ToJsonTest, Meh )
{
	const auto v = GetParam();
	EXPECT_EQ( v.output_esc, simple_write( to_json<std::string>( v.input )) );
	EXPECT_EQ( v.output_raw, js::write( to_json<std::string>( v.input ), js::raw_utf8) );
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
