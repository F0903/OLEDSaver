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
import ShutdownEffect;
import ShutdownEffect.DefaultShutdownEffect;
import Time;

void RenderThread(Window* window, D3D11Renderer* renderer) {
	auto effect = DefaultShutdownEffect(*window, *renderer, 1.0f);
	effect.PowerOff();

	while(!window->IsClosed()) {
		const auto frameStart = Timepoint();
		effect.Update(frameStart);
	}
}

#pragma warning(push)
#pragma warning(disable : 4297) // WinMain is marked noexcept by default so this will make VS shut up about it.
#pragma warning(disable : 4100) // For unused parameters.
int WINAPI WinMain(_In_ HINSTANCE instance, _In_opt_ HINSTANCE prevInstance, _In_ PSTR cmdLine, _In_ int cmdShow)
try {
	ASSERT(CoInitializeEx(nullptr, COINIT_MULTITHREADED));
	auto window = Window(instance, L"OLEDSaver", Window::Style::Fullscreen);
	window.SetCursorVisibility(false);
	auto renderer = D3D11Renderer(window);
	renderer.LoadShadersParallel({
		{DefaultVertexShader_code, ShaderType::Vertex},
		{DefaultPixelShader_code, ShaderType::Pixel},
	});

	std::thread renderThread(&RenderThread, &window, &renderer);

	MSG message{0};
	while (!window.IsClosed()) {
		bool gotMsg = (GetMessage(&message, NULL, 0, 0) != 0);
		if (gotMsg) {
			TranslateMessage(&message);
			DispatchMessage(&message);
		}
	}

	renderThread.join();
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
