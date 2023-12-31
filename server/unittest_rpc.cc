#include <gtest/gtest.h>
#include "json_rpc.hh"
#include "json-adapter.hh"
#include "function_map.hh"
#include "c_string.hh"
#include "pEp-types.hh"
#include "session_registry.hh"
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

std::ostream& operator<<(std::ostream& os, const Object& obj)
{
	js::write(obj, os, 0x1B, 0);
	return os;
}

} // end of namespace json_spirit



namespace {

// HACK: Define a dummy type, so I can define an operator<< which is used by GTest,
// which refuses to pretty-print js::Object directly, for whatever reason. *sigh*
struct O
{
	js::Object obj;
};

std::ostream& operator<<(std::ostream& os, const O& o)
{
	js::write(o.obj, os, 0x1B, 0);
	return os;
}

bool operator==(const O& o1, const O& o2)
{
	return o1.obj == o2.obj;
}

/// END OF HACK


class DummyAdapter : public JsonAdapterBase
{
public:
	DummyAdapter()
	: sr{nullptr, nullptr, 4}
	{}
	
	virtual bool verify_security_token(const std::string& token) const override { return true; }
	
	virtual void cache(const std::string& client_id, const std::string& func_name, const std::function<void(PEP_SESSION)>& fn) override
	{
		sr.add_to_cache(client_id, func_name, fn);
	}
	
private:
	SessionRegistry sr;
};


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


js::Array gen_array(size_t num_elements)
{
	js::Array a;
	for(unsigned u=0; u<num_elements; ++u)
	{
		a.push_back( js::Value( static_cast<int>(u)) );
	}
	return a;
}


const FunctionMap test_functions = {
		FP( "add_mul_simple", new Func<int, In<int>, In<int>, In<int>>( &add_mul_simple )),
		FP( "add_mul_inout" , new Func<char*, In<int>, In<c_string>, InOutP<int>, Out<char*>>( &add_mul_inout )),
		FP( "stringlist_add", new Func<Out<stringlist_t*, ParamFlag::DontOwn>, InOut<stringlist_t*>, In<c_string>>( &stringlist_add )),
		FP( "tohex_1",        new Func<char*, In<c_string>, In<size_t>>( &tohex )), // with explicit length parameter
		FP( "tohex_2",        new Func<char*, In<c_string>, InLength<>>( &tohex )), // with implicit length parameter, with dummy JSON parameter
		FP( "tohex_3",        new Func<char*, In<c_string>, InLength<ParamFlag::NoInput>>( &tohex )), // with implicit length parameter, without JSON parameter
		FP( "gen_array",      new Func<js::Array, In<size_t>>( &gen_array )),

// TODO: test FuncCache stuff, too. :-/
//		FP( "cache_s1",  new FuncCache<void, In_Pep_Session, In<c_string>> ( "cache_s1", &cache_s1 )),
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
		{ "{\"jsonrpc\":\"2.0\", \"id\":29, \"method\":\"tohex_3\", \"params\":[\"tohex\"]}",
		  "{\"jsonrpc\":\"2.0\", \"id\":29, \"result\":{ \"outParams\":[], \"return\":\"74 6f 68 65 78\"}}"
		},
		{ "{\"jsonrpc\":\"2.0\", \"id\":30, \"method\":\"tohex_3\", \"params\":[\"Größe\"]}", // some non-ASCII BMP chars
		  "{\"jsonrpc\":\"2.0\", \"id\":30, \"result\":{ \"outParams\":[], \"return\":\"47 72 c3 b6 c3 9f 65\"}}"
		},
		{ "{\"jsonrpc\":\"2.0\", \"id\":30, \"method\":\"tohex_3\", \"params\":[\"\\u0000\\uD83D\\uDE47\"]}", // all hell breaks loose: Non-BMP characters
		  "{\"jsonrpc\":\"2.0\", \"id\":30, \"result\":{ \"outParams\":[], \"return\":\"00 f0 9f 99 87\"}}"
		},

		{ "{\"jsonrpc\":\"2.0\", \"id\":40, \"method\":\"gen_array\", \"params\":[0]}",
		  "{\"jsonrpc\":\"2.0\", \"id\":40, \"result\":{ \"outParams\":[], \"return\":[]}}"
		},
		{ "{\"jsonrpc\":\"2.0\", \"id\":40, \"method\":\"gen_array\", \"params\":[1]}",
		  "{\"jsonrpc\":\"2.0\", \"id\":40, \"result\":{ \"outParams\":[], \"return\":[0]}}"
		},
		{ "{\"jsonrpc\":\"2.0\", \"id\":40, \"method\":\"gen_array\", \"params\":[2]}",
		  "{\"jsonrpc\":\"2.0\", \"id\":40, \"result\":{ \"outParams\":[], \"return\":[0,1]}}"
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
	static DummyAdapter dummyAdapter;
	
	const auto v = GetParam();
	js::Value request;
	js::read_or_throw(v.input, request);

	js::Value expected_result;
	js::read_or_throw(v.result, expected_result);
	auto r = request;
	
	const js::Value actual_result = call( test_functions, request.get_obj(), &dummyAdapter);
	js::Object result_obj = actual_result.get_obj();
	js::Object::iterator q = std::find_if(result_obj.begin(), result_obj.end(), [](const js::Pair& v){ return js::Config::get_name(v) == "thread_id"; } );
	result_obj.erase( q );
	EXPECT_EQ( O{expected_result.get_obj()}, O{result_obj} );
}
