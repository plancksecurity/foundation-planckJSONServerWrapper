#ifndef JSON_ADAPTER_NFC_HH
#define JSON_ADAPTER_NFC_HH

#include <string>
#include <stdexcept>

enum class IsNFC
{
	No=0,    // contains a character that cannot occur in NFC
	Maybe=1, // contains a character that is only allowed in certain positions in NFC
	Yes=2    // contains no invalid or partially valid character
};


class illegal_utf8 : public std::runtime_error
{
public:
	illegal_utf8(const std::string& s, unsigned position, const char* reason);
protected:
	explicit illegal_utf8(const std::string& message);
};


// return No or Maybe, if at least one character with NFC_Quickcheck class is "No" or "Maybe"
// might throw illegal_utf8 exception
IsNFC isNFC_quick_check(const std::string& s);

// runs first quick check and a deep test if quick check returns "Maybe".
bool isNFC(const std::string& s);

// converts a C++ string (in UTF-8) into NFC form
// s is ''moved'' to the return value if possible so no copy is done here.
std::string toNFC(std::string s);

#endif  // JSON_ADAPTER_NFC_HH
