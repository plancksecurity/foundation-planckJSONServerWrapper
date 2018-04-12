#include <gtest/gtest.h>

#include "nfc.hh"   // for illegal_utf8 exception
#include <vector>


namespace {

struct TestPair
{
	std::string input;
	bool yesno;
};


std::ostream& operator<<(std::ostream& o, const TestPair& tt)
{
	return o << "input=«" << tt.input << "», isNfc=" << tt.yesno << ". ";
}


const char nullo[4] = {0,0,0,0};

const std::vector<TestPair> testValues =
	{
		{ ""      , true      },  // always start with the simple case ;-)
		{ "123"   , true      },  // some ASCII digits. Still easy.
		{ "\n\\\b", true      },  // backslash escapes for ASCII and control chars
		{ "ä", true },

		{ std::string(nullo, nullo+1), true  },  // Yeah, 1 NUL byte
		{ std::string(nullo, nullo+4), true  },  // Yeah, 4 NUL bytes
		
		{ "EOF", true }
	};

}

class NfcTest : public ::testing::TestWithParam<TestPair>
{
	// intentionally left blank for now.
};

INSTANTIATE_TEST_CASE_P(NfcTestInstance, NfcTest, testing::ValuesIn(testValues) );

TEST_P( NfcTest, Meh )
{
	const auto v = GetParam();
	EXPECT_EQ( v.yesno, isNFC( v.input ) );
}
