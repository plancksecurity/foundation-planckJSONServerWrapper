var lineRegEx = /(\d+)\s+(\S+)\s+(0x[0-9a-fA-F]+)\s+(.*)/;

// Beautify the output by replacing overlong C++ symbol names by shorter aliases.
// NOTA BENE: Order of replacements matter!
var replaces =
	[
		[ /std::__1::allocator/g , "ALLOC" ],
		[ /std::__1::basic_string<char, std::__1::char_traits<char>, ALLOC<char> >/g, "STRING"],
		[ /std::__1::vector<json_spirit::Pair_impl<json_spirit::Config_vector<STRING > >, ALLOC<json_spirit::Pair_impl<json_spirit::Config_vector<STRING > > > >/g, "JS::VALUE"],
		[ /</g , '&lt;' ],
		[ />/g , '&gt;' ]
	];

function replaceAll(s)
{
	replaces.forEach(function(elem)
		{
			s = s.replace(elem[0], elem[1]);
		}
	);
	
	return s;
}

function addLogLine(nr, module, address, logline)
{
	document.getElementById('tbl').innerHTML +=
		'<tr><td align="right">'
		+ nr + '</td><td>'
		+ module + '</td><td>'
		+ address + '</td><td class="log">'
		+ replaceAll(logline) + '</td></tr>';
}


function parseData(stringData)
{
	var lines = stringData.split('\n');
	
	for(var i=0; i<lines.length; ++i)
	{
		var l = lines[i];
		var m = l.match(lineRegEx);
		if(m && m.length==5)
		{
			var nr       = m[1];
			var module   = m[2];
			var address  = m[3];
			var logline  = m[4];
			
			addLogLine(nr, module, address, logline);
		}else{
			document.getElementById('tbl').innerHTML += '<tr class="th0"><td colspan="4">' + replaceAll(l) + '&nbsp;</td></tr>';
		}
	}
}


function handleFileSelect(evt)
{
	var files = evt.target.files; // FileList object
	var file=files[0];

	// files is a FileList of File objects. List some properties.
	var output = [];
	for (var i = 0, f; f = files[i]; i++)
	{
		output.push('<li><strong>', escape(f.name), '</strong> (', f.type || 'n/a', ') - ',
		f.size, ' bytes. </li>');
	}
	
	document.getElementById('list').innerHTML = '<ul>' + output.join('') + '</ul>';
	document.getElementById('tbl').innerHTML = '';
	
	var reader = new FileReader();
	reader.onload = function(progressEvent)
	{
		var stringData = reader.result;
		parseData(stringData);
	}
	
	reader.readAsText(file, "UTF-8"); 
}


function handleDropzone()
{
	document.getElementById('tbl').innerHTML = '';
	parseData( document.getElementById("dropzone").value );
}
