#pragma once
#include <cassert>
#include <string>
namespace utils {
	// ת16����
	unsigned char ToHex(unsigned char x);
	unsigned char FromHex(unsigned char x);
	std::string UrlEncode(const std::string& str);
	std::string UrlDecode(const std::string& str);
} // namespace utils

