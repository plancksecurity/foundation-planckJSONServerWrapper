#include <gtest/gtest.h>

#include "inout.hh" // for to_json() and from_json()
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
		{ "\n\\\b", R"("\n\\b")"              , R"("\n\\b")"   },  // backslash escapes for ASCII and control chars
		{ "\x1f"  , R"("\u001F")"             , R"("\u001F")"  },  // C compiler knows \x##, but JSON does not
		{ "\x7f"  , R"("\u007F")"             , R"("\u007F")"  },  // C compiler knows \x##, but JSON does not
		{ "äöü"   , R"("\u00E4\u00F6\u00FC")" , R"("äöü")"     },
		{ "\xf0\x9f\x92\xa3", R"("\uD83D\uDCA3")" , "\"\xF0\x9f\x92\xA3\"" },

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

