// a program that parses (decodeS) a mail from stdin and prints the 'struct message' as JSON to stdout.

#include <pEp/mime.h>
#include <pEp/status_to_string.hh>
#include "pEp-types.hh"
#include <string>
#include <iostream>

namespace js = json_spirit;

int main()
{
	std::string mime_text;
	std::string line;
	while(std::cin)
	{
		std::getline(std::cin, line);
		mime_text += line;
		mime_text += "\r\n";
	}
	
	bool has_pEp_msg = false;
	message* msg = nullptr;
	
	const PEP_STATUS status = mime_decode_message(mime_text.data(), mime_text.size(), &msg, &has_pEp_msg);
	
	std::cout << "status: " << pEp::status_to_string(status) << '\n';
	std::cout << "has_pEp_msg: " << (has_pEp_msg?"true":"false") << '\n';
	std::cout << "message: " << (msg ? js::write(to_json<const message*>(msg), js::pretty_print | js::raw_utf8 | js::single_line_arrays) : "null" ) << '\n';
	
	return status;
}
