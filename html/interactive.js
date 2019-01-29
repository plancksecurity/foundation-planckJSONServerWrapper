
var call_ID = 1000;
var func;
var func_params = [];

var OutputParam = ["OP"];

function genInput(id, type, direction, value, onchange)
{
	var size=25;
	var aux='';
	
	switch(type)
	{
		case 'text' : size=30; aux='<input type="checkbox" id="' + id + '_chk" name="' + id + '_chk"> <small>parse esc seq.</small>';
			break;
		case 'number' : size=20;
			break;
	}
	
	return '<input type="text" type="' + type + '" id="' + id + '"'
		+ (direction == "Out" ? ' readonly' : '')
		+ ' value="' + value + '"'
		+ (onchange != undefined ? ' onChange="' + onchange + '"' : '')
		+ ' >' + aux;
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
						return '<small><i>– Sessions are handled internally –</i></small>';
					},
		String : function(nr, pp, value)
					{
						if(pp.direction=="Out")
						{
							return 'String output';
						}else{
							return genInput('inp_param_' + nr , 'text', pp.direction, value);
						}
					},
		Integer : function(nr, pp, value)
					{
						if(pp.direction=="Out")
						{
							return 'Integer output';
						}else{
							return genInput('inp_param_' + nr , 'number', pp.direction, 0);
						}
					},
		Bool : function(nr, pp, value)
					{
						if(pp.direction=="Out")
						{
							return 'Bool output';
						}else{
							return 'Bool: <input type="checkbox" id="inp_param_' + nr + '" value="' + value + '" >';
						}
					},
		Language : function(nr, pp, value)
					{
						if(pp.direction=="Out")
						{
							return 'Language output';
						}else{
							return '<label>Language: <select id="inp_param_' + nr + '" size="1">'
							/* TODO: generate that list dynamically via get_languagelist() */
									+ '<option value="ca">Català</option>'
									+ '<option value="de">Deutsch</option>'
									+ '<option value="en">English</option>'
									+ '<option value="es">Español</option>'
									+ '<option value="fr">Français</option>'
									+ '<option value="tr">Türkçe</option>'
									+ '<option value="ru">Русский</option>'// russian - no trustwords, yet
									+ '<option value="zh">中文</option>'   // chinese - no trustwords, yet
								+ '</select></label>';
						}
						
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
							+ '<tr><td>cc: </td><td>'       + Param2Form.IdentityList( nr + '_cc', pp.direction, "(cc)") + '</td></tr>'
							+ '<tr><td>bcc: </td><td>'      + Param2Form.IdentityList( nr + '_bcc', pp.direction, "(bcc)") + '</td></tr>'
							+ '<tr><td>attachments:</td><td>' + Param2Form.BlobList ( nr + '_blob', pp.direction, "(att)") + '</td></tr>'
							+ '</table>';
					},
		BlobList : function(nr, pp, value)
					{
						if(pp.direction=='Out')
							return 'BlobList (output)';
						
						return '<input type="file" multiple id="inp_blob_' + nr + '" name="inp_blob_' + nr + '">';
					},
		Identity : function(nr, pp, value)
					{
						return 'Identity:<table class="smalltable">'
							+ '<tr><td>user_id: </td><td>'     + genInput('inp_param_' + nr + '_id', 16, pp.direction, "") + '</td></tr>'
							+ '<tr><td>username: </td><td>'    + genInput('inp_param_' + nr + '_name', 16, pp.direction, "name") + '</td></tr>'
							+ '<tr><td>address: </td><td>'     + genInput('inp_param_' + nr + '_addr', 25, pp.direction, "address") + '</td></tr>'
							+ '<tr><td>fingerprint: </td><td>' + genInput('inp_param_' + nr + '_fpr', 25, pp.direction, "4ABE3AAF59AC32CFE4F86500A9411D176FF00E97") + '</td></tr>'
							+ '</table>';
					},
		IdentityList : function(nr, pp, value)
					{
						return 'IdentityList (only 1 entry supported here):<table class="smalltable">'
							+ '<tr><td>user_id: </td><td>'     + genInput('inp_param_' + nr + '_id', 16, pp.direction, "") + '</td></tr>'
							+ '<tr><td>username: </td><td>'    + genInput('inp_param_' + nr + '_name', 16, pp.direction, "name") + '</td></tr>'
							+ '<tr><td>address: </td><td>'     + genInput('inp_param_' + nr + '_addr', 25, pp.direction, "address") + '</td></tr>'
							+ '<tr><td>fingerprint: </td><td>' + genInput('inp_param_' + nr + '_fpr', 25, pp.direction, "4ABE3AAF59AC32CFE4F86500A9411D176FF00E97") + '</td></tr>'
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

function fromHex(charCode)
{
	if(charCode>=48 && charCode<=57)
		return charCode-48;
	
	if(charCode>=65 && charCode<=70)
		return charCode-65+10;
	
	if(charCode>=97 && charCode<=102)
		return charCode-97+10;
	
	return 0;
}

function de_backslash(input)
{
	var ret = "";
	for(var i=0; i<input.length; ++i)
	{
		var c = input.charAt(i);
		if(c=="\\")
		{
			++i;
			var e = input.charAt(i);
			switch(e)
			{
				case '0' : ret += "\000"; break;
				case 'a' : ret += "\a"; break;
				case 'b' : ret += "\b"; break;
				case 'f' : ret += "\f"; break;
				case 'n' : ret += "\n"; break;
				case 'r' : ret += "\r"; break;
				case 't' : ret += "\t"; break;
				case 'v' : ret += "\v"; break;
				case "\\" : ret += "\\"; break;
				case "\"" : ret += "\""; break;
				case "?"  : ret += "\177"; break;
				case 'u' : 
					{
						++i;
						var code = input.charCodeAt(i)*256*256*256
						         + input.charCodeAt(i+1)*256*256
						         + input.charCodeAt(i+2)*256
						         + input.charCodeAt(i+3);
						i+=4;
						ret += String.fromCharCode(code);
						break;
					}
			}
		}else{
			ret += c;
		}
	}
	
	return ret;
}

// fetches the form data and return appropriate JSON object
var Form2Param =
	{
		Session : function(nr, pp, value)
					{
						return null; // no longer necessary in the API. :-)
					},
		String : function(nr, pp, value)
					{
						var s = document.getElementById( 'inp_param_' + nr ).value
						if(document.getElementById( 'inp_param_' + nr + '_chk').checked)
						{
							return de_backslash(s);
						}else{
							return s;
						}
					},
		Integer: function(nr, pp, value)
					{
						return parseInt( document.getElementById( 'inp_param_' + nr ).value );
					},
		Bool: function(nr, pp, value)
					{
						return document.getElementById( 'inp_param_' + nr ).checked;
					},
		Language: function(nr, pp, value)
					{
						return document.getElementById( 'inp_param_' + nr ).value;
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
		BlobList : function(nr, pp, value)
					{
						var ret = [];
						var att = document.getElementById('inp_blob_' + nr);
						
						for(var i=0; i<att.files.length; ++i)
						{
							var f = att.files[i];
							var reader = new FileReader();
							reader.readAsArrayBuffer(f);
							
							var obj={};
							obj.mime_type = f.type;
							obj.filename = f.name;
							obj.value = // string containinng the base64-encoded octets of the file
							ret.push(obj);
						}
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
	
	on_select_change(); // to update the function parameters
}


function pc(c)
{
	if(c>32 && c<127)
	{
		return '“' + String.fromCharCode(c) + '|' + c + '”';
	}else
		return '(' + c + ')';
}

function button_click()
{
	var deb = document.getElementById("debug");
	var pre = document.getElementById("resultpre");
	pre.innerHTML = "momentele…";
	pre.className = "red";
	
	var url = document.getElementById("server").value + 'callFunction';
	var request = {};
	request.id = ++call_ID;
	request.jsonrpc = "2.0";
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
			
			// HACK to try to parse hdr.responseText:
			var rt = hdr.responseText;
			if($.type(rt) === 'string')
			{
				try{
					var j = JSON.parse(rt);
					emsg += "Header.responseText: " + JSON.stringify(j, null, 2) + "\n";
				}
				catch(e)
				{
					emsg += "Header.responseText cannot be parsed: " + e + "\n";
					for(var i=0; i<200; ++i)
					{
						emsg += i + ":" + pc(rt.charCodeAt(i)) + ', ';
					}
					emsg += "\n";
				}
			}
			
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
		optionList += '<option' + (f.separator? ' disabled> —— ':'>') + f.name + "</option>\n";
	}
	document.getElementById("fn_name").innerHTML = optionList;
	document.getElementById("spn_version").innerHTML = "version: " + server_version + " “" + server_version_name + "”";
	
	if(add_sharks)
	{
		var h1 = document.getElementsByTagName("h1")[0];
		var shark = document.createElement("img");
		shark.src = "data:image/png;base64," +
		"iVBORw0KGgoAAAANSUhEUgAAAMQAAABYAQMAAACQzMoQAAAABlBMVEUAAAAAAAClZ7nPAA" +
		"AAAXRSTlMAQObYZgAAAAFiS0dEAIgFHUgAAAAJcEhZcwAACxMAAAsTAQCanBgAAAAHdElN" +
		"RQfiBQQJFzochG6lAAAB0UlEQVRIx62WPW7cMBCFH8NCnXUBI8oxXBjRVXyELV1FzClyHQ" +
		"IpfA3eIAQCrFUIfCkokjNaMYAXZrWLbzX75s0PBbTzBb0z9InrkdF3SeiSeAdZu2T7OJnS" +
		"HYSfS9wdxPeJJclb+2b+JklyuyXlxC5hnxweWhpJ5+SNPGRWHvhFDKpZbCURUIUfC3kD8H" +
		"wqLQJ4OPsbukMnmwI2ADCCDCQTSQYAMF4LSM2ZoAVcSa43ZNnJHuZFC7g2x4J24Pqnuhy0" +
		"A+/VSqFtyqQU2rpjCQKweD3nzRlDL32zwplxk14PNRhg6BCBKAQUZ2ZvPfBjzzKpxTBiV7" +
		"KXs+Rg4yuAeRX1LE799XU4s4DaFlkiXQ1XHbnkX4cSrYkIOcW1eFObOTeBIUt3uMNyJF0W" +
		"0II9FpNDFtBqdSmdsWYBzV1fHEtQyWBqLv9UydQ5sHmUmrK6go3ocwCw27HOvoTwJ4MbjR" +
		"5rMeyHgZ80WU/m84YM/yWh9NsiF5XlNvnciytmTeLYDNAkjCK0Iv6r+JIkcY9iKBXBk5iW" +
		"JFXjmxhxSZLeS4JsvcthPGy5HhnYuyytIJPapkYSfYNIonfz0r7OhzsAPTK1j9+7bxav3Z" +
		"eMS5e84IPnH+BiDVEeA5CdAAAAAElFTkSuQmCC";
		shark.alt = "Shark. CC_BY Rachel Haley (AU).";
		shark.width = 98;
		shark.height = 44;
		h1.insertBefore(shark, h1.childNodes[0]);
	}
}


function create_doc()
{
	var format_def = {
		html : {   section_start : '<h3>' ,    section_end : '</h3>\n',
				subsection_start : '<h4>' , subsection_end : '</h4>\n',
				b_start : '<b>', b_end : '</b>',
				table_start  : '<table class="dtable">', table_end: '</table>\n',
				table_header : '<tr><th>Function name</th><th>Return Type</th><th>Parameters</th></tr>\n',
				line_start : '<tr>', line_end : '</tr>\n',
				cell_start : '<td>', cell_end : '</td>'
				},
		md :   {   section_start : '### '  ,    section_end : ' ###\n',
				subsection_start : '#### ' , subsection_end : ' ####\n',
				b_start : ' **', b_end : '** ',
				table_start : '', table_end: '\n',
				table_header: '| Function name | Return Type | Parameters |\n' +
				              '|---------------|-------------|------------|\n',
				line_start : ''  , line_end: '|\n',
				cell_start : '| ', cell_end: ' '
				},
		trac : {   section_start : '=== '  ,    section_end : ' ===\n',
				subsection_start : '==== ' , subsection_end : ' ====\n',
				b_start : ' **', b_end : '** ',
				table_start : ''  , table_end: '\n',
				table_header: '||= Function name =||= Return Type =||= Parameters =||\n',
				line_start : '|' , line_end : '|\n',
				cell_start : '| ', cell_end : ' |'
				}
	};
	
	var format_name = document.getElementById('doc_format').value;
	var fd  = format_def[format_name];
	var output = ""; 
	
	for(var i=0, len=pep_functions.length; i<len; ++i)
	{
		var f = pep_functions[i];
		if(f.separator)
		{
			if(output.length > 0)
			{
				output += fd.table_end + "\n";
			}
			output += fd.subsection_start + f.name + fd.subsection_end
			 + fd.table_start + fd.table_header;
		}else{
			output += fd.line_start
				+ fd.cell_start + f.name + fd.cell_end
				+ fd.cell_start + f["return"] + fd.cell_end
				+ fd.cell_start ;
			
			for( var p=0, plen = f.params.length; p<plen; ++p)
			{
				if(p>0) output += ', ';
				output += f.params[p].type;
				switch(f.params[p].direction)
				{
					case 'In'    : break;
					case 'Out'   : output += '⇑'; break;
					case 'InOut' :  output += '⇕'; break;
				}
			}
			
			output += fd.cell_end + fd.line_end;
			
		}
	}
	
	output =
		fd.section_start + 'Function reference for the p≡p JSON Server Adapter. Version “' + server_version_name + '”, API version ' + server_version + fd.section_end
		+ 'Output parameters are denoted by a ' + fd.b_start + '⇑' + fd.b_end + ', '
		+ 'InOut parameters are denoted by a ' + fd.b_start + '⇕' + fd.b_end + ' after the parameter type.\n\n'
		+ output + fd.table_end;

	if(format_name != "html")
		output = "<pre>" + output + "\n</pre>";
	
	document.getElementById("doc_out").innerHTML = output;
}
