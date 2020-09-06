#include <gtest/gtest.h>

#include <pEp/constant_time_algo.hh>

namespace {

const char nullo[4] = {0,0,0,0};
const char null_x[4] = { '\0', 'X', '\0', '\n' };

const std::vector<std::string> testValuesInput =
	{
		{ ""                   },  // always start with the simple case ;-)
		{ "123"                },  // some ASCII digits. Still easy.
		{ "\n\\\b"             },  // backslash escapes for ASCII and control chars
		{ "äöü\x80\x7f"        },  // also with some non-ASCII chars
		
		{ std::string(nullo, nullo+1)   },  // Yeah, 1 NUL byte
		{ std::string(nullo, nullo+2)   },  // Yeah, 2 NUL bytes
		{ std::string(null_x, null_x+4) },  // guess what...
		
		{ "EOF" }
	};

}


class StringCompareTest : public ::testing::TestWithParam<std::string>
{
	// intentionally left blank for now.
};

INSTANTIATE_TEST_CASE_P(StringcompareTestInstance, StringCompareTest, testing::ValuesIn(testValuesInput) );


TEST_P( StringCompareTest, Equal )
{
	const auto param = GetParam();
	EXPECT_TRUE( pEp::constant_time_equal(param, param) );
	EXPECT_FALSE( pEp::constant_time_equal(param, "€") );
	EXPECT_FALSE( pEp::constant_time_equal("@@@", param) );
}
