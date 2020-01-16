'use strict';

// all unittests will add themselves here:
var unittests =
	[
		{"description":"call gen_random_name()", "onclick":"random_name_test()" }
	];


var base32_alphabet = '0123456789abcdefghijklmnopqrstuvwxyz';

// creates a string of 12 random letters
function gen_random_name()
{
	var rndArray = new Uint32Array(12);
	window.crypto.getRandomValues(rndArray);
	var s = '';
	for(var i=0; i<rndArray.length; ++i)
	{
		s += base32_alphabet[ rndArray[i] % (base32_alphabet.length-1) ];
	}
	return s;
}

function add_test_buttons()
{
	var content = '<table><tr class="t"><th></th><th>Test name</th><th></th></tr>\n';
	for(var i=0; i<unittests.length; ++i)
	{
		var u = unittests[i];
		content += '<tr class="t"><td>#' + (i+1) + '</td><td>' + u.description + '</td>'
			+ '<td><button type="button" onclick="' + u.onclick + '"> Run! </button></td></tr>\n';
	}
	content += '</table>';
	document.getElementById('unittest_div').innerHTML = content;
}

function random_name_test()
{
	document.getElementById('unittest_out').innerHTML =  'pEp_' + gen_random_name() + '_' + gen_random_name() + '@pep.lol';
}
