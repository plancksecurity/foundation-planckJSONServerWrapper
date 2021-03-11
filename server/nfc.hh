// This file is under GNU General Public License 3.0
// see LICENSE.txt

#ifndef JSON_ADAPTER_NFC_HH
#define JSON_ADAPTER_NFC_HH

#include "config.hh"  // to switch between std::string_view or boost::string_view.hh
#include <string>
#include <stdexcept>
#include <iosfwd>

enum class IsNFC
{
	No=0,    // contains a character that cannot occur in NFC
	Maybe=1, // contains a character that is only allowed in certain positions in NFC
	Yes=2    // contains no invalid or partially valid character
};

std::ostream& operator<<(std::ostream& o, IsNFC is_nfc);


class illegal_utf8 : public std::runtime_error
{
public:
	illegal_utf8(sv, unsigned position, const std::string& reason);
protected:
	explicit illegal_utf8(const std::string& message);
};


// scans the char sequences and parses UTF-8 sequences. Detect UTF-8 errors and throws exceptions.
uint32_t parseUtf8(const char*& c, const char* end);

// converts 'c' into a UTF-8 sequence and adds it to 'ret'
void toUtf8(const char32_t c, std::string& ret);

// throws illegal_utf8 exception if s is not valid UTF-8
void assert_utf8(sv s);

// creates an NFD u32string from UTF-8 input string s
std::u32string fromUtf8_decompose(sv s);

// convert NFD to NFC
std::u32string createNFC(std::u32string nfd_string);

// return No or Maybe, if at least one character with NFC_Quickcheck class is "No" or "Maybe"
// might throw illegal_utf8 exception
IsNFC isNFC_quick_check(sv s);

// runs first quick check and a deep test if quick check returns "Maybe".
bool isNFC(sv s);

// returns true if the sequence is valid UTF-8
bool isUtf8(const char* begin, const char* end);

// converts a C++ string (in UTF-8) into NFC form
// s is ''moved'' to the return value if possible so no copy is done here.
std::string toNFC(sv s);

#endif  // JSON_ADAPTER_NFC_HH
