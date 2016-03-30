// converts a C++ string into NFC form

#include <string>
#include <stdexcept>


class illegal_utf8 : public std::runtime_error
{
public:
	illegal_utf8(const std::string& s, unsigned position);
protected:
	explicit illegal_utf8(const std::string& message);
};


bool isNFC(const std::string& s);

// s is ''moved'' to the return value if possible so no copy is done here.
std::string toNFC(std::string s);