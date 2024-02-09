module;
#include <vector>
#include <string>
#include <dxgi1_5.h> 
#include <d3d11.h>
#include <wrl.h>
#include <fstream>
#include "../MacroUtils.h"
export module Shader.PixelShader;
import Shader;

export class PixelShader : public Shader
{
	friend class D3D11Renderer;

#pragma warning(push)
#pragma warning(disable : 4324)
	__declspec(align(16))
		struct ConstantBuffer
	{
		float timeMs;
		struct
		{
			float width, height;
		} resolution;
	};
#pragma warning(pop)

	inline static PixelShader* active; /// The currently active PixelShader;

	Microsoft::WRL::ComPtr<ID3D11PixelShader> shader;
	Microsoft::WRL::ComPtr<ID3D11Buffer> constantBuffer;

public:
	PixelShader(ID3D11Device& device, ID3D11DeviceContext& context, const ShaderCode& code) : Shader(device, context, code) {
		ASSERT(device.CreatePixelShader(code.data, code.dataLength, nullptr, &shader));
	}

	PixelShader(ID3D11Device& device, ID3D11DeviceContext& context, const std::string& fileName) : Shader(device, context, fileName) {
		ASSERT(device.CreatePixelShader(code.data, code.dataLength, nullptr, &shader));
	}

public:
	void InitConstantBuffer(const ConstantBuffer& bufferData) NOEXCEPT_RELEASE {
		D3D11_BUFFER_DESC bufferDesc{
			.ByteWidth = sizeof(ConstantBuffer),
			.Usage = D3D11_USAGE_DYNAMIC,
			.BindFlags = D3D11_BIND_CONSTANT_BUFFER,
			.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
			.MiscFlags = 0,
			.StructureByteStride = 0,
		};

		D3D11_SUBRESOURCE_DATA initData{
			.pSysMem = &bufferData,
			.SysMemPitch = 0,
			.SysMemSlicePitch = 0,
		};

		ASSERT(device.CreateBuffer(&bufferDesc, &initData, &constantBuffer));
		context.PSSetConstantBuffers(0, 1, constantBuffer.GetAddressOf());
	}

	void SetConstantBuffer(const ConstantBuffer& bufferData) NOEXCEPT_RELEASE {
		D3D11_MAPPED_SUBRESOURCE resourceMap{0};
		ASSERT(context.Map(constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &resourceMap));
		memcpy(resourceMap.pData, &bufferData, sizeof(ConstantBuffer));
		context.Unmap(constantBuffer.Get(), 0);
	}

	void SetActive() NOEXCEPT_RELEASE override {
		context.PSSetShader(shader.Get(), NULL, NULL);
		active = this;
	}
};