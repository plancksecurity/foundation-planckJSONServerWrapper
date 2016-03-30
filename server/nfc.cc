// converts a C++ string into NFC form

#include "nfc.hh"
#include <cstdint>


namespace
{

	class cont_without_start
	{
	public:
		cont_without_start(uint8_t u) : octet(u) {}
		uint8_t octet;
	};


	class overlong_sequence
	{
	public:
		overlong_sequence(uint8_t u) : octet(u) {}
		uint8_t octet;
	};

	class unexpected_end
	{
	public:
		unexpected_end(uint8_t u) : octet(u) {}
		uint8_t octet;
	};
	
	class no_unicode
	{
	public:
		no_unicode(uint8_t u) : octet(u) {}
		uint8_t octet;
	};


	std::string escape(const std::string& s)
	{
		std::string ret; ret.reserve(s.size() + 16 );
		for(char c : s)
		{
			const uint8_t u = c;
			if(u>=32 && u<=126)
			{
				ret += c;
			}else{
				char buf[16];
				snprintf(buf,15, "«%02x»", u );
				ret += buf;
			}
		}
		return ret;
	}

	uint32_t getUni(const char*& c, const char* end)
	{
		while(c<end)
		{
			const uint8_t u = uint8_t(*c);
			switch(u)
			{
				case 0x00 ... 0x7f : return u;
				case 0x80 ... 0xBF : throw cont_without_start(u);
				case 0xC0 ... 0xC1 : throw overlong_sequence(u);
				case 0xC2 ... 0xDF : // 2 octet sequence
					{
						++c;
						if(c==end) throw unexpected_end(u);
						const uint8_t uu = uint8_t(*c);
						if((uu & 0xC0) != 0x80)
						{
							throw unexpected_end(uu);
						}
						return  ((u & 0x1F) << 6) + (uu & 0x3F);
					}
				case 0xE0 ... 0xEF :  // 3 octet sequence
					{
						++c;
						if(c==end) throw unexpected_end(u);
						const uint8_t uu = uint8_t(*c);
						if((uu & 0xC0) != 0x80)
						{
							throw unexpected_end(uu);
						}
						++c;
						if(c==end) throw unexpected_end(uu);
						const uint8_t uuu = uint8_t(*c);
						if((uuu & 0xC0) != 0x80)
						{
							throw unexpected_end(uuu);
						}
						
						const uint32_t ret = ((u & 0xF) << 12) + ((uu & 0x3F)<<6) + (uuu & 0x3F);
						if(ret<0x800) throw overlong_sequence(u);
						return ret;
					}
				case 0xF0 ... 0xF4 :  // 4 octet sequence
					{
						++c;
						if(c==end) throw unexpected_end(u);
						const uint8_t uu = uint8_t(*c);
						if((uu & 0xC0) != 0x80)
						{
							throw unexpected_end(uu);
						}
						++c;
						if(c==end) throw unexpected_end(uu);
						const uint8_t uuu = uint8_t(*c);
						if((uuu & 0xC0) != 0x80)
						{
							throw unexpected_end(uuu);
						}
						++c;
						if(c==end) throw unexpected_end(uuu);
						const uint8_t uuuu = uint8_t(*c);
						if((uuuu & 0xC0) != 0x80)
						{
							throw unexpected_end(uuuu);
						}
						
						const uint32_t ret = ((u & 0xF) << 18) + ((uu & 0x3F)<<12) + ((uuu & 0x3F)<<6) + (uuuu & 0x3F);
						if(ret<0x10000) throw overlong_sequence(u);
						if(ret>0x10FFFF) throw no_unicode(u);
						return ret;
					}
				default:
					throw no_unicode(u);
			}
			
		}
		
		throw unexpected_end(-1);
	}

} // end of anonymous namespace


illegal_utf8::illegal_utf8( const std::string& s, unsigned position)
: std::runtime_error( "Illegal UTF-8 string \"" + escape(s) + "\" at position " + std::to_string(position) + "." )
{}


illegal_utf8::illegal_utf8( const std::string& msg )
: std::runtime_error( msg )
{}


bool isNFC(const std::string& s)
{
	const char* begin = s.data();
	const char* const end = s.data() + s.size();
	while(begin<end)
	{
		const uint32_t u = getUni(begin, end);
		if(u>=0x300 && u<0x30A) return false; // That's bullshit. Use a better algorithm!
		++begin;
	}
	return true;
}

// s is ''moved'' to the return value if possible so no copy is done here.
std::string toNFC(std::string s)
{
	if(isNFC(s))
		return std::move(s);
	
	std::string ret;
	
	// TODO:
	
	return ret;
}
