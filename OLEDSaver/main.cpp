#include <string> 
#include <Windows.h>
#include <exception> 

import Window;
import ErrorHandling;


int WINAPI WinMain(_In_ HINSTANCE instance, _In_opt_ HINSTANCE prevInstance, _In_ PSTR cmdLine, _In_ int cmdShow) {
	try {
		Window window = Window::Fullscreen(instance, L"OLEDSaver");
		window.update();
		MSG message{0};
		while (GetMessage(&message, window.getHandle(), 0, 0)) {
			TranslateMessage(&message);
			DispatchMessage(&message);
		}
	}
	catch (std::exception& ex) {
		const auto msg = getLastErrorMessage();
		errorPopUp(msg);
	}
}