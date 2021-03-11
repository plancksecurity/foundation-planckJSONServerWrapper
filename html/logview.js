

var lineRegEx = /(\d\d\d\d-\d\d-\d\d\.\d\d:\d\d:\d\d) (¶\S+) (\S+) :(.*)/;
var colors = ['fcf7de', 'defde0', 'def3fd', 'f0defd', 'fddfdf', 'dfdfdf', 'B7C68B', 'F4F0CB', 'DED29E'];

function addLogLine(timestamp, thread, severity, logline)
{
	document.getElementById('tbl').innerHTML +=
		'<tr class="thd_' + thread.substr(1) + '"><td>'
		+ timestamp + '</td><td>'
		+ thread + '</td><td>'
		+ severity + '</td><td class="log">'
		+ logline + '</td></tr>';
}


function handleFileSelect(evt)
{
	var threads = {};
	
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
		var lines = stringData.split('\n');
		var multiline = '';
		
		for(var i=0; i<lines.length; ++i)
		{
			var l = lines[i];
			var m = l.match(lineRegEx);
			if(m && m.length==5)
			{
				var timestamp = m[1];
				var thread    = m[2];
				var severity  = m[3];
				var logline   = m[4];
				switch(logline[0])
				{
					case '⎡' : multiline = logline.substr(1); break;
					case '⎢' : multiline += '<br>' + logline.substr(1); break;
					case '⎣' : multiline += '<br>' + logline.substr(1); 
					           addLogLine(timestamp, thread, severity, multiline);
					           break;
					default :
						multiline = '';
						addLogLine(timestamp, thread, severity, logline);
				}
				threads[thread.substr(1)] = 1;
				
			}else{
				document.getElementById('tbl').innerHTML += '<tr class="th0"><td colspan="4">' + l + '</td></tr>';
			}
		}
		
		// create CSS colors for threads
		var colnum = 0;
		var buttons = '<blockquote>Toggle threads:&nbsp;&nbsp;<button id="btn0" class="enabled" onclick="toggle(\'th0\')"> ∅ </button>';
		for(var t in threads)
		{
			var style = document.createElement('style');
			  document.head.appendChild(style);
			  
			var rule = '.thd_' + t + ' { background-color: #' + colors[colnum] + '; }'
			style.sheet.insertRule(rule);
			colnum = (colnum+1) % colors.length;
			buttons += '&nbsp;&nbsp;<button id="btn_' + t + '" class="enabled" onclick="toggle(\'thd_' + t +'\')" >¶' + t + '</button>';
		}
		
		threads["th0"] = 1;
		document.getElementById('list').innerHTML += buttons + '</blockquote>';
	}

	reader.readAsText(file, "UTF-8"); 
}


document.getElementById('files').addEventListener('change', handleFileSelect, false);
