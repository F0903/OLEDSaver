module;
#include <chrono>
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

	ShutdownEffect(D3D11Renderer& renderer, VertexShader& vertexShader, PixelShader& pixelShader, float durationSeconds) : renderer(renderer), vertexShader(vertexShader), pixelShader(pixelShader), durationSeconds(durationSeconds) {
	}

public:
	virtual void Initialize() {
		vertexShader.SetActive();
		pixelShader.SetActive();
		renderer.CreateFullscreenRect();
		renderer.Initialize();
	}

	inline bool GetIsDone() const noexcept {
		return isDone;
	}

	inline bool GetPoweringOnState() const noexcept {
		return poweringOn;
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

	virtual bool DrawEffect(const Duration<std::nano>& deltaTime, const Timepoint& frameStartTime) = 0;
};