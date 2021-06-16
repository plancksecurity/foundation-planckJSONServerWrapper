#include <gtest/gtest.h>

#include <random>
#include "inout.hh" // for to_json() and from_json()
#include "pEp-types.hh"
#include "pEp-utils.hh"
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
	const unsigned alphabet_size = sizeof(alphabet_base36)-1;
	
	static_assert( sizeof(alphabet_base36) >= 37 , "alphabet must have at least 36 chars + NUL");
	
	std::string base36(uint64_t u)
	{
		if(u==0) return "qq";
		std::string ret;
		while(u)
		{
			ret += alphabet_base36[u % alphabet_size];
			u = u / alphabet_size;
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
		init(&session, &dummy_send, &dummy_inject, &dummy_ensure_passphrase);
		
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
		std::cout << "Delete key with fpr <" << my_identity->fpr << ">\n";
		delete_keypair(session, my_identity->fpr);
		free_identity(my_identity);
		my_identity = nullptr;
		
		release(session);
		session = nullptr;
	}

	static
	int dummy_inject(SYNC_EVENT, void*) {return PEP_STATUS_OK; }
	
	static
	PEP_STATUS dummy_send(struct _message*) { return PEP_STATUS_OK; }
	
	static
	PEP_STATUS dummy_ensure_passphrase(PEP_SESSION, const char* /* fpr */) { return PEP_STATUS_OK; }
	
	PEP_SESSION session = nullptr;
	std::string random_name;
	pEp_identity* my_identity = nullptr;
};



TEST_F( EncodeDecodeTest, Msg )
{
	ASSERT_EQ( myself(session, my_identity) , PEP_STATUS_OK );
	
	const std::string msg1s = 
		"{ \"id\":\"<pEp." + gen_random_name() + "." + gen_random_name() + "@peptest.ch>\""
		", \"shortmsg\": \"Subject\""
		", \"longmsg\": \"Body\""
		", \"attachments\": []"
		", \"from\": " + js::write(to_json<pEp_identity*>(my_identity)) + "\n"
		", \"to\": [ " + js::write(to_json<pEp_identity*>(my_identity)) + "] \n"
		"}\n";

	js::Value msg1v;
	js::read_or_throw( msg1s, msg1v );

	message* msg2_raw = nullptr;
	auto msg1 = pEp::utility::make_c_ptr( from_json<message*>(msg1v), &free_message);

	std::cout << "=== MESSAGE ORIGINAL ===\n"
		<< js::write(to_json<message*>(msg1.get())) << std::endl;

	PEP_STATUS status_enc = encrypt_message(session, msg1.get(), nullptr, &msg2_raw, PEP_enc_format(3), 0);
	EXPECT_EQ( status_enc, PEP_STATUS_OK);
	ASSERT_NE( msg2_raw , nullptr);
	auto msg2 = pEp::utility::make_c_ptr( msg2_raw, &free_message);

	std::cout << "=== MESSAGE ENCRYPTED ===\n"
		<< js::write(to_json<message*>(msg2.get())) << std::endl;
	
	message* msg3_raw = nullptr;
	
	// FIXME: do you provide a non-empty keylist?
	stringlist_t* keylist = new_stringlist(nullptr);
	PEP_rating rating{};
	PEP_decrypt_flags_t flags{}; // FIXME: which flags do you use on input?
	
	PEP_STATUS status_dec = decrypt_message(session, msg2.get(), &msg3_raw, &keylist, &rating, &flags);
	EXPECT_EQ( status_dec, PEP_STATUS_OK);
	ASSERT_NE( msg3_raw , nullptr);
	auto msg3 = pEp::utility::make_c_ptr( msg3_raw, &free_message);
	
	std::cout << "=== MESSAGE DECRYPTED ===\n"
		<< js::write(to_json<message*>(msg3.get())) << std::endl;
}
