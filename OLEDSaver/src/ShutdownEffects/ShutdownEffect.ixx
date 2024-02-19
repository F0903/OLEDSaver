module;
#include <chrono>
#include <thread>
export module ShutdownEffect;

import D3D11Renderer;
import Shader.PixelShader;
import Shader.VertexShader;
import Time;

export class ShutdownEffect
{
protected:
	D3D11Renderer& renderer;

	VertexShader& vertexShader;
	PixelShader& pixelShader;

	float durationSeconds;

	float effectTime = 0.0f;

	bool isDone = false;
	bool poweringOn = false;

	Duration<std::nano> deltaTime = 0;

	ShutdownEffect(D3D11Renderer& renderer, VertexShader& vertexShader, PixelShader& pixelShader, float durationSeconds) : renderer(renderer), vertexShader(vertexShader), pixelShader(pixelShader), durationSeconds(durationSeconds) {
	}

	virtual bool DrawEffect(const Timepoint& frameStartTime) = 0;

public:
	virtual void Initialize() {
		vertexShader.SetActive();
		pixelShader.SetActive();
		renderer.CreateFullscreenRect();
		renderer.Initialize();
	}

	/// <summary>
	/// Called when "shutting down"
	/// </summary>
	virtual void SetShutDown() {
		isDone = false;
		poweringOn = false;
	}

	/// <summary>
	/// Called when "powering on"
	/// </summary>
	void SetPowerOn() {
		isDone = false;
		poweringOn = true;
	}

	void Update(const Timepoint& frameStart) {
		using namespace std::chrono_literals;
		constexpr Duration<std::milli> idleSleepTime = 1000ms;

		const bool sleptFrame = DrawEffect(frameStart);
		if (sleptFrame) {
			std::this_thread::sleep_for(1s);
		}
		const auto frameEnd = Timepoint();
		deltaTime = frameEnd - frameStart;
		if (sleptFrame) deltaTime -= idleSleepTime;
	}
};