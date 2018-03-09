#include <gtest/gtest.h>

#include "inout.hh" // for to_json() and from_json()

TEST(ToFromJsonTest, SimpleCases)
{
	EXPECT_EQ( js::Value(""), to_json<std::string>("") );
}

