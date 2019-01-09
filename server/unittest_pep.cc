#include <gtest/gtest.h>

#include "inout.hh" // for to_json() and from_json()
#include "pEp-types.hh"
#include "json_spirit/json_spirit_writer.h"
#include "json_spirit/json_spirit_reader.h"
#include <pEp/message.h>
#include <pEp/pEp_string.h>
#include <vector>

namespace js = json_spirit;

namespace {

// Generated from pep-mini.png, tanks to xxd command line tool!
static const unsigned char pep_mini_png[103] =
	{
		0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d,
		0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x08,
		0x01, 0x03, 0x00, 0x00, 0x00, 0xca, 0xb8, 0xed, 0xd4, 0x00, 0x00, 0x00,
		0x06, 0x50, 0x4c, 0x54, 0x45, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x55,
		0xc2, 0xd3, 0x7e, 0x00, 0x00, 0x00, 0x1c, 0x49, 0x44, 0x41, 0x54, 0x08,
		0xd7, 0x63, 0xd8, 0x7c, 0x8d, 0xe1, 0x84, 0x24, 0x43, 0xf7, 0x45, 0x86,
		0x0e, 0x41, 0x86, 0xcf, 0xf7, 0x18, 0x1a, 0x04, 0x20, 0x08, 0x00, 0x6e,
		0x01, 0x07, 0xe1, 0xd8, 0x5e, 0xc3, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x49,
		0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82
	};

static const char mini_text[] = "„Fix, Schwyz!“, quäkt Jürgen blöd vom Paß.";

} // end of anonymous namespace

class PEPJsonTest : public ::testing::Test
{
	// intentionally left blank for now.
};



TEST_F( PEPJsonTest, Msg )
{
	message* m = new_message(PEP_dir_incoming);
	ASSERT_NE(m, nullptr);
	// only check some members
	ASSERT_EQ(m->id, nullptr);
	ASSERT_EQ(m->to, nullptr);
	ASSERT_EQ(m->cc, nullptr);
	
	// now fill the members
	m->id = new_string("test.883fcfcb-c1f9-4d87-bf9f-64d3ff17d0f1@peptest.ch", 0);
	m->shortmsg = new_string("Test Mail", 0);
	m->longmsg  = new_string("This is only a test mail.\nPlease go on.\n", 0);
	m->longmsg_formatted = new_string("<!doctype html>\n<html><head><title>Test Mail</title></head><body>This is only a test mail.<br>Please go on.</body></html>\n", 0);
	
	m->attachments = new_bloblist( new_string((char*)pep_mini_png, sizeof(pep_mini_png)), sizeof(pep_mini_png), "image/png", "pep-mini.png" );
	bloblist_add( m->attachments, new_string(mini_text, 0), sizeof(mini_text), "text/plain", "pangram.txt" );
	
	m->recv = new_timestamp(1547064444);
	
	const js::Value v = to_json(m);
	const std::string serialized = js::write(v);
	std::cout << "~~~~\n" <<  serialized << "\n~~~~~~\n";
	
	js::Value v2;
	
	ASSERT_TRUE( js::read(serialized, v2) );
	message* m2 = from_json<message*>(v2);
	
	ASSERT_STREQ( m->id, m2->id );
	ASSERT_STREQ( m->shortmsg, m2->shortmsg );
	ASSERT_STREQ( m->longmsg, m2->longmsg );
	
	free_message(m2);
	free_message(m);
}
