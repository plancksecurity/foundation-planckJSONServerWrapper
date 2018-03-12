#include <gtest/gtest.h>

#include "inout.hh" // for to_json() and from_json()
#include "json_spirit/json_spirit_writer.h"

namespace js = json_spirit;

TEST(ToFromJsonTest, SimpleCases)
{
	// simple_write() has raw_utf8 not set, so every non-ASCII is escaped:
	EXPECT_EQ( R"("")"        , simple_write(to_json<std::string>("")) );
	EXPECT_EQ( R"("\n")"      , simple_write(to_json<std::string>("\n")) );
	EXPECT_EQ( R"("\u001F")"  , simple_write(to_json<std::string>("\x1f")) );
	EXPECT_EQ( R"("\u00E4\u00F6\u00FC")" , simple_write(to_json<std::string>("äöü")) );
	EXPECT_EQ( R"("\uD83D\uDCA3")" , simple_write(to_json<std::string>("\xf0\x9f\x92\xa3")) ); // <U+1F4A3> Unicode BOMB
}

// this is how the JSON Adapter writes its JSON:
TEST(ToFromJsonTest, RawUtf8Cases)
{
	EXPECT_EQ( R"("")"        , js::write(to_json<std::string>(""), js::raw_utf8) );
	EXPECT_EQ( R"("\n")"      , js::write(to_json<std::string>("\n"), js::raw_utf8) );
	EXPECT_EQ( R"("\u001F")"  , js::write(to_json<std::string>("\x1f"), js::raw_utf8) );
	EXPECT_EQ( R"("äöü")" , js::write(to_json<std::string>("äöü"), js::raw_utf8) );
	EXPECT_EQ( "\"\xF0\x9f\x92\xA3\"" , js::write(to_json<std::string>("\xf0\x9f\x92\xa3"), js::raw_utf8) ); // <U+1F4A3> Unicode BOMB
}

