module;
#include <chrono> //needed due to bug
#include <thread> //needed due to bug
#include <Windows.h>
#include "../../shaders/out/DefaultPixelShader.h"
export module ShutdownEffect.DefaultShutdownEffect;

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

public:
	DefaultShutdownEffect(Window& window, D3D11Renderer& renderer, float durationSeconds) : ShutdownEffect(
		window,
		renderer,
		renderer.GetLoadedVertexShader(0), // Use default vertex shader (assume it has already been loaded)
		static_cast<PixelShader&>(renderer.LoadShader(DefaultPixelShader_code, ShaderType::Pixel)),
		durationSeconds
	) {
		pixelShader.InitConstantBuffer<ConstantBuffer>({effectTime, durationSeconds}, ConstantBufferSlot::Effect);
	}

	void OnInputReceived() override {
		if (!poweringOn) {
			PowerOn();
		}
	}

	void OnPowerOff() override {
		window.DisableMousePassthrough();
		window.Restore();
	}

	void OnPowerOn() override {
		window.EnableMousePassthrough();
	}

	bool DrawEffect(const Timepoint& frameStartTime) {
		const auto stepCast = deltaTime.Cast<std::milli>();
		const auto stepAmount = (static_cast<float>(stepCast.GetCount<double>() / 1000.0)) / durationSeconds;

		if (!poweringOff) {
			effectTime -= stepAmount;
			if (effectTime <= 0.0f) {
				window.Minimize();
				return true;
			}
		}
		else {
			effectTime += stepAmount;
			if (effectTime >= 1.0f) {
				return true;
			}
		}

		pixelShader.SetConstantBuffer<ConstantBuffer>({effectTime, durationSeconds}, ConstantBufferSlot::Effect);
		renderer.Draw(pixelShader, frameStartTime);
		return false;
	}
};