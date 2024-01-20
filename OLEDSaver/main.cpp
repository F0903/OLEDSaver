#include <string> 
#include <Windows.h>

import Window;

static void errorPopUp(const std::wstring& msg) {
	MessageBox(NULL, msg.c_str(), L"Error!", MB_OK);
}

static const std::wstring getLastErrorMessage() {
	const auto code = GetLastError();
	constexpr int BUF_SIZE = 1024;
	wchar_t buffer[BUF_SIZE];
	const auto count = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, code, NULL, buffer, BUF_SIZE, NULL);
	if (count == 0) {
		throw std::exception("Could not format error message!");
	}
	return std::wstring(buffer);
}



int WINAPI WinMain(_In_ HINSTANCE instance, _In_opt_ HINSTANCE prevInstance, _In_ PSTR cmdLine, _In_ int cmdShow) { 
	try {
		Window window = Window::Fullscreen(instance, L"OLEDSaver");
		system("pause");
	}
	catch (std::exception& ex) {
		const auto msg = getLastErrorMessage();
		errorPopUp(msg);
	}
}