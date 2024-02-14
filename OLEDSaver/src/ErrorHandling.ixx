module;
#include <string> 
#include <Windows.h>
#include <string>
export module ErrorHandling;

import StringUtils;
import Optional;

export OptionalValue<const std::wstring> GetLastErrorMessage() {
	const auto code = GetLastError();
	if (code == 0) {
		return Optional::None<const std::wstring>();
	}
	constexpr int BUF_SIZE = 1024;
	wchar_t buffer[BUF_SIZE];
	const auto count = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, code, NULL, buffer, BUF_SIZE, NULL);
	if (count == 0) {
		throw std::exception("Could not format error message!");
	}
	return Optional::Value(std::wstring(buffer));
}

export void ErrorPopUp(const std::wstring& msg) {
	auto result = MessageBox(NULL, msg.c_str(), L"OLEDSaver Error!", MB_OK | MB_SYSTEMMODAL | MB_DEFAULT_DESKTOP_ONLY);
	if (result == 0) {
		auto wideErrorMsg = GetLastErrorMessage();
		auto errorMsg = wideErrorMsg.HasValue() ? ConvertWString(wideErrorMsg.Unwrap()) : "No error specified.";
		auto exMsg = std::string("Could not create error modal!\n");
		exMsg.append(errorMsg);
		throw std::exception(exMsg.c_str());
	}
} 