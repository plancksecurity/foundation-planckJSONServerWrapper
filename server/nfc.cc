// converts a C++ string into NFC form

#include "nfc.hh"
#include <cstdint>
#include <set>
#include <ostream>

#include "nfc_sets.hh"

namespace
{
	// unicode to hex string
	std::string u2h(unsigned u)
	{
		char buf[16] = {0};
		snprintf(buf, 15, "<U+%04X>", u );
		return buf;
	}

	// octet to hex string
	std::string o2h(uint8_t octet)
	{
		char buf[16] = {0};
		snprintf(buf, 15, "0x%02hhX", octet);
		return buf;
	}


	class utf8_exception
	{
	public:
		utf8_exception(uint8_t u) : octet(u) {}
		virtual ~utf8_exception() = default;
		virtual std::string reason() const = 0;
		uint8_t octet;
	};


	class cont_without_start : public utf8_exception
	{
	public:
		cont_without_start(uint8_t u) : utf8_exception(u) {}
		std::string reason() const override { return "Continuation octet " + o2h(octet) + " without start octet"; }
	};


	class overlong_sequence : public utf8_exception
	{
	public:
		overlong_sequence(uint8_t octet, unsigned u) : utf8_exception(octet), unicode(u) {}
		std::string reason() const override { return "Overlong sequence for " + u2h(unicode); }
		unsigned unicode;
	};


	class unexpected_end : public utf8_exception
	{
	public:
		unexpected_end(uint8_t u) : utf8_exception(u) {}
		std::string reason() const override { return "Unexpected end of string"; }
	};
	
	class surrogate : public utf8_exception
	{
	public:
		surrogate(uint8_t u, unsigned s) : utf8_exception(u), surr(s) {}
		std::string reason() const override { return "UTF-8-encoded UTF-16 surrogate " + u2h(surr) + " detected"; }
	private:
		unsigned surr;
	};

	class no_unicode : public utf8_exception
	{
	public:
		explicit no_unicode(uint8_t _octet) : utf8_exception(_octet) {}
		std::string reason() const override { return "Octet " + o2h(octet) + " is illegal in UTF-8"; }
	};

	class too_big : public utf8_exception
	{
	public:
		explicit too_big(uint8_t _octet, unsigned u) : utf8_exception(_octet), unicode(u) {}
		std::string reason() const override { return "Value " + u2h(unicode) + " is too big for Unicode"; }
		unsigned unicode;
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

	// returns the "CanonicalCombinincClass" of the given Unicode codpoint u
	unsigned canonicalClass(unsigned u)
	{
		const auto q = NFC_CombiningClass.find(u);
		if(q==NFC_CombiningClass.end())
		{
			return 0; // not found in map.
		}else{
			return q->second;
		}
	}
	
	std::pair<int,int> decompose(unsigned u)
	{
		const auto q = NFC_Decompose.find(u);
		if(q==NFC_Decompose.end())
		{
			return std::make_pair(-1, -1);
		}else{
			return q->second;
		}
	}
	
	std::u32string decompose_full(unsigned u)
	{
		const std::pair<int,int> d = decompose(u);
		if(d.first<0)
		{
			return std::u32string( 1, char32_t(u) );
		}else{
			if(d.second<0)
			{
				return decompose_full(d.first);
			}
		}
		return decompose_full(d.first) + decompose_full(d.second);
	}
	

	// according to Unicode Standard, clause D108:
	bool isReorderablePair(unsigned a, unsigned b)
	{
		const unsigned cca = canonicalClass(a);
		const unsigned ccb = canonicalClass(b);
		
		return (cca > ccb) && (ccb>0);
	}

	// Unicode standard requires bubble sort, for stability reasons?
	void canonicalOrdering(std::u32string& us)
	{
		if(us.size()<2)
			return;
		
		for(unsigned n=us.size(); n>1; --n)
		for(unsigned i=0; i<n-1; ++i)
		{
			char32_t& a = us[i];
			char32_t& b = us[i+1];
			if( isReorderablePair(a,b) )
			{
				std::swap(a,b);
			}
		}
	}

} // end of anonymous namespace




std::ostream& operator<<(std::ostream& o, IsNFC is_nfc)
{
	switch(is_nfc)
	{
		case IsNFC::No    : return o << "No";
		case IsNFC::Maybe : return o << "Maybe";
		case IsNFC::Yes   : return o << "Yes";
	}
	throw std::logic_error("Unknown value of IsNFC");
}


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
		} else if (u<=0xC1) // 0xC0, 0xC1 would form "overlong sequences" and are therefore always illegal in UTF-8
		{
			throw no_unicode(u);
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
			if(ret<0x800) throw overlong_sequence(u, ret);
			if(ret>=0xD800 && ret<=0xDFFF) throw surrogate(u, ret);
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
			if(ret<0x10000) throw overlong_sequence(u, ret);
			if(ret>0x10FFFF) throw too_big(u, ret);
			return ret;
		} else
		{
			throw no_unicode(u);
		}
	}
	
	throw unexpected_end(-1);
}



illegal_utf8::illegal_utf8( const std::string& s, unsigned position, const std::string& reason)
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
		throw illegal_utf8(s, begin - s.data(), e.reason());
	}
}


// creates a NFD string from s
std::u32string fromUtf8_decompose(const std::string& s)
{
	std::u32string u32s;
	u32s.reserve(s.size()*1.25);
	const char* begin = s.c_str();
	const char* end   = s.c_str() + s.size();
	for(; begin<end; ++begin)
	{
		unsigned u = parseUtf8(begin, end);
		u32s += decompose_full(u);
	}
	canonicalOrdering(u32s); // works inplace.
	return u32s;
}


IsNFC isNFC_quick_check(const std::string& s)
{
	const char* begin = s.data();
	const char* const end = s.data() + s.size();
	try
	{
		unsigned last_cc = 0;
		while(begin<end)
		{
			const uint32_t u = parseUtf8(begin, end);
			const unsigned cc = canonicalClass(u);
			if( (cc!=0) && (last_cc > cc) )
			{
				return IsNFC::No;
			}
			if(NFC_No.count(u)) return IsNFC::No;
			if(NFC_Maybe.count(u)) return IsNFC::Maybe;
			++begin;
			last_cc = cc;
		}
	}
	catch(const utf8_exception& e)
	{
		throw illegal_utf8(s, begin - s.data(), e.reason());
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
	if(isNFC_quick_check(s)==IsNFC::Yes)
		return s;
	
	// TODO:
	throw std::logic_error("NFC normalization is necessary, but unimplemented. Sorry.");
}
