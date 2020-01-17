"use strict";

function test_encode_decode()
{
	var rndName = gen_random_name();
	var testAddress = 'enc-dec-test.' + rndName + '@peptest.ch';
	var testUsername = 'Test User ' + rndName;
	var identity = { "address": testAddress, "user_id": "pEp_own_userId", "username": testUsername };
	
	var result = await call_json_rpc_function("myself", [identity] );
	
}

unittests.push( {"description": "Encode/Decode", "onclick":"test_encode_decode()" } );
