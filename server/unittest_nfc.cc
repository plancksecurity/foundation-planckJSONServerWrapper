#include <gtest/gtest.h>

#include "nfc.hh"   // for illegal_utf8 exception
#include <vector>


namespace {

struct TestEntry
{
	std::string input;
	int   yesno; // HACK: -1 means: will throw "unimplemented", yet.
	IsNFC quick;
};


std::ostream& operator<<(std::ostream& o, const TestEntry& tt)
{
	return o << "input=«" << tt.input << "», isNfc=" << tt.yesno << ", quick=" << tt.quick << ".  ";
}


const char nullo[4] = {0,0,0,0};

const std::vector<TestEntry> testValues =
	{
		{ ""      , 1, IsNFC::Yes      },  // always start with the simple case ;-)
		{ "123"   , 1, IsNFC::Yes      },  // some ASCII digits. Still easy.
		{ "\n\\\b", 1, IsNFC::Yes      },  // backslash escapes for ASCII and control chars
		{ "ä"     , 1, IsNFC::Yes        },  // <U+00E4> small a with diaeresis
		{ "\xc4\x85" , 1, IsNFC::Yes     },  // <U+0105> small a with ogonek
// Not implemented, yet:
		{ "a\xcc\x88", -1,  IsNFC::Maybe }, // a + <U+0308> combining diaresis
		{ "a\xcc\xa8", -1,  IsNFC::Maybe }, // a + <U+0328> combining ogonek
		{ "a\xcc\xa8\xcc\x88", -1,  IsNFC::Maybe }, // a + <U+0328> + <U+0308> ( ogonek +  diaresis)
		{ "a\xcc\x88\xcc\xa8", -1,  IsNFC::Maybe }, // a + <U+0308> + <U+0328> ( diaeresis + ogonek)
// use these values when full NFC check is implemented:
//		{ "a\xcc\x88",  1,  IsNFC::Yes }, // a + <U+0308> combining diaresis
//		{ "a\xcc\xa8",  1,  IsNFC::Yes }, // a + <U+0328> combining ogonek
//		{ "a\xcc\xa8\xcc\x88",  1,  IsNFC::Yes }, // a + <U+0328> + <U+0308> ( ogonek +  diaresis)
//		{ "a\xcc\x88\xcc\xa8",  0,  IsNFC::No }, // a + <U+0308> + <U+0328> ( diaeresis + ogonek)

// Already implemented, because <U+305> and <U+33C> have neither "No" nor "Maybe" NFC class:
		{ "a\xcc\x85\xcc\xbc",  0,  IsNFC::No  }, // a + <U+0305> + <U+033C> ( overline + seagull_below)
		{ "a\xcc\xbc\xcc\x85", +1,  IsNFC::Yes }, // a + <U+033C> + <U+0305> ( seagull_below + overline)

		{ std::string(nullo, nullo+1), 1, IsNFC::Yes  },  // Yeah, 1 NUL byte
		{ std::string(nullo, nullo+4), 1, IsNFC::Yes  },  // Yeah, 4 NUL bytes
		
		{ "EOF", 1, IsNFC::Yes }
	};

}

class NfcTest : public ::testing::TestWithParam<TestEntry>
{
	// intentionally left blank for now.
};

INSTANTIATE_TEST_CASE_P(NfcTestInstance, NfcTest, testing::ValuesIn(testValues) );

TEST_P( NfcTest, Meh )
{
	const auto v = GetParam();
	EXPECT_EQ( v.quick, isNFC_quick_check(v.input) );
	
	if(v.yesno == -1) // needs deep test which is unimplemented, yet.
	{
		EXPECT_THROW( isNFC( v.input ) , std::logic_error);
	}else{
		EXPECT_EQ( bool(v.yesno), isNFC( v.input ) );
	}
}
