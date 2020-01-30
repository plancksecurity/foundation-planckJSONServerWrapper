#include "websocket.hh"

namespace ws
{

	namespace
	{
		typedef unsigned char UC;
		
		void xor_data(std::string& data, const unsigned char* mask)
		{
			for(unsigned u=0; u<data.size(); ++u)
			{
				data[u] ^= mask[u % 4];
			}
		}
		
		// add length as 8 octet value
		void add_length8(std::string& data, size_t length)
		{
			data += char(127<<1); // skip LSB, it is always 0.
			for(int u=7*8; u>=0; u-=8)
			{
				data += char( (length >> u) & 0xFF );
			}
		}
		
		unsigned char fetch(std::istream& is)
		{
			const int c = is.get();
			if(c == EOF || !is)
			{
				throw std::runtime_error("Unexpected end of input stream");
			}
			return UC(c);
		}
		
		void fetch_mask(std::istream& is, unsigned char* mask)
		{
			*mask++ = fetch(is);
			*mask++ = fetch(is);
			*mask++ = fetch(is);
			*mask++ = fetch(is);
		};
	}


std::string encode_frame(const std::string& payload)
{
	const unsigned len_len = payload.length() < 126 ? 1 : (payload.length()<=0xFFFF ? 2 : 8 );
	std::string ret;
	ret.reserve( 1 + len_len + payload.length() );  // Opcode, Length, Data
	ret += char( (unsigned(Opcode::Binary) << 4) + 1); // FIN bit always set
	switch(len_len)
	{
		case 1 : ret += char( payload.length() << 1); // skip LSB, it is always 0.
		         break;
		case 2 : ret += char(126<<1);
		         ret += char( payload.length() >> 8 );
		         ret += char( payload.length() & 0xFF );
		         break;
		case 8 : add_length8( ret, payload.length() );
		         break;
		default:
			throw std::logic_error("Illegal len_len value of " + std::to_string(len_len) + ". Payload.length=" + std::to_string(payload.length()));
	}
	
	ret += payload;
	
	return ret;
}


// accepts only XOR-masked payload, because we are the "server"
Frame decode_frame(std::istream& is)
{
	unsigned char opcode = fetch(is);
	Frame f{Opcode(opcode>>4), bool(opcode & 1), std::string{}};
	
	unsigned payload_len = fetch(is);
	const bool is_masked = payload_len & 1;
	if(is_masked == false)
	{
		return Frame{Opcode::Illegal, true, "Must be masked"};
	}
	
	unsigned char mask[8];
	
	// shift-out the LSB:
	payload_len = payload_len >> 1;
	
	if(payload_len==126)
	{
		payload_len = fetch(is) * 256 + fetch(is);
	}else if(payload_len==127)
	{
		payload_len = 0;
		for(unsigned octet=0; octet<8; ++octet)
		{
			payload_len = payload_len*256 + fetch(is);
		}
	}
	
	fetch_mask(is, mask);
	f.data.resize(payload_len);
	is.read(&f.data[0], f.data.size());  // TODO: C++17 allows f.data.data(), because it is non-const
	xor_data(f.data, mask);
	return f;
}


}
