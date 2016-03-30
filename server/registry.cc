#include "registry.hh"
#include <algorithm>
#include <iostream>

// mix the bits of 'value' into a less-
uint64_t joggle(uint64_t realm, uint64_t value)
{
	++value;
	value ^= realm;
	value *= 11400714819323198485ull; // golden ratio (0.618...) * 2^64
	value = (value>>19) | (value<<(64-19));  // bit rotate right by 19 bits
	return realm - value;
}


uint64_t unjoggle(uint64_t realm, uint64_t value)
{
	value = realm - value;
	value = (value<<19) | (value>>(64-19));  // bit rotate left by 19 bits
	value *= 17428512612931826493ull;  // inv_mod( 11400714819323198485, 2^64 );
	return (value ^ realm)-1;
}


namespace
{
	const char* const alphabet = "98765432aAbBcCdDeEfFgGhHiIjJkKlLmMnNoOpPqQrRsStTuUvVwWxXyYzZ";

	int8_t reverse[256];
	
	struct Alphabet
	{
		Alphabet()
		{
			std::fill(reverse, reverse+256, -1 );
			uint8_t u = 0;
			const char* a = alphabet;
			while(*a)
			{
				reverse[uint8_t(*a)] = u;
				++a;
				++u;
			}
			std::cout << "initialize " << int(u) << " elements of the reverse map.\n";
		}
	};

//	Alphabet static_initialize_reverse_array;
}


std::string base57(std::uint64_t x)
{
	char buffer[12] = "+.:-*^?-#~$";
	for(unsigned u=0; u<11; ++u)
	{
		buffer[u] = alphabet[ x % 57 ];
		x/=57;
	}
	
	return std::string(buffer, buffer+11);
}


std::uint64_t  unbase57(const std::string& s)
{
	static Alphabet nono;

	if(s.size() != 11)
		throw std::runtime_error("Invalid base57 string! Length=" + std::to_string(s.size()) + ". \"" + s + "\" is not a valid base62-encoded 64 bit integer");
	
	std::uint64_t ret = 0;
	
	std::uint64_t factor = 1;
	for(unsigned u=0; u<11; ++u)
	{
		int f = reverse[ uint8_t(s[u]) ];
		if(f<0)
			throw std::runtime_error("At position " + std::to_string(u) + ": Invalid char '" + std::to_string( int(s[u]) ) + "'");
		
		ret += f*factor;
		factor *= 57;
	}
	
	return ret;
}


unsigned test_joggle()
{
	for(unsigned u=0; u<99; ++u)
	{
		const uint64_t j = joggle( 0xCAFEBABE00000000, u );
		const std::string s = base57(j);
		std::cout << "# u=" << u << ", j=" << j << ", s=\"" << s << "\".\n";

		const uint64_t jj = joggle( 00, u );
		const std::string ss = base57(jj);
		std::cout << "#       j=" << jj << ", s=\"" << ss << "\".\n";
	}
	
	uint64_t counter = 0;
	
	for(unsigned realm = 100; realm < 10000000; realm = realm*1.123)
	for(unsigned u = 0; u < 99999; ++u)
	{
		++counter;
		const uint64_t j = joggle( realm, u );
		const uint64_t jj = unjoggle( realm, j );
		
		if( jj!=u )
		{
			std::cerr << "Joggle mismatch! realm=" << realm << ", u = " << u << ", j = " << j << ", jj = " << jj << ".\n";
			return 1;
		}
		
		const std::string s = base57(j);
		const uint64_t   us = unbase57(s);
		
		if( us != j )
		{
			std::cerr << "base57 mismatch! j = " << j << ", s = " << s << ", us = " << us << ".\n";
			return 2;
		}
		
		if( ((u*1111^0x777) + (realm*777^0xaaaa))%77777==0)
		{
			std::cout << "#" << counter << ": realm=" << realm << ", u=" << u << ", j=" << j << ", s=\"" << s << "\".\n";
		}
	}
	
	std::cout << counter << " values checked successfully. :-)\n";
	return 0;
}
