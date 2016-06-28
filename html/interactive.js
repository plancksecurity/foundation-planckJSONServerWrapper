
var call_ID = 1000;
var openSessions = [];
var func;
var func_params = [];

var OutputParam = ["OP"];

function genInput(id, size, direction, value, onchange)
{
	return '<input type="text" size="' + size + '" id="' + id + '"'
		+ (direction == "Out" ? ' readonly' : '')
		+ ' value="' + value + '"'
		+ (onchange != undefined ? ' onChange="' + onchange + '"' : '')
		+ ' >';
}


function addString(nr)
{
	document.getElementById('debug').innerHTML += ' *  addString(' + nr + ')<\nfunc_params = ' + JSON.stringify(func_params) + '\n';
	var pp = {direction:"In", type:"StringList"};
	
	func_params[nr].push("");
	genStringList(nr, pp);
	document.getElementById('li_' + nr ).innerHTML = Param2Form.StringList(nr, pp, func_params[nr]);
}

function removeString(nr, idx)
{
	var pp = {direction:"In", type:"StringList"};
	func_params[nr].splice(idx,1);
	genStringList(nr, pp);
	document.getElementById('li_' + nr ).innerHTML = Param2Form.StringList(nr, pp, func_params[nr]);
}


function genStringList(nr, pp)
{
	if(func_params[nr] == undefined)
	{
		func_params[nr] = [];
	}
	var value = func_params[nr];
	var ret = 'StringList:<br>';
	for(var i=0; i<value.length; ++i)
	{
		ret +='#' + i + ': ' + genInput('inp_param_' + nr + '_' + i, 25, pp, value[i],
			 'func_params[' + nr + '][' + i + '] = this.value;' )
			+ '<input type="button" value="Remove!" onClick="removeString(' + nr + ',' + i + ');" >'
			+ '<br>';
	}
	
	document.getElementById('debug').innerHTML += "\nfunc_params = " + JSON.stringify(func_params) + '\n';
	
	return ret + '<input type="button" value="Add String!" onClick="addString(' + nr + ')">';
}


// generates the HTML form for the appropriate function parameter
var Param2Form =
	{
		Session : function(nr, pp, value)
					{
						return getSessions(true);
					},
		String : function(nr, pp, value)
					{
						return genInput('inp_param_' + nr , 25, pp.direction, value);
					},
		Integer : function(nr, pp, value)
					{
						return genInput('inp_param_' + nr , 25, pp.direction, value);
					},
		
		StringList : function(nr, pp, value)
					{
						if(pp.direction=="Out")
						{
							return 'StringList (output)';
						}else{
							return genStringList(nr,pp);
						}
					},
		Message : function(nr, pp, value)
					{
						if(pp.direction=='Out')
							return 'Message (output)';
						
						return 'Message:<table class="smalltable">'
							+ '<tr><td>dir: </td><td>'      + Param2Form.PEP_msg_direction( nr + '_dir', pp.direction, "(dir)") + '</td></tr>'
							+ '<tr><td>id: </td><td>'       + genInput('inp_param_' + nr + '_id', 16, pp.direction, "id") + '</td></tr>'
							+ '<tr><td>shortmsg: </td><td>' + genInput('inp_param_' + nr + '_smsg', 25, pp.direction, "shortmsg") + '</td></tr>'
							+ '<tr><td>longmsg: </td><td>'  + genInput('inp_param_' + nr + '_lmsg', 25, pp.direction, "longmsg") + '</td></tr>'
							+ '<tr><td>from: </td><td>'     + Param2Form.Identity( nr + '_from', pp.direction, "(from)") + '</td></tr>'
							+ '<tr><td>to: </td><td>'       + Param2Form.IdentityList( nr + '_to', pp.direction, "(to)") + '</td></tr>'

							+ '</table>';
					},
		Identity : function(nr, pp, value)
					{
						return 'Identity:<table class="smalltable">'
							+ '<tr><td>user_id: </td><td>'     + genInput('inp_param_' + nr + '_id', 16, pp.direction, "id") + '</td></tr>'
							+ '<tr><td>username: </td><td>'    + genInput('inp_param_' + nr + '_name', 16, pp.direction, "name") + '</td></tr>'
							+ '<tr><td>address: </td><td>'     + genInput('inp_param_' + nr + '_addr', 25, pp.direction, "address") + '</td></tr>'
							+ '<tr><td>fingerprint: </td><td>' + genInput('inp_param_' + nr + '_fpr', 25, pp.direction, "fingerprint") + '</td></tr>'
							+ '</table>';
					},
		IdentityList : function(nr, pp, value)
					{
						return 'IdentityList (only 1 entry supported here):<table class="smalltable">'
							+ '<tr><td>user_id: </td><td>'     + genInput('inp_param_' + nr + '_id', 16, pp.direction, "id") + '</td></tr>'
							+ '<tr><td>username: </td><td>'    + genInput('inp_param_' + nr + '_name', 16, pp.direction, "name") + '</td></tr>'
							+ '<tr><td>address: </td><td>'     + genInput('inp_param_' + nr + '_addr', 25, pp.direction, "address") + '</td></tr>'
							+ '<tr><td>fingerprint: </td><td>' + genInput('inp_param_' + nr + '_fpr', 25, pp.direction, "fingerprint") + '</td></tr>'
							+ '</table>';
					},
		PEP_enc_format : function(nr, pp, value)
					{
						var disabled = pp.direction == 'Out' ? " disabled" : "";
						
						return 'Encoding format: '
							+ '<input type="radio" name="inp_rad_' + nr + '" value="0"' + disabled + '>None,&nbsp;&nbsp;'
							+ '<input type="radio" name="inp_rad_' + nr + '" value="1"' + disabled + '>Pieces,&nbsp;&nbsp;'
							+ '<input type="radio" name="inp_rad_' + nr + '" value="2"' + disabled + '>S/MIME,&nbsp;&nbsp;'
							+ '<input type="radio" name="inp_rad_' + nr + '" value="3"' + disabled + '>PGP/MIME,&nbsp;&nbsp;'
							+ '<input type="radio" name="inp_rad_' + nr + '" value="4"' + disabled + ' checked>p≡p';
					},
		PEP_msg_direction : function(nr, pp, value)
					{
						var disabled = pp.direction == 'Out' ? " disabled" : "";
						
						return 'Message direction: '
							+ '<input type="radio" name="inp_rad_' + nr + '" value="0"' + disabled + '>Incoming,&nbsp;&nbsp;'
							+ '<input type="radio" name="inp_rad_' + nr + '" value="1"' + disabled + ' checked>Outgoing';
					}
	};

