// converts a C++ string into NFC form

#include "nfc.hh"
#include <cstdint>
#include <set>

#include "nfc_sets.hh"

namespace
{
	class utf8_exception
	{
	public:
		utf8_exception(uint8_t u) : octet(u) {}
		virtual ~utf8_exception() = default;
		virtual const char* reason() const = 0;
		uint8_t octet;
	};


	class cont_without_start : public utf8_exception
	{
	public:
		cont_without_start(uint8_t u) : utf8_exception(u) {}
		const char* reason() const override { return "Continuation octet without start octet"; }
	};


	class overlong_sequence : public utf8_exception
	{
	public:
		overlong_sequence(uint8_t u) : utf8_exception(u) {}
		const char* reason() const override { return "Overlong sequence"; }
	};


	class unexpected_end : public utf8_exception
	{
	public:
		unexpected_end(uint8_t u) : utf8_exception(u) {}
		const char* reason() const override { return "Unexpected end of string"; }
	};
	
	class no_unicode : public utf8_exception
	{
	public:
		no_unicode(uint8_t u) : utf8_exception(u) {}
		const char* reason() const override { return "Octet illegal in UTF-8"; }
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

} // end of anonymous namespace

uint32_t parseUtf8(const char*& c, const char* end)
{
	while(c<end)
	{
		const uint8_t u = uint8_t(*c);
		
		if (u<=0x7f)
		{
			return u;
		} else if (u<=0xBF)
		{
			throw cont_without_start(u);
		} else if (u<=0xC1)
		{
			throw overlong_sequence(u);
		} else if (u<=0xDF)  // 2 octet sequence
		{
			++c;
			if(c==end) throw unexpected_end(u);
			const uint8_t uu = uint8_t(*c);
			if((uu & 0xC0) != 0x80)
			{
				throw unexpected_end(uu);
			}
			return  ((u & 0x1F) << 6) + (uu & 0x3F);
		} else if (u<=0xEF)  // 3 octet sequence
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
		} else if (u<=0xF4)  // 4 octet sequence
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
		} else
		{
			throw no_unicode(u);
		}
	}
	
	throw unexpected_end(-1);
}



illegal_utf8::illegal_utf8( const std::string& s, unsigned position, const char* reason)
: std::runtime_error( "Illegal UTF-8 string \"" + escape(s) + "\" at position " + std::to_string(position) + ": " + reason  )
{}


illegal_utf8::illegal_utf8( const std::string& msg )
: std::runtime_error( msg )
{}


void assert_utf8(const std::string& s)
{
	const char* begin = s.data();
	const char* const end = s.data() + s.size();
	try
	{
		while(begin<end)
		{
			parseUtf8(begin, end); // ignore the output
			++begin;
		}
	}
	catch(const utf8_exception& e)
	{
		throw illegal_utf8(s, e.octet, e.reason());
	}
}


IsNFC isNFC_quick_check(const std::string& s)
{
	const char* begin = s.data();
	const char* const end = s.data() + s.size();
	try
	{
		while(begin<end)
		{
			const uint32_t u = parseUtf8(begin, end);
			if(NFC_No.count(u)) return IsNFC::No;
			if(NFC_Maybe.count(u)) return IsNFC::Maybe;
			++begin;
		}
	}
	catch(const utf8_exception& e)
	{
		throw illegal_utf8(s, e.octet, e.reason());
	}
	return IsNFC::Yes;
}


bool isNFC(const std::string& s)
{
	switch( isNFC_quick_check(s) )
	{
		case IsNFC::Yes : return true;
		case IsNFC::No  : return false;
		case IsNFC::Maybe:
			{
				throw std::logic_error("Deep NGC check is not yet implemented. Sorry.");
			}
	}
	
	throw -1; // could never happen, but compiler is too dumb to see this.
}


// s is ''moved'' to the return value if possible so no copy is done here.
std::string toNFC(std::string s)
{
	if(isNFC(s))
		return s;
	
	// TODO:
	throw std::logic_error("NFC normalization is necessary, but unimplemented. Sorry.");
}
