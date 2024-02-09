module;
#include <vector>
#include <string>
#include <dxgi1_5.h> 
#include <d3d11.h>
#include <wrl.h>
#include <fstream>
#include "MacroUtils.h"
export module Shader;

// using namespace aliasing will break intellisense on module import

export
enum class ShaderType
{
	Vertex,
	Pixel,
};

export struct ShaderCode
{
	const void* data;
	const size_t dataLength;
	const bool heapAllocated;

	ShaderCode(ShaderCode&&) = default;
	ShaderCode(const ShaderCode&) = default;

	template<class DataT, size_t DataLength>
	ShaderCode(DataT(&data)[DataLength], const bool heapAllocated = false) :
		data(data),
		dataLength(DataLength),
		heapAllocated(heapAllocated) {
	}

	ShaderCode(const void* data, const size_t dataLength, const bool heapAllocated = true) :
		data(data),
		dataLength(dataLength),
		heapAllocated(heapAllocated) {
	}

	~ShaderCode() {
		if (heapAllocated) {
			delete data;
		}
	}
};

export class Shader
{
	friend class D3D11Renderer;

protected:
	ID3D11Device& device;
	ID3D11DeviceContext& context;
	const ShaderCode code;
	const ShaderType type;

public:
	Shader(ID3D11Device& device, ID3D11DeviceContext& context, const ShaderCode& code) :
		device(device),
		context(context),
		code(code),
		type(type) {
	}

	Shader(ID3D11Device& device, ID3D11DeviceContext& context, const std::string& fileName) :
		device(device),
		context(context),
		code(CompileOrLoadFile(fileName)),
		type(type) {
	}

protected:
	ShaderCode CompileOrLoadFile(const std::string& fileName) const NOEXCEPT_RELEASE {
		std::ifstream shaderFile(fileName, std::ios::binary);

		shaderFile.seekg(0, std::ios::end);
		const auto fileSize = shaderFile.gcount();
		shaderFile.seekg(0, std::ios::beg);

		auto buf = new char[fileSize];
		shaderFile.read(buf, fileSize);

		//TODO: Figure out if file is compiled or code. (prob go with the file extension)

		return ShaderCode(buf, fileSize);
	}

public:
	virtual void SetActive() = 0;

};

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
		SetInputLayout();
		active = this;
	}
};

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

private:
	void SetConstantBuffer(ConstantBuffer&& bufferData) NOEXCEPT_RELEASE {
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
		//TODO: Map and set the constant buffer instead of creating a new one each time.
	}

public:
	void SetActive() NOEXCEPT_RELEASE override {
		context.PSSetShader(shader.Get(), NULL, NULL);
		active = this;
	}
};