// fetches the form data and return appropriate JSON object
var Form2Param =
	{
		Session : function(nr, pp, value)
					{
						return $('input[name=sessionRadio]:checked', '#frm').val();
					},
		String : function(nr, pp, value)
					{
						return document.getElementById( 'inp_param_' + nr ).value;
					},
		Integer: function(nr, pp, value)
					{
						return parseInt( document.getElementById( 'inp_param_' + nr ).value );
					},
		StringList : function(nr, pp, value)
					{
						var ret = [];
						for(var i=0; i<value.length; ++i)
						{
							ret.push( document.getElementById('inp_param_' + nr + '_' + i).value );
						}
						return ret;
					},
		Message : function(nr, pp, value)
					{
						var ret = {};
						ret.dir = Form2Param.PEP_msg_direction(nr + '_dir');
						ret.id = document.getElementById('inp_param_' + nr + '_id').value;
						ret.shortmsg = document.getElementById('inp_param_' + nr + '_smsg').value;
						ret.longmsg = document.getElementById('inp_param_' + nr + '_lmsg').value;
						ret.from = Form2Param.Identity( nr + '_from');
						ret.to = Form2Param.IdentityList( nr + '_to');
						return ret;
					},
		Identity : function(nr, pp, value)
					{
						var ret = {};
						ret.user_id = document.getElementById('inp_param_' + nr + '_id').value;
						ret.username = document.getElementById('inp_param_' + nr + '_name').value;
						ret.address = document.getElementById('inp_param_' + nr + '_addr').value;
						ret.fpr = document.getElementById('inp_param_' + nr + '_fpr').value;
						return ret;
					},
		IdentityList : function(nr, pp, value)
					{
						var iden = {};
						iden.user_id = document.getElementById('inp_param_' + nr + '_id').value;
						iden.username = document.getElementById('inp_param_' + nr + '_name').value;
						iden.address = document.getElementById('inp_param_' + nr + '_addr').value;
						iden.fpr = document.getElementById('inp_param_' + nr + '_fpr').value;
						var ret = [];
						ret.push(iden);
						return ret;
					},
		PEP_enc_format : function(nr, pp, value)
					{
						return parseInt( $('input[name=inp_rad_' + nr + ']:checked', '#frm').val() );
					},
		PEP_msg_direction : function(nr, pp, value)
					{
						return parseInt( $('input[name=inp_rad_' + nr + ']:checked', '#frm').val() );
					}
	};

// uses {0}, {1} etc. instead of %d etc.
function sprintf(format)
{
	var args = Array.prototype.slice.call(arguments, 1);
	return format.replace(/{(\d+)}/g, function(match, number)
	{
		return typeof args[number] != 'undefined' ? args[number] : match;
	});
};


function getSessions(with_radio)
{
	if(openSessions.length == 0)
	{
		return '<i>(no open sessions)</i>';
	}
	
	var content = '';
	for(var i=0; i<openSessions.length; ++i)
	{
		var os = openSessions[i];
		content += (with_radio ? '<input type="radio" name="sessionRadio" value="' + os.session + '">' : '') + '&nbsp;<tt>' + os.session + '</tt><br>';
	}
	
	return content;
}

function showSessions()
{
	document.getElementById("sessions").innerHTML = getSessions(false);
}


function createSession()
{
	var url = document.getElementById("server").value + 'createSession';
	var request = {};
	request.method = 'createSession';
	request.params = [':-)'];
	request.id = ++call_ID;
	request.jsonrpc = "2.0";
	var x = $.post(url, JSON.stringify(request), null, "json")
		.done(function(data, status, xhr) {
			openSessions.push(data);
			showSessions();
		})
		.fail(function( hdr, txt, err) {
			alert( "error [" + hdr + "|" + txt + "|" + err + "]" );
		})
	;
}


