#include <Windows.h>
#include <string> 
#include <exception>
#include <iostream>
#include <chrono> //needed due to bug
#include <thread> //needed due to bug
#include "MacroUtils.h" 
#include "../shaders/out/DefaultVertexShader.h"
#include "../shaders/out/DefaultPixelShader.h"

import ErrorHandling; 
import StringUtils; 
import Window;
import Shader;
import D3D11Renderer;
import DefaultShutdownEffect;
import Time;

#pragma warning(push)
#pragma warning(disable : 4297) // WinMain is marked noexcept by default so this will make VS shut up about it.
#pragma warning(disable : 4100) // For unused parameters.
int WINAPI WinMain(_In_ HINSTANCE instance, _In_opt_ HINSTANCE prevInstance, _In_ PSTR cmdLine, _In_ int cmdShow)
try {
	ASSERT(CoInitializeEx(nullptr, COINIT_MULTITHREADED));
	auto window = Window(instance, L"OLEDSaver", Window::Style::Fullscreen);;
	auto renderer = D3D11Renderer(window);
	renderer.LoadShadersParallel({
		{DefaultVertexShader_code, ShaderType::Vertex},
		{DefaultPixelShader_code, ShaderType::Pixel},
	});

	window.Update();
	window.Show();

	auto effect = DefaultShutdownEffect(window, renderer, 1.0f);
	effect.Initialize();

	MSG message{0};
	PeekMessage(&message, NULL, 0, 0, PM_NOREMOVE);
	bool gotMsg = false;

	while (message.message != WM_QUIT) {
		const auto frameStart = Timepoint();
		gotMsg = (PeekMessage(&message, NULL, 0, 0, PM_REMOVE) != 0);
		if (gotMsg) {
			TranslateMessage(&message);
			DispatchMessage(&message);
		}

		effect.Update(frameStart);
	}

	CoUninitialize();
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

#pragma warning(pop)
