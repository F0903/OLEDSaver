#include <string> 
#include <Windows.h>
#include <exception> 
#include <codecvt>
#include "MacroUtils.h"

import Window;
import ErrorHandling;
import D3D11Renderer;
import StringUtils;

int WINAPI WinMain(_In_ HINSTANCE instance, _In_opt_ HINSTANCE prevInstance, _In_ PSTR cmdLine, _In_ int cmdShow) {
	ASSERT(CoInitializeEx(nullptr, COINIT_MULTITHREADED));

	Window* window = nullptr;
	D3D11Renderer* renderer = nullptr; 
	try {
		window = new Window(instance, L"OLEDSaver", Window::Style::Fullscreen);
		window->update();
		window->show();

		renderer = new D3D11Renderer(*window);

		MSG message{0};
		PeekMessage(&message, NULL, 0, 0, PM_NOREMOVE);
		bool gotMsg = false;

		while (message.message != WM_QUIT) {
			gotMsg = (PeekMessage(&message, NULL, 0, 0, PM_REMOVE) != 0);
			if (gotMsg) {
				TranslateMessage(&message);
				DispatchMessage(&message);
				continue;
			}
			//renderer.update();
			renderer->DrawFullscreenRect();
			//device.present();
		}
	}
	catch (std::exception& ex) {
		auto msgResult = GetLastErrorMessage();
		if (msgResult.HasValue()) {
			auto msg = msgResult.Unwrap();
			auto exMsg = ConvertString(ex.what());
			msg.append(L"\n");
			msg.append(exMsg);
			ErrorPopUp(msg);
		}
		else {
			ErrorPopUp(ConvertString(ex.what()));
		}
	}
	delete window;
	delete renderer;
}