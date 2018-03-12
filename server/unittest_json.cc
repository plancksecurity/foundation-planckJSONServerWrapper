#include <gtest/gtest.h>

#include "inout.hh" // for to_json() and from_json()

TEST(ToFromJsonTest, SimpleCases)
{
	EXPECT_EQ( R"("")"        , simple_write(to_json<std::string>("")) );
	EXPECT_EQ( R"("\n")"      , simple_write(to_json<std::string>("\n")) );
	EXPECT_EQ( R"("\u001F")"  , simple_write(to_json<std::string>("\x1f")) );
	EXPECT_EQ( R"("äöü")"     , simple_write(to_json<std::string>("äöü")) );
}

