#include <string> 
#include <Windows.h>
#include <exception> 
#include <codecvt>
#include "MacroUtils.h"
#include "shaders/out/PixelShader.h"
#include "shaders/out/VertexShader.h" 

import Window;
import ErrorHandling; 
import Shader;
import D3D11Renderer;
import StringUtils; 

int WINAPI WinMain(_In_ HINSTANCE instance, _In_opt_ HINSTANCE prevInstance, _In_ PSTR cmdLine, _In_ int cmdShow) NOEXCEPT_RELEASE {
	ASSERT(CoInitializeEx(nullptr, COINIT_MULTITHREADED));

	Window* window = nullptr;
	D3D11Renderer* renderer = nullptr;
	try {
		window = new Window(instance, L"OLEDSaver", Window::Style::Fullscreen);
		window->Update();
		window->Show();

		renderer = new D3D11Renderer(*window);
		ShaderLoadInfo shaders[] = {
			{ShaderLoadType::RawCode, ShaderType::Vertex, {VertexShader_code}},
			{ShaderLoadType::RawCode, ShaderType::Pixel, {PixelShader_code}}
		};
		renderer->LoadShadersAsync(shaders);
		renderer->SetActiveLoadedVertexShader(0);
		renderer->SetActiveLoadedPixelShader(0);

		MSG message{0};
		PeekMessage(&message, NULL, 0, 0, PM_NOREMOVE);
		bool gotMsg = false;

		renderer->CreateFullscreenRect();
		while (message.message != WM_QUIT) {
			gotMsg = (PeekMessage(&message, NULL, 0, 0, PM_REMOVE) != 0);
			if (gotMsg) {
				TranslateMessage(&message);
				DispatchMessage(&message);
				continue;
			} 
			renderer->Draw(); 
		}
	}
	catch (std::exception& ex) {
		auto msgResult = GetLastErrorMessage();
		if (msgResult.HasValue()) {
			std::wstring msg = msgResult.Unwrap();
			const std::wstring exMsg = ConvertString(ex.what());
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