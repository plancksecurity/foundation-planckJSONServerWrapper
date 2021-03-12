#include "base64.hh"
#include <stdint.h>
#include <stdexcept>

namespace
{

#define __ (-1) // invalid char -> exception!
#define SP (-2) // space char -> ignore
#define EQ (-3) // '=' char -> special handling of EOF

	const char* const b64c = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	const int8_t values[256] = {
			//   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
			__, __, __, __, __, __, __, __, __, SP, SP, __, SP, SP, __, __,  // 0x00 .. 0x0F
			__, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __,  // 0x10 .. 0x1F
			SP, __, __, __, __, __, __, __, __, __, __, 62, __, __, __, 63,  // 0x20 .. 0x2F
			52, 53, 54, 55, 56, 57, 58, 59, 60, 61, __, __, __, EQ, __, __,  // 0x30 .. 0x3F   0x3D = '='
			__,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,  // 0x40 .. 0x4F
			15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, __, __, __, __, __,  // 0x50 .. 0x5F
			__, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,  // 0x60 .. 0x6F
			41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, __, __, __, __, __,  // 0x70 .. 0x7F
			__, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __,  // 0x80 .. 0x8F
			__, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __,  // 0x90 .. 0x9F
			__, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __,  // 0xA0 .. 0xAF
			__, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __,  // 0xB0 .. 0xBF
			__, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __,  // 0xC0 .. 0xCF
			__, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __,  // 0xD0 .. 0xDF
			__, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __,  // 0xE0 .. 0xEF
			__, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __,  // 0xF0 .. 0xFF
		};
	
	struct IllegalCharacter
	{
		char c;
	};
	
	unsigned fetch(const char*& s, const char* end)
	{
		while( s < end)
		{
			const int8_t sc =  values[ uint8_t(*s) ];
			if(sc==-1) throw IllegalCharacter{*s};
			++s;
			if(sc>=0)
			{
				return uint8_t(sc);
			}else{
				if(sc==EQ) { return 255; }
			}
		}
		
		return 255;
	}

} // end of anonymous namespace


// decodes base64-encoded 'input', skip whitespaces, throw if illegal character found in string
std::string base64_decode(const std::string& input)
try{
	std::string ret;
	ret.reserve( (input.size()+3)/4 * 3 );

	const char*       c   = input.data();
	const char* const end = c + input.size();
	
	uint8_t  u0, u1, u2, u3=0;
	
	while(c < end)
	{
		u0 = fetch(c,end);
		u1 = fetch(c,end);
		u2 = fetch(c,end);
		u3 = fetch(c,end);
		
		if(u1!=255) { ret += char( (u0 << 2) | (u1 >> 4) ); }
		if(u2!=255) { ret += char( (u1 << 4) | (u2 >> 2) ); }
		if(u3!=255) { ret += char( (u2 << 6) | (u3     ) ); }
	}

	return ret;
}
catch(const IllegalCharacter& ic)
{
	throw std::runtime_error("Illegal character (" + std::to_string( int(ic.c) ) + ") in base64-encoded string \"" + input + "\"" );
}


std::string base64_encode(const std::string& input)
{
	typedef uint8_t U8;

	std::string ret;
	ret.reserve( (input.size()+2)/3 * 4 );
	
	const unsigned triples = input.size()/3;
	const char* s = input.data();
	for(unsigned q=0; q<triples; ++q, s+=3)
	{
		const uint32_t u = U8(s[0])*65536 + U8(s[1])*256 + U8(s[2]);
		ret += b64c[ (u>>18) & 63 ];
		ret += b64c[ (u>>12) & 63 ];
		ret += b64c[ (u>> 6) & 63 ];
		ret += b64c[ (u    ) & 63 ];
	}
	
	switch(input.size() - triples*3)
	{
		case 2 :
			{
				const uint32_t u = U8(s[0])*65536 + U8(s[1])*256;
				ret += b64c[ (u>>18) & 63 ];
				ret += b64c[ (u>>12) & 63 ];
				ret += b64c[ (u>> 6) & 63 ];
				ret += '=';
				return ret;
			}

		case 1 :
			{
				const uint32_t u = U8(s[0])*65536;
				ret += b64c[ (u>>18) & 63 ];
				ret += b64c[ (u>>12) & 63 ];
				ret += '=';
				ret += '=';
				return ret;
			}
		case 0: return ret;
		default : throw std::logic_error("Internal error in base64_encode()!");
	}
}
