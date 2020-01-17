"use strict";

async function test_encode_decode()
{
	var out = document.getElementById('unittest_out');
	var rndName = gen_random_name();
	var testAddress = 'enc-dec-test.' + rndName + '@peptest.ch';
	var testUsername = 'Test User ' + rndName;
	var identity = { "address": testAddress, "user_id": "pEp_own_userId", "username": testUsername };
	
	var ret = await call_json_rpc_function("myself", [identity] );
	var newIdent = ret.result.outParams[0];
	var fpr = newIdent.fpr;
	out.innerHTML = JSON.stringify(newIdent, null, 2);
	
	var msg1 = {
			"id":"<pEp." + gen_random_name() + "." + gen_random_name() + "@peptest.ch>",
			"shortmsg": "Subject",
			"longmsg": "Body",
			"attachments": [],
			"from": identity,
			"to": [ identity ]
		};

	ret = await call_json_rpc_function("encrypt_message", [msg1, null, "out", 3, 0] );
	
	out.innerHTML += "\n=========\n" + JSON.stringify(ret, null, 2);
	
	await call_json_rpc_function("delete_keypair", [fpr] );
}

unittests.push( {"description": "Encode/Decode", "onclick":"test_encode_decode()" } );
