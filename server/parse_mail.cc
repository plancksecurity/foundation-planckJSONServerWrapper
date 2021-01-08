// a program that parses (decodeS) a mail from stdin and prints the 'struct message' as JSON to stdout.

#include <pEp/mime.h>
#include <pEp/status_to_string.hh>
#include <pEp/slurp.hh>
#include "pEp-types.hh"
#include <string>
#include <iostream>

namespace js = json_spirit;

std::string read_from_stdin()
{
	std::string mime_text;
	std::string line;
	while(std::cin)
	{
		std::getline(std::cin, line);
		mime_text += line;
		mime_text += "\r\n";
	}
	return mime_text;
}


PEP_STATUS parse_mail(const std::string& mime_text)
{
	bool has_pEp_msg = false;
	message* msg = nullptr;
	
	const PEP_STATUS status = mime_decode_message(mime_text.data(), mime_text.size(), &msg, &has_pEp_msg);
	
	std::cout << "status: " << pEp::status_to_string(status) << '\n';
	std::cout << "has_pEp_msg: " << (has_pEp_msg?"true":"false") << '\n';
	std::cout << "message: " << (msg ? js::write(to_json<const message*>(msg), js::pretty_print | js::raw_utf8 | js::single_line_arrays) : "null" ) << '\n';
	
	return status;
}


int main(int argc, char** argv)
{
	if(argc==1 || argv[1]==std::string("-"))
	{
		const std::string mime_text = read_from_stdin();
		return parse_mail(mime_text);
	}
	
	for(int a=1; a<argc; ++a)
	{
		std::cout << "=== File #" << a << ": \"" << argv[a] << "\" ===\n";
		const std::string mime_text = pEp::slurp(argv[a]);
		const PEP_STATUS status = parse_mail(mime_text);
		if(status!=PEP_STATUS_OK)
		{
			return status;
		}
	}
	
	return 0;
}
