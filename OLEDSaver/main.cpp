#include <string> 
#include <Windows.h>
#include <exception> 
#include <codecvt>

import Window;
import ErrorHandling;
import D2D1Renderer;
import StringUtils;

int WINAPI WinMain(_In_ HINSTANCE instance, _In_opt_ HINSTANCE prevInstance, _In_ PSTR cmdLine, _In_ int cmdShow) {
	Window* window = nullptr;
	D2D1Renderer* renderer = nullptr; 
	try {
		window = new Window(instance, L"OLEDSaver", Window::Style::Fullscreen);
		window->update();
		window->show();

		renderer = new D2D1Renderer(*window);

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
	if (window) delete window;
	if (window) delete renderer;
}