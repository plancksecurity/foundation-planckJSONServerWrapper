#include <gtest/gtest.h>
#include <string>
#include "sha1.hh"

namespace
{
	struct TestValue
	{
		const std::string input;
		const std::string hash_hex;
		const std::string hash_base64;
	};
	
	const char nullo[4] = {0,0,0,0};
	
	const std::vector<TestValue> v =
	{
		{"", "da39a3ee5e6b4b0d3255bfef95601890afd80709", "2jmj7l5rSw0yVb/vlWAYkK/YBwk=" },
		{"abc", "a9993e364706816aba3e25717850c26c9cd0d89d", "qZk+NkcGgWq6PiVxeFDCbJzQ2J0=" },
		{"Q567ikmnbt%ZJNBVFDSertzhnbvfdswerthnb", "ccbac471be027d6495ed5dd2658e36c36eeb0663", "zLrEcb4CfWSV7V3SZY42w27rBmM=" },
		{"\x01\x02\x03\x04", "12dada1fff4d4787ade3333147202c3b443e376f", "EtraH/9NR4et4zMxRyAsO0Q+N28=" },
		{std::string(nullo, nullo+1), "5ba93c9db0cff93f52b521d7420e43f6eda2784f", "W6k8nbDP+T9StSHXQg5D9u2ieE8="},
		{std::string(nullo, nullo+2), "1489f923c4dca729178b3e3233458550d8dddf29", "FIn5I8TcpykXiz4yM0WFUNjd3yk="},
		{std::string(nullo, nullo+3), "29e2dcfbb16f63bb0254df7585a15bb6fb5e927d", "KeLc+7FvY7sCVN91haFbtvtekn0="},
		{std::string(nullo, nullo+4), "9069ca78e7450a285173431b3e52c5c25299e473", "kGnKeOdFCihRc0MbPlLFwlKZ5HM="},
		 // example from RFC 6455:
		{"dGhlIHNhbXBsZSBub25jZQ==258EAFA5-E914-47DA-95CA-C5AB0DC85B11",
			"b37a4f2cc0624f1690f64606cf385945b2bec4ea",
			"s3pPLMBiTxaQ9kYGzzhZRbK+xOo="
		},
	};
}

class Sha1Test : public ::testing::TestWithParam<TestValue>
{
	// Intentionally left blank	
};

INSTANTIATE_TEST_CASE_P( Sha1TestInstance, Sha1Test, testing::ValuesIn(v) );

TEST_P(Sha1Test, Meh)
{
	const TestValue& tv = GetParam();
	EXPECT_EQ( sha1::hex(tv.input), tv.hash_hex );
	EXPECT_EQ( sha1::base64(tv.input), tv.hash_base64 );
}
