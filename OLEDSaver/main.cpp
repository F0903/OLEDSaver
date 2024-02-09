#include <Windows.h>
#include <string> 
#include <exception>
#include <iostream>
#include "MacroUtils.h"
#include "shaders/out/PixelShader.h"
#include "shaders/out/VertexShader.h" 

import Window;
import ErrorHandling; 
import Shader;
import D3D11Renderer;
import StringUtils; 

#pragma warning(push)
#pragma warning(disable : 4297) // WinMain is marked noexcept by default so this will make VS shut up about it.
#pragma warning(disable : 4100) // For unused parameters.
int WINAPI WinMain(_In_ HINSTANCE instance, _In_opt_ HINSTANCE prevInstance, _In_ PSTR cmdLine, _In_ int cmdShow) {
	ASSERT(CoInitializeEx(nullptr, COINIT_MULTITHREADED));

	Window* window = nullptr;
	D3D11Renderer* renderer = nullptr;
	try {
		window = new Window(instance, L"OLEDSaver", Window::Style::Fullscreen);
		window->Update();
		window->Show();

		renderer = new D3D11Renderer(*window);
		renderer->LoadShadersParallel({
			{{VertexShader_code}, ShaderType::Vertex},
			{{PixelShader_code}, ShaderType::Pixel}
		}); 

		auto& vertexShader = renderer->GetLoadedVertexShader(0);
		auto& pixelShader = renderer->GetLoadedPixelShader(0);
		vertexShader.SetActive();
		pixelShader.SetActive();
		
		renderer->CreateFullscreenRect();
		window->SetCursorVisibility(false);

		MSG message{0};
		PeekMessage(&message, NULL, 0, 0, PM_NOREMOVE);
		bool gotMsg = false;
		while (message.message != WM_QUIT) {
			gotMsg = (PeekMessage(&message, NULL, 0, 0, PM_REMOVE) != 0);
			if (gotMsg) {
				TranslateMessage(&message);
				DispatchMessage(&message);
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
	CoUninitialize();
}
#pragma warning(pop)
