module;
#include <vector>
#include <string>
#include <dxgi1_5.h> 
#include <d3d11.h>
#include <wrl.h>
#include <fstream>
#include "../MacroUtils.h"
export module Shader.VertexShader;
import Shader;

export class VertexShader : public Shader
{
	friend class D3D11Renderer;

	inline static VertexShader* active; /// The currently active PixelShader;

	Microsoft::WRL::ComPtr<ID3D11VertexShader> shader;

public:
	VertexShader(ID3D11Device& device, ID3D11DeviceContext& context, const ShaderCode& code) : Shader(device, context, code) {
		ASSERT(device.CreateVertexShader(code.data, code.dataLength, nullptr, &shader));
	}

	VertexShader(ID3D11Device& device, ID3D11DeviceContext& context, const std::string& fileName) : Shader(device, context, fileName) {
		ASSERT(device.CreateVertexShader(code.data, code.dataLength, nullptr, &shader));
	}

private:
	void SetInputLayout() const NOEXCEPT_RELEASE {
		D3D11_INPUT_ELEMENT_DESC vertexProps[] = {
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}
		};

		Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;
		ASSERT(device.CreateInputLayout(vertexProps, static_cast<UINT>(std::size(vertexProps)), code.data, code.dataLength, &inputLayout));
		context.IASetInputLayout(inputLayout.Get());
	}

public:
	void SetActive() NOEXCEPT_RELEASE override {
		context.VSSetShader(shader.Get(), NULL, NULL);
		active = this;
	}
};