module;
#include <vector>
#include <string>
#include <dxgi1_5.h> 
#include <d3d11.h>
#include <wrl.h>
#include <fstream>
#include "../MacroUtils.h"
export module Shader;

// using namespace aliasing will break intellisense on module import

export
enum class ConstantBufferSlot
{
	Default = 0,
	Custom = 1
};

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