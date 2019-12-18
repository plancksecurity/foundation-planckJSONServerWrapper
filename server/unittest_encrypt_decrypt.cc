#include <gtest/gtest.h>

#include <random>
#include "inout.hh" // for to_json() and from_json()
#include "pEp-types.hh"
#include "json_spirit/json_spirit_reader.h"
#include <pEp/message.h>
#include <pEp/pEp_string.h>
#include <vector>

namespace js = json_spirit;

namespace {

	std::string ptr0(const char* s)
	{
		return s ? ("\"" + std::string(s) + "\"") : "(NULL)";
	}

	const char alphabet_base36[]="qaywsxedcrfvtgbzhnujmikolp-_0987654321";
	static_assert( sizeof(alphabet_base36) >= 37 , "alphabet must have at least 36 chars + NUL");
	
	std::string base36(uint64_t u)
	{
		if(u==0) return "qq";
		std::string ret;
		while(u)
		{
			ret += alphabet_base36[u % sizeof(alphabet_base36)];
			u = u / sizeof(alphabet_base36);
		}
		return ret;
	}

	std::string gen_random_name()
	{
		static std::random_device rd;
		static std::mt19937_64 gen(rd());
		return base36( gen() );;
	}

} // end of anonymous namespace

class EncodeDecodeTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		random_name = gen_random_name();
		const std::string address = "encode-decode-test." + random_name + "@peptest.ch";
		const std::string username = "Test User " + random_name;
		
		const std::string my_idens =
			"{ \"address\":\"" + address + "\""
			", \"user_id\": \"pEp_own_userId\""
			", \"username\": \"" + username + "\""
			"}";
		
		js::Value my_idenv;
		js::read_or_throw( my_idens, my_idenv );
		
		my_identity = from_json<pEp_identity*>(my_idenv);
	}
	
	void TearDown() override
	{
		free_identity(my_identity);
	}

	static
	int dummy_inject(SYNC_EVENT, void*) {return PEP_STATUS_OK; }
	
	static
	PEP_STATUS dummy_send(struct _message*) { return PEP_STATUS_OK; }

	std::string random_name;
	pEp_identity* my_identity = nullptr;
};



TEST_F( EncodeDecodeTest, Msg )
{
	PEP_SESSION session;
	init(&session, &dummy_send, &dummy_inject);
	ASSERT_EQ( myself(session, my_identity) , PEP_STATUS_OK );
	
	/*
	message* m = new_message(PEP_dir_incoming);
	
	ASSERT_STREQ( m->id, m2->id );
	ASSERT_STREQ( m->shortmsg, m2->shortmsg );
	ASSERT_STREQ( m->longmsg, m2->longmsg );
	
	free_message(m2);
	free_message(m);
	*/
	
	std::cout << "@@: " << random_name << " °° " << gen_random_name() << "\n"
		"\tiden->address: " << ptr0(my_identity->address) << "\n"
		"\tiden->fpr: " << ptr0(my_identity->fpr) << "\n"
		"\n";
}
