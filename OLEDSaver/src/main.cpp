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
int WINAPI WinMain(_In_ HINSTANCE instance, _In_opt_ HINSTANCE prevInstance, _In_ PSTR cmdLine, _In_ int cmdShow) {
	Window* window = nullptr;
	try {
		ASSERT(CoInitializeEx(nullptr, COINIT_MULTITHREADED));

		window = new Window(instance, L"OLEDSaver", Window::Style::Fullscreen);
		auto renderer = D3D11Renderer(*window);
		renderer.LoadShadersParallel({
			{DefaultVertexShader_code, ShaderType::Vertex},
			{DefaultPixelShader_code, ShaderType::Pixel},
		});

		window->Update();
		window->Show();

		auto effect = DefaultShutdownEffect(*window, renderer, 1.0f);
		effect.Initialize();

		MSG message{0};
		PeekMessage(&message, NULL, 0, 0, PM_NOREMOVE);
		bool gotMsg = false;

		using namespace std::chrono_literals;
		Duration<std::nano> deltaTime = 0;
		Duration<std::milli> idleSleepTime = 1000ms;
		bool sleptFrame = false;
		bool poweredOff = false;
		while (message.message != WM_QUIT) {
			const auto frameStart = Timepoint();
			gotMsg = (PeekMessage(&message, NULL, 0, 0, PM_REMOVE) != 0);
			if (gotMsg) {
				TranslateMessage(&message);
				DispatchMessage(&message);
			}

			if ((sleptFrame = effect.DrawEffect(deltaTime, frameStart)) == true) {
				poweredOff = !effect.GetPoweringOnState();
				std::this_thread::sleep_for(1s);
			}
			const auto frameEnd = Timepoint();
			deltaTime = frameEnd - frameStart;
			if(sleptFrame) deltaTime -= idleSleepTime;
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
	delete window;
}
#pragma warning(pop)
