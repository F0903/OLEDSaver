module;
#include <string>
#include <sstream>
#include <Windows.h>
export module StringUtils;

export
template<auto length = 256>
std::string ConvertWString(const std::wstring& str) {
	char buf[length]{};
	if (str.length() >= length) throw std::exception("convertWString input too large!");
	const auto count = WideCharToMultiByte(CP_UTF8, WC_NO_BEST_FIT_CHARS | WC_COMPOSITECHECK | WC_DEFAULTCHAR, str.c_str(), str.length(), buf, sizeof(buf) * sizeof(char), NULL, NULL);
	auto newStr = std::string(buf, count); 
	return newStr;
}

export
template<auto length = 256>
std::wstring ConvertString(const std::string& str) {
	wchar_t buf[length]{};
	if (str.length() >= length) throw std::exception("convertString input too large!");
	const auto count = MultiByteToWideChar(CP_UTF8, MB_COMPOSITE, str.c_str(), str.length(), buf, sizeof(buf) * sizeof(wchar_t));
	auto newStr = std::wstring(buf, count); 
	return newStr;
}