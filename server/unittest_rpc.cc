#include <gtest/gtest.h>
#include "json_rpc.hh"
#include "function_map.hh"
#include "c_string.hh"
#include "json_spirit/json_spirit_reader.h"

#include <pEp/pEp_string.h> // for new_string()
#include <pEp/stringlist.h>

#include <vector>


namespace js = json_spirit;

namespace json_spirit
{
std::ostream& operator<<(std::ostream& os, const Value& value)
{
	js::write(value, os, js::pretty_print | js::raw_utf8 );
	return os;
}

} // end of namespace json_spirit

namespace {

int add_mul_simple(int x, int y, int z)
{
	return (x+y) * z;
}

// test function for InOut parameters etc.
char* add_mul_inout(int x, const char* y_str, int* z_result, char** result)
{
	const int y = y_str ? strtol(y_str, nullptr, 0) : -1;
	const int z = z_result ? *z_result : -1;
	const int r = (x+y) * z;
	
	if(z_result)
		*z_result = r;
	
	const std::string rs = std::to_string(r);
	char* rcs = new_string( rs.c_str(), 0 ); // == strdup() but allocated on Engine's heap
	*result = rcs;
	return new_string( ("x" + rs + "x").c_str(), 0);
}

const FunctionMap test_functions = {
		FP( "add_mul_simple", new Func<int, In<int>, In<int>, In<int>>( &add_mul_simple )),
		FP( "add_mul_inout", new Func<char*, In<int>, In<c_string>, InOutP<int>, Out<char*>>( &add_mul_inout )),
		FP( "stringlist_add", new Func<stringlist_t*, In<stringlist_t*, DontOwn>, In<c_string>>( &stringlist_add )),
	};


struct TestEntry
{
	std::string input;
	std::string result;
};


std::ostream& operator<<(std::ostream& o, const TestEntry& tt)
{
	return o << "input=«" << tt.input << "», result=«" << tt.result << "».  ";
}




const std::vector<TestEntry> testValues =
	{
		{ "{\"jsonrpc\":\"2.0\", \"id\":21, \"method\":\"add_mul_simple\", \"params\":[10,11,12]}",
		  "{\"jsonrpc\":\"2.0\", \"id\":21, \"result\":{ \"outParams\":[], \"return\":252}}"
		},
		{ "{\"jsonrpc\":\"2.0\", \"id\":22, \"method\":\"add_mul_simple\", \"params\":[10,-11,-12]}",
		  "{\"jsonrpc\":\"2.0\", \"id\":22, \"result\":{ \"outParams\":[], \"return\":12}}"
		},
		{ "{\"jsonrpc\":\"2.0\", \"id\":23, \"method\":\"add_mul_inout\", \"params\":[100,\"111\",123, \"dummy\"]}",
		  "{\"jsonrpc\":\"2.0\", \"id\":23, \"result\":{ \"outParams\":[\"25953\",25953], \"return\":\"x25953x\"}}"
		},
		{ "{\"jsonrpc\":\"2.0\", \"id\":24, \"method\":\"stringlist_add\", \"params\":[[\"abc\",\"def\"], \"ADD\"]}",
		  "{\"jsonrpc\":\"2.0\", \"id\":24, \"result\":{ \"outParams\":[], \"return\":[\"abc\", \"def\", \"ADD\"]}}"
		},

	};

} // end of anonymous namespace


class RpcTest : public ::testing::TestWithParam<TestEntry>
{
	// intentionally left blank for now.
};

INSTANTIATE_TEST_CASE_P(RpcTestInstance, RpcTest, testing::ValuesIn(testValues) );

TEST_P( RpcTest, Meh )
{
	const auto v = GetParam();
	js::Value request;
	js::read_or_throw(v.input, request);

	js::Value expected_result;
	js::read_or_throw(v.result, expected_result);
	
	const js::Value actual_result = call( test_functions, request.get_obj(), nullptr, false ); // don't check for security token in this unittest
	EXPECT_EQ( expected_result, actual_result );
}


TEST( RpcTest, StringListAdd)
{
	js::Value v;
	js::read_or_throw("[\"abc\", \"def\"]", v);
	
	stringlist_t* sl = from_json<stringlist_t*>( v );
	std::cout << "€€: " << to_json(sl) << std::endl;
	EXPECT_EQ(1,1);
}
