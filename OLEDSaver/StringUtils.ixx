module;
#include <string>
#include <sstream>
#include <Windows.h>
export module StringUtils;

export std::string convertWString(const std::wstring& str) {
	char buf[256];
	if (str.length() >= 256) throw std::exception("convertWString input too large!");
	const auto count = WideCharToMultiByte(CP_UTF8, WC_NO_BEST_FIT_CHARS | WC_COMPOSITECHECK | WC_DEFAULTCHAR, str.c_str(), str.length(), buf, sizeof(buf) * sizeof(char), NULL, NULL);
	auto newStr = std::string(buf, count); 
	return newStr;
}

export std::wstring convertString(const std::string& str) {
	wchar_t buf[256];
	if (str.length() >= 256) throw std::exception("convertString input too large!");
	const auto count = MultiByteToWideChar(CP_UTF8, MB_COMPOSITE, str.c_str(), str.length(), buf, sizeof(buf) * sizeof(wchar_t));
	auto newStr = std::wstring(buf, count); 
	return newStr;
}