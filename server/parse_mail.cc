// a program that parses (decodeS) a mail from stdin and prints the 'struct message' as JSON to stdout.

#include <pEp/mime.h>
#include <pEp/status_to_string.hh>
#include <pEp/slurp.hh>
#include "pEp-types.hh"
#include <string>
#include <iostream>

namespace js = json_spirit;

std::string S(const char* s, std::size_t size = std::string::npos)
{
	if(s)
	{
		const std::string ss = std::string(s);
		if(size>ss.size())
		{
			return '\"' + ss + '\"';
		}else{
			return '\"' + ss.substr(0, size) + "…\"";
		}
	}else{
		return "(NULL)";
	}
}

std::ostream& operator<<(std::ostream& o, const bloblist_t* att)
{
	if(att == nullptr)
	{
		return o << "(NULL)";
	}
	
	o << bloblist_length(att) << " elements:\n";
	while(att)
	{
		o << "\t\t size=" << att->size
		  << ", mime_type=" << S(att->mime_type)
		  << ", filename=" << S(att->filename)
		  << ", data=" << S(att->value, 32) << ".\n";
		att = att->next;
	}
	return o;
}

std::ostream& operator<<(std::ostream& o, const tm* t)
{
	if(t==nullptr)
		return o << "(NULL)";
	
	char buf[64];
	strftime(buf, 63, "%f %T", t);
	return o << buf;
}

std::ostream& operator<<(std::ostream& o, const pEp_identity* ident)
{
	if(ident==nullptr)
		return o << "(NULL)";
	
	o << "address=" << S(ident->address) << ", fpt=" << S(ident->fpr) << ", uid=" << S(ident->user_id) << ", name=" << S(ident->username);
	return o;
}

std::ostream& operator<<(std::ostream& o, const identity_list* ident)
{
	if(ident==nullptr)
		return o << "(NULL)";
	
	o << identity_list_length(ident) << " elements:\n";
	while(ident)
	{
		o << "\t\t" << ident->ident << "\n";
		ident = ident->next;
	}
	return o;
}

std::ostream& operator<<(std::ostream& o, const stringlist_t* sl)
{
	if(sl==nullptr)
		return o << "(NULL)";
	
	o << stringlist_length(sl) << " elements:";
	while(sl)
	{
		o << "  " << S(sl->value);;
		sl = sl->next;
	}
	return o;
}

std::ostream& operator<<(std::ostream& o, const stringpair_list_t* spl)
{
	if(spl==nullptr)
		return o << "(NULL)";
	
	o << stringpair_list_length(spl) << " elements:\n";
	while(spl)
	{
		o << "\t\t" << S(spl->value->key) << " -> " << S(spl->value->value) << "\n";
		spl = spl->next;
	}
	return o;
}



void dump_message(const message* msg)
{
	if(msg==nullptr)
	{
		std::cout << "Message: NULL!\n";
		return;
	}
	
	std::cerr << "Message:\n";
	std::cerr << "\tdir       : " <<   msg->dir << "\n";
	std::cerr << "\tid        : " << S(msg->id) << "\n";
	std::cerr 	<< "\tshortmsg  : " << S(msg->shortmsg) << "\n";
	std::cerr 	<< "\tlongmsg   : " << S(msg->longmsg) << "\n";
	std::cerr 	<< "\tlongmsg_f : " << S(msg->longmsg_formatted) << "\n";
	std::cerr 	<< "\tattachmen : " << msg->attachments << "\n";
	std::cerr 	<< "\tsent      : " << msg->sent << "\n";
	std::cerr 	<< "\trecv      : " << msg->recv << "\n";
		
	std::cerr 	<< "\tfrom      : " << msg->from << "\n";
	std::cerr 	<< "\tto        : " << msg->to << "\n";
	std::cerr 	<< "\trecv_by   : " << msg->recv_by << "\n";
		
	std::cerr 	<< "\tcc        : " << msg->cc << "\n";
	std::cerr 	<< "\tbcc       : " << msg->bcc << "\n";
	std::cerr 	<< "\treply_to  : " << msg->reply_to << "\n";
	std::cerr 	<< "\tin_reply_t: " << msg->in_reply_to << "\n";
		
		// TODO: refering_msg_ref
	std::cerr 	<< "\treferences: " << msg->references << "\n";
		// TODO: refered_by
		
	std::cerr 	<< "\tkeywords  : " << msg->keywords << "\n";
	std::cerr 	<< "\tcomments  : " << S(msg->comments) << "\n";
	std::cerr 	<< "\topt_fields: " << msg->opt_fields << "\n";
	std::cerr 	<< "\tenc_format: " << msg->enc_format << "\n";
	std::cerr 	<< std::endl;
}


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
	
	std::cerr << "status: " << pEp::status_to_string(status) << '\n';
	std::cerr << "has_pEp_msg: " << (has_pEp_msg?"true":"false") << '\n';
	dump_message(msg);
	
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
