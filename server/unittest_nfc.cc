#include <gtest/gtest.h>

#include "nfc.hh"   // for illegal_utf8 exception
#include <vector>


namespace {

struct TestEntry
{
	std::string input;
	bool  is_nfc;
	IsNFC quick;
	std::string nfc;
};

typedef TestEntry TE;


std::ostream& operator<<(std::ostream& o, const TestEntry& tt)
{
	return o << "input=«" << tt.input << "», isNfc=" << tt.is_nfc << ", quick=" << tt.quick << ".  ";
}


const char nullo[4] = {0,0,0,0};

const std::vector<TestEntry> testValues =
	{
		{ ""         , true, IsNFC::Yes, ""           },  // always start with the simple case ;-)
		{ "123"      , true, IsNFC::Yes, "123"        },  // some ASCII digits. Still easy.
		{ "\n\\\b"   , true, IsNFC::Yes, "\n\\\b"     },  // backslash escapes for ASCII and control chars
		{ "ä"        , true, IsNFC::Yes, "ä"          },  // <U+00E4> small a with diaeresis
		{ "\xc4\x85" , true, IsNFC::Yes, "\xc4\x85"   },  // <U+0105> small a with ogonek

		{ "a\xcc\x88", false, IsNFC::Maybe, "ä"        }, // a + <U+0308> combining diaresis
		{ "a\xcc\xa8", false, IsNFC::Maybe, "\xc4\x85" }, // a + <U+0328> combining ogonek
		{ "a\xcc\xa8\xcc\x88", false, IsNFC::Maybe, "\xc4\x85\xcc\x88" }, // a + <U+0328> + <U+0308> (ogonek + diaeresis)
		{ "a\xcc\x88\xcc\xa8", false, IsNFC::Maybe, "\xc4\x85\xcc\x88" }, // a + <U+0308> + <U+0328> (diaeresis + ogonek)

		{ "\xc4\x85\xcc\x88" ,  true, IsNFC::Maybe, "\xc4\x85\xcc\x88" }, // <U+0105> small a with ogonek + combining diaeresis
		{ "ä\xcc\xa8"        , false, IsNFC::Maybe, "\xc4\x85\xcc\x88" }, // a diaeresis + <U+0328> combining ogonek

// Already implemented, because <U+305> and <U+33C> have neither "No" nor "Maybe" NFC class:
		{ "a\xcc\x85\xcc\xbc", false,  IsNFC::No  , "a\xcc\xbc\xcc\x85"}, // a + <U+0305> + <U+033C> (overline + seagull_below)
		{ "a\xcc\xbc\xcc\x85",  true,  IsNFC::Yes , "a\xcc\xbc\xcc\x85"}, // a + <U+033C> + <U+0305> (seagull_below + overline)

		{ std::string(nullo, nullo+1), true, IsNFC::Yes, std::string(nullo, nullo+1)  },  // Yeah, 1 NUL byte
		{ std::string(nullo, nullo+4), true, IsNFC::Yes, std::string(nullo, nullo+4)  },  // Yeah, 4 NUL bytes
		
		{ "EOF", true, IsNFC::Yes, "EOF" }
	};

}

class NfcTest : public ::testing::TestWithParam<TestEntry>
{
	// intentionally left blank for now.
};

INSTANTIATE_TEST_CASE_P(NfcTestInstance, NfcTest, testing::ValuesIn(testValues) );

TEST_P( NfcTest, Meh )
{
	const auto& v = GetParam();
	EXPECT_EQ( v.quick, isNFC_quick_check(v.input) );
	
	EXPECT_EQ( v.is_nfc, isNFC(v.input) );
	EXPECT_EQ( v.nfc   , toNFC(v.input) );
	
	if(v.is_nfc)
	{
		EXPECT_EQ( v.input, toNFC(v.input) );
	}
}
