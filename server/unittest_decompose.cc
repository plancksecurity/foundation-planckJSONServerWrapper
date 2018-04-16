#include <gtest/gtest.h>

#include "nfc.hh"   // for illegal_utf8 exception
#include <vector>


namespace {

struct TestEntry
{
	std::string input;
	std::u32string output;
};

std::string uplus(const std::u32string& u32)
{
	std::string ret; ret.reserve(u32.size()*6);
	char hexa[16];
	for(auto c:u32)
	{
		snprintf(hexa,15, "<U+%04X>", unsigned(c));
		ret += hexa;
	}
	return ret;
}

std::ostream& operator<<(std::ostream& o, const TestEntry& tt)
{
	return o << "input=«" << tt.input << "», output=«" << uplus(tt.output) << "» ";
}


const char     nullo[4]   = {0,0,0,0};
const char32_t nullo32[4] = {0,0,0,0};

const std::vector<TestEntry> testValues =
	{
		{ ""      , U""      },  // always start with the simple case ;-)
		{ "123"   , U"123"   },  // some ASCII digits. Still easy.
		{ "\n\\\b", U"\n\\\b"  },  // backslash escapes for ASCII and control chars
		{ "ä"     , U"a\u0308"     },  // <U+00E4> small a with diaeresis -> decompose ä
		{ "\xc4\x85" , U"a\u0328"   },  // <U+0105> small a with ogonek -> decompose ą

		{ "a\xcc\x88", U"a\u0308" }, // a + <U+0308> combining diaresis
		{ "a\xcc\xa8", U"a\u0328" }, // a + <U+0328> combining ogonek
		{ "a\xcc\xa8\xcc\x88", U"a\u0328\u0308" }, // a + <U+0328> + <U+0308> ( ogonek +  diaresis)
		{ "a\xcc\x88\xcc\xa8", U"a\u0328\u0308" }, // a + <U+0308> + <U+0328> ( diaeresis + ogonek) -> canonicalOrdering reorders the accents!

		// ogonek sorts before diaeresis or breve-below:
		{ "ä\xcc\xa8ü\xcc\xa8ḫ\xcc\xa8", U"a\u0328\u0308u\u0328\u0308h\u0328\u032e"}, // ä + ogonek, ü + ogonek, h-breve-below + ogonek

		{ "a\xcc\x85\xcc\xbc", U"a\u033c\u0305" }, // a + <U+0305> + <U+033C> ( overline + seagull_below) -> canonicalOrdering reorders the accents!
		{ "a\xcc\xbc\xcc\x85", U"a\u033c\u0305" }, // a + <U+033C> + <U+0305> ( seagull_below + overline)

		{ std::string(nullo, nullo+1), std::u32string(nullo32, nullo32+1) },  // Yeah, 1 NUL byte
		{ std::string(nullo, nullo+4), std::u32string(nullo32, nullo32+4) },  // Yeah, 4 NUL bytes
		
		{ "EOF", U"EOF" }
	};

}

class DecomposeTest : public ::testing::TestWithParam<TestEntry>
{
	// intentionally left blank for now.
};

INSTANTIATE_TEST_CASE_P(DecomposeTestInstance, DecomposeTest, testing::ValuesIn(testValues) );

TEST_P( DecomposeTest, Meh )
{
	const auto v = GetParam();
	EXPECT_EQ( v.output, fromUtf8_decompose(v.input) );
}
