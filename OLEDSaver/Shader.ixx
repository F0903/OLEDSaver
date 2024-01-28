module;
#include <vector>
#include <string>
#include <dxgi1_5.h> 
#include <d3d11.h> 
#include <wrl.h>
export module Shader;

using namespace Microsoft::WRL;

export struct ShaderInfo
{
	enum class ShaderType
	{
		Vertex,
		Pixel,
	} const type;
	const std::string name;
};

export struct VertexShaderInfo : public ShaderInfo
{
	using ShaderPixelFormat = DXGI_FORMAT;
	using ShaderInputClassification = D3D11_INPUT_CLASSIFICATION;

	struct ShaderInputElementDescriptor
	{
		const char* semanticName;
		unsigned int semanticIndex;
		ShaderPixelFormat format;
		unsigned int inputSlot;
		unsigned int alignedByteOffset;
		ShaderInputClassification inputSlotClass;
		unsigned int instanceDataStepRate;
	};
	const std::vector<ShaderInputElementDescriptor> inputDescriptors;
};

export struct PixelShader
{
	ShaderInfo info;
	ComPtr<ID3D11PixelShader> pixelShader;
};

export struct VertexShader
{
	VertexShaderInfo info;
	ComPtr<ID3D11VertexShader> vertexShader;
	ComPtr<ID3D11InputLayout> inputLayout;
};