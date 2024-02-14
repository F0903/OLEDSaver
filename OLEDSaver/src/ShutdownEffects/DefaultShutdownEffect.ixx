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
	DefaultShutdownEffect(D3D11Renderer& renderer, float durationSeconds) : ShutdownEffect(
		renderer,
		renderer.GetLoadedVertexShader(0), // Use default vertex shader (assume it has already been loaded)
		static_cast<PixelShader&>(renderer.LoadShader(DefaultPixelShader_code, ShaderType::Pixel)),
		durationSeconds
	) {
	}

	void Initialize() override {
		ShutdownEffect::Initialize();
		pixelShader.InitConstantBuffer<ConstantBuffer>({effectTime, durationSeconds}, ConstantBufferSlot::Custom);
	}

	bool DrawEffect(const Duration<std::nano>& deltaTime, const Timepoint& frameStartTime) {
		const auto stepCast = deltaTime.Cast<std::milli>();
		const auto stepAmount = (static_cast<float>(stepCast.GetCount<double>() / 1000.0)) / durationSeconds;

		if (poweringOn) {
			effectTime -= stepAmount;
			if (effectTime <= 0.0f) {
				return true;
			}
		}
		else {
			effectTime += stepAmount;
			if (effectTime >= 1.0f) {
				return true;
			}
		}

		pixelShader.SetConstantBuffer<ConstantBuffer>({effectTime, durationSeconds}, ConstantBufferSlot::Custom);
		renderer.Draw(pixelShader, frameStartTime);
		return false;
	}
};