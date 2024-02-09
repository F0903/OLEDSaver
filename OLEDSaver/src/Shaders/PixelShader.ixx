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
	std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> resourceViews;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerState;

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

	void SetTexture2D(ID3D11Texture2D* texture) {
		D3D11_TEXTURE2D_DESC textureDesc;
		texture->GetDesc(&textureDesc);

		D3D11_MAPPED_SUBRESOURCE subresourceMap;
		ASSERT(context.Map(texture, 0, D3D11_MAP_READ, NULL, &subresourceMap));

		D3D11_SUBRESOURCE_DATA subresourceData{
			.pSysMem = subresourceMap.pData,
			.SysMemPitch = subresourceMap.RowPitch,
			.SysMemSlicePitch = subresourceMap.DepthPitch,
		};

		textureDesc.Usage = D3D11_USAGE_DEFAULT;
		textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

		Microsoft::WRL::ComPtr<ID3D11Texture2D> newTexture;
		ASSERT(device.CreateTexture2D(&textureDesc, &subresourceData, &newTexture));
		context.Unmap(texture, 0);
		
		D3D11_SHADER_RESOURCE_VIEW_DESC resourceDesc{
			.Format = textureDesc.Format,
			.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D,
			.Texture2D = {
				.MostDetailedMip = textureDesc.MipLevels - 1,
				.MipLevels = textureDesc.MipLevels,
			},
		};
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> resourceView;
		ASSERT(device.CreateShaderResourceView(newTexture.Get(), &resourceDesc, &resourceView));

		resourceViews.push_back(std::move(resourceView));

		D3D11_SAMPLER_DESC samplerDesc{
			.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR,
			.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP,
			.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP,
			.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP,
			.MipLODBias = 0,
			.MaxAnisotropy = 16,
			.ComparisonFunc = D3D11_COMPARISON_NEVER,
			.MinLOD = 0,
			.MaxLOD = D3D11_FLOAT32_MAX,
		};

		ASSERT(device.CreateSamplerState(&samplerDesc, &samplerState));
	}

	void SetActive() NOEXCEPT_RELEASE override {
		context.PSSetShader(shader.Get(), NULL, NULL);
		context.PSSetShaderResources(0, static_cast<UINT>(resourceViews.size()), resourceViews.data()->GetAddressOf());
		active = this;
	}
};