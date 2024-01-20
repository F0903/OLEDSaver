module;
#include <string> 
#include <Windows.h>
export module ErrorHandling;

export void errorPopUp(const std::wstring& msg) {
	MessageBox(NULL, msg.c_str(), L"Error!", MB_OK);
}

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