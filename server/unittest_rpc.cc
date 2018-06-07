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

class DummyContext : public Context
{
public:
	virtual bool verify_security_token(const std::string& token) const override { return true; }
	virtual void augment(js::Object&) override { /* do nothing */ }
};


DummyContext dummyContext;


// some example & test functions:

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


char* tohex(const char* input, size_t length)
{
	std::string h; h.reserve(length*3);
	char buffer[8] = { 0 };
	const char* end = input+length;
	for(; input<end; ++input)
	{
		snprintf(buffer,7, "%02hhx", (unsigned char)*input );
		if(!h.empty()) h += ' ';
		h += buffer;
	}
	return new_string( h.c_str(), 0 );
}


const FunctionMap test_functions = {
		FP( "add_mul_simple", new Func<int, In<int>, In<int>, In<int>>( &add_mul_simple )),
		FP( "add_mul_inout" , new Func<char*, In<int>, In<c_string>, InOutP<int>, Out<char*>>( &add_mul_inout )),
		FP( "stringlist_add", new Func<Out<stringlist_t*, ParamFlag::DontOwn>, InOut<stringlist_t*>, In<c_string>>( &stringlist_add )),
		FP( "tohex_1",        new Func<char*, In<c_string>, In<size_t>>( &tohex )), // with explicit length parameter
		FP( "tohex_2",        new Func<char*, In<c_string>, InLength<>>( &tohex )), // with implicit length parameter, with dummy JSON parameter
		FP( "tohex_3",        new Func<char*, In<c_string>, InLength<ParamFlag::NoInput>>( &tohex )), // with implicit length parameter, without JSON parameter
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
		  "{\"jsonrpc\":\"2.0\", \"id\":24, \"result\":{ \"outParams\":[[\"abc\", \"def\", \"ADD\"]], \"return\":[\"ADD\"]}}"
		},
		// tohex:
		{ "{\"jsonrpc\":\"2.0\", \"id\":25, \"method\":\"tohex_1\", \"params\":[\"tohex\",3]}",
		  "{\"jsonrpc\":\"2.0\", \"id\":25, \"result\":{ \"outParams\":[], \"return\":\"74 6f 68\"}}"
		},
		{ "{\"jsonrpc\":\"2.0\", \"id\":26, \"method\":\"tohex_1\", \"params\":[\"tohex\",5]}",
		  "{\"jsonrpc\":\"2.0\", \"id\":26, \"result\":{ \"outParams\":[], \"return\":\"74 6f 68 65 78\"}}"
		},
		{ "{\"jsonrpc\":\"2.0\", \"id\":27, \"method\":\"tohex_2\", \"params\":[\"tohex\",0]}",
		  "{\"jsonrpc\":\"2.0\", \"id\":27, \"result\":{ \"outParams\":[], \"return\":\"74 6f 68 65 78\"}}"
		},
		{ "{\"jsonrpc\":\"2.0\", \"id\":28, \"method\":\"tohex_2\", \"params\":[\"tohex\",\"dummy_parameter\"]}",
		  "{\"jsonrpc\":\"2.0\", \"id\":28, \"result\":{ \"outParams\":[], \"return\":\"74 6f 68 65 78\"}}"
		},
		{ "{\"jsonrpc\":\"2.0\", \"id\":28, \"method\":\"tohex_3\", \"params\":[\"tohex\"]}",
		  "{\"jsonrpc\":\"2.0\", \"id\":28, \"result\":{ \"outParams\":[], \"return\":\"74 6f 68 65 78\"}}"
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
	auto r = request;
	
	const js::Value actual_result = call( test_functions, request.get_obj(), &dummyContext);
	EXPECT_EQ( expected_result, actual_result );
}
