#include <gtest/gtest.h>

#include "inout.hh" // for to_json() and from_json()

TEST(ToFromJsonTest, SimpleCases)
{
	EXPECT_EQ( "\"\""         , simple_write(to_json<std::string>("")) );
	EXPECT_EQ( "\"\\n\""      , simple_write(to_json<std::string>("\n")) );
	EXPECT_EQ( "\"\\u001F\"", simple_write(to_json<std::string>("\x1f")) );
	EXPECT_EQ( "\"äöü\""      , simple_write(to_json<std::string>("äöü")) );
}