function getAllSessions()
{
	var url = document.getElementById("server").value + 'getAllSessions';
	var request = {};
	request.method = 'getAllSessions';
	request.params = {};
	request.id = ++call_ID;
	request.jsonrpc = "2.0";
	var x = $.post(url, JSON.stringify(request), null, "json")
		.done(function(data, status, xhr) {
			openSessions = data;
			showSessions();
		})
		.fail(function( hdr, txt, err) {
			alert( "error [" + hdr + "|" + txt + "|" + err + "]" );
		})
	;
}


function displayResult(response)
{
	var pre = document.getElementById("resultpre");
	pre.innerHTML = "cklickiii [" + JSON.stringify(response, null, '\t') + ']'
	
	if (response.result)
	{
		pre.innerHTML = JSON.stringify(response.result, null, 2);
		pre.className = "green";
	}else if (response.error)
	{
		pre.innerHTML = JSON.stringify(response.error, null, 2);
		pre.className = "red";
	}
	
	getAllSessions(); // to update the session lists
	on_select_change(); // to update the function parameters
}


function button_click()
{
	var deb = document.getElementById("debug");
	var pre = document.getElementById("resultpre");
	pre.innerHTML = "momentele…";
	pre.className = "red";
	
	var url = document.getElementById("server").value + 'callFunction';
	var request = {};
	request.security_token = document.getElementById("security_token").value;
	request.method = document.getElementById("fn_name").value;
	request.params = new Array(func.params.length);
	
	for(var i=0, len=func.params.length; i<len; ++i)
	{
		var pp = func.params[i];
		
		if(pp.direction == "Out")
		{
			request.params[i] = OutputParam;
		}else{
			var form2param_func = Form2Param[pp.type];
			if(form2param_func == undefined)
				form2param_func = function(nr, pp, value) { return "Unknown(" + pp.type + ") nr=" + nr + ", value=" + value; };
			
			request.params[i] = form2param_func(i, pp, func_params[i]);
		}
	}
	
	request.id = ++call_ID;
	request.jsonrpc = "2.0";
	var x = $.post(url, JSON.stringify(request), null, "json")
		.done(function(moo) {
			displayResult(moo);
		})
		.fail(function( hdr, txt, err) {
			var emsg = 
			"\nAjax POST request failed: \n" 
			+ "Header: " + JSON.stringify(hdr, null, 2) + "\n"
			+ "Text: " +   JSON.stringify(txt, null, 2) + "\n"
			+ "Error:" +   JSON.stringify(err, null, 2) + "\n";
			
			var pre = document.getElementById("resultpre");
			pre.innerHTML += emsg;
			pre.className = "red";
			
			alert(emsg);
		})
	;
	
	pre.innerHTML = "post request sent. request=" + JSON.stringify(request, null, 2);
	deb.innerHTML = "url=«" + url + "»,\nfn_name=“" + document.getElementById("fn_name").value + "”\nrequest=" + JSON.stringify(request, null, 2);
}


function prepare_call(f)
{
	func = f;
	if(f.separator)
	{
		document.getElementById("td_param").innerHTML = "——";
		document.getElementById("td_param").disabled = true;
		return;
	}
	
	document.getElementById("td_param").disabled = false;
	func_params = new Array(func.params.length);
	
	var len = f.params.length;
	if(len==0)
	{
		document.getElementById("td_param").innerHTML = '<i>(no parameters at all)</i>';
		return;
	}

	var parameters='<ol>';
	for(var i=0, len=f.params.length; i<len; ++i)
	{
		var pp = f.params[i];
		var param2form_func = Param2Form[pp.type];
		if(param2form_func == undefined)
			param2form_func = function(nr, pp, value)
				{
					if(pp.direction == 'Out')
					{
						return '<i>Output parameter of type "' + pp.type + '"</i>';
					}else{
						return "Unknown(" + pp.type + ", direction=" + pp.direction + ") nr=" + nr + ", value=" + value;
					}
				};
		
		parameters += '<li id="li_' + i + '">' + param2form_func(i, pp, func_params[i])+ '</li>';
	}
	parameters += '</ol>\n';
	document.getElementById("td_param").innerHTML = parameters;
}


function on_select_change()
{
	var fn = $( "#fn_name" ).val();
	for(var i=0, len=pep_functions.length; i<len; ++i)
	{
		var f = pep_functions[i];
		if(f.name == fn)
		{
			prepare_call(f);
		}
	}
}


function init_pep_functions()
{
	var optionList = "";
	for(var i=0, len=pep_functions.length; i<len; ++i)
	{
		var f = pep_functions[i];
		optionList += '<option' + (f.separator? ' disabled>':'>') + f.name + "</option>\n";
	}
	document.getElementById("fn_name").innerHTML = optionList;
	document.getElementById("spn_version").innerHTML = "version: " + server_version;
	getAllSessions();
}
