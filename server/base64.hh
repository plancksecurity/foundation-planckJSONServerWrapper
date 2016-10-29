#ifndef JSON_ADAPTER_BASE64_HH
#define JSON_ADAPTER_BASE64_HH

#include <string>

// decodes base64-encoded 'input', skip whitespaces, throw std::runtime_error if an illegal character found in string
std::string base64_decode(const std::string& input);

// encodes base64-encoded 'input', without any whitespaces nor linebreaks
std::string base64_encode(const std::string& input);

#endif // JSON_ADAPTER_BASE64_HH
