module;
#include <string> 
#include <Windows.h>
#include <string>
export module ErrorHandling;

import StringUtils;

export const std::wstring getLastErrorMessage() {
	const auto code = GetLastError();
	constexpr int BUF_SIZE = 1024;
	wchar_t buffer[BUF_SIZE];
	const auto count = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, code, NULL, buffer, BUF_SIZE, NULL);
	if (count == 0) {
		throw std::exception("Could not format error message!");
	}
	return std::wstring(buffer);
}

export void errorPopUp(const std::wstring& msg) {   
	auto result = MessageBox(NULL, msg.c_str(), L"Error!", MB_OK | MB_SYSTEMMODAL);
	if (result == 0) {
		auto wideErrorMsg = getLastErrorMessage();
		auto errorMsg = convertWString(wideErrorMsg);
		auto msg = std::string("Could not create error modal!\n");
		msg.append(errorMsg);
		throw std::exception(msg.c_str());
	}
} 