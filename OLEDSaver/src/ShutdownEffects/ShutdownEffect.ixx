module;
#include <chrono>
#include <thread>
#include "../MacroUtils.h"
export module ShutdownEffect;

import Window;
import D3D11Renderer;
import Shader.PixelShader;
import Shader.VertexShader;
import Time;

export class ShutdownEffect
{
protected:
	Window& window;

	D3D11Renderer& renderer;

	VertexShader& vertexShader;
	PixelShader& pixelShader;

	float durationSeconds;

	float effectTime = 0.0f;

	bool poweringOff = false;
	bool poweringOn = false;

	Duration<std::nano> deltaTime = 0;
	Timepoint lastInputTime;

	ShutdownEffect(Window& window, D3D11Renderer& renderer, VertexShader& vertexShader, PixelShader& pixelShader, float durationSeconds) : window(window), renderer(renderer), vertexShader(vertexShader), pixelShader(pixelShader), durationSeconds(durationSeconds) {
		vertexShader.SetActive();
		pixelShader.SetActive();
		renderer.CreateFullscreenRect();
		renderer.Initialize();
		window.OnKeyboardMouseEvent.Subscribe(typeid(this), [this] {
			lastInputTime = Timepoint();
			this->OnInputReceived();
		});
	}

	virtual ~ShutdownEffect() {
		window.OnKeyboardMouseEvent.Unsubscribe(typeid(this));
	}

	virtual bool DrawEffect(const Timepoint& frameStartTime) = 0;

public:
	virtual void OnInputReceived() = 0;

	virtual void OnPowerOff() = 0;

	virtual void OnPowerOn() = 0;

	void PowerOff() {
		VSTUDIO_DEBUG_OUTPUT("Powering off.");
		poweringOn = false;
		poweringOff = true;
		OnPowerOff();
	}

	void PowerOn() {
		VSTUDIO_DEBUG_OUTPUT("Powering on.");
		poweringOff = false;
		poweringOn = true;
		OnPowerOn();
	}

	void Update(const Timepoint& frameStart) {
		using namespace std::chrono_literals;
		constexpr auto idleSleepTimeMs = 1000ms;
		constexpr Duration<std::milli> idleSleepTime = idleSleepTimeMs;

		const bool sleptFrame = DrawEffect(frameStart);
		if (sleptFrame) {
			std::this_thread::sleep_for(idleSleepTimeMs);
		}
		const auto frameEnd = Timepoint();
		deltaTime = frameEnd - frameStart;
		if (sleptFrame) {
			deltaTime -= idleSleepTime;
			auto secondsSinceInput = (Timepoint() - lastInputTime).Cast<std::ratio<1,1>>();
			VSTUDIO_DEBUG_OUTPUT("Seconds since input: " << secondsSinceInput.Get());
			if (!poweringOff && (secondsSinceInput) >= 10s) {
				PowerOff();
			}
		}
	}
};