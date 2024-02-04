module;
#include <vector>
#include <string>
#include <dxgi1_5.h> 
#include <d3d11.h> 
#include <wrl.h>
export module Shader;

// using namespace aliasing will break intellisense on module import

export struct ShaderCode
{
	const void* data;
	const unsigned int dataLength;
	const bool heapAllocated;

	ShaderCode(ShaderCode&&) = default;
	ShaderCode(const ShaderCode&) = default;

	template<class DataT, auto DataLength>
	ShaderCode(DataT(&data)[DataLength], const bool heapAllocated = false) :
		data(data),
		dataLength(DataLength),
		heapAllocated(heapAllocated) {
	}

	ShaderCode(const void* data, const unsigned int dataLength, const bool heapAllocated = true) :
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

export
template<class T> requires std::is_base_of_v<ID3D11DeviceChild, T>
struct Shader
{
	const Microsoft::WRL::ComPtr<T> shader;
	const ShaderCode code;
};

export
enum class ShaderType
{
	Vertex,
	Pixel,
};

export
enum class ShaderLoadType
{
	File,
	RawCode
};

export struct ShaderLoadInfo
{
	const ShaderLoadType loadType;
	const ShaderType type;
	const ShaderCode code;

	ShaderLoadInfo(const ShaderLoadType loadType, const ShaderType type, const ShaderCode code) :
		loadType(loadType),
		type(type),
		code(code) {
	}
};