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
			//renderer.render();
			//device.present();
		}
	}
	catch (std::exception& ex) {
		const auto msg = getLastErrorMessage();
		errorPopUp(msg);
	}
}