module;
#include <chrono> //needed due to bug
#include <thread> //needed due to bug
#include <Windows.h>
#include "../../shaders/out/DefaultPixelShader.h"
export module DefaultShutdownEffect;

import ShutdownEffect;
import D3D11Renderer;
import Shader.PixelShader;
import Shader.VertexShader;
import Time;
import Window;

export class DefaultShutdownEffect : public ShutdownEffect
{
#pragma warning(push)
#pragma warning(disable : 4324)
	__declspec(align(16)) struct ConstantBuffer
	{
		float effectTime;
		float duration;
	};
#pragma warning(pop)

	Window& window;

	bool initPowerOffState = false;

public:
	DefaultShutdownEffect(Window& window, D3D11Renderer& renderer, float durationSeconds) : ShutdownEffect(
		renderer,
		renderer.GetLoadedVertexShader(0), // Use default vertex shader (assume it has already been loaded)
		static_cast<PixelShader&>(renderer.LoadShader(DefaultPixelShader_code, ShaderType::Pixel)),
		durationSeconds
	), window(window) {
	}

	void Initialize() override {
		ShutdownEffect::Initialize();
		pixelShader.InitConstantBuffer<ConstantBuffer>({effectTime, durationSeconds}, ConstantBufferSlot::Custom);
		window.SetOnInputCallback([this]() {
			if (GetIsDone()) {
				SetPowerOn();
				window.SetCursorVisibility(true);
			}
		});
	}

	bool DrawEffect(const Duration<std::nano>& deltaTime, const Timepoint& frameStartTime) {
		const auto stepCast = deltaTime.Cast<std::milli>();
		const auto stepAmount = (static_cast<float>(stepCast.GetCount<double>() / 1000.0)) / durationSeconds;

		if (poweringOn) {
			effectTime -= stepAmount;
			if (effectTime <= 0.0f) {
				initPowerOffState = false;
				isDone = true;
				return true;
			}
		}
		else {
			if (!initPowerOffState) {
				window.SetCursorVisibility(false);
				initPowerOffState = true;
			}
			effectTime += stepAmount;
			if (effectTime >= 1.0f) {
				isDone = true;
				return true;
			}
		}

		pixelShader.SetConstantBuffer<ConstantBuffer>({effectTime, durationSeconds}, ConstantBufferSlot::Custom);
		renderer.Draw(pixelShader, frameStartTime);
		return false;
	}
};