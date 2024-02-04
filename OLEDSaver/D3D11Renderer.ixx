module;
#include <d3d11.h> 
#include <dxgi1_5.h> 
#include <d2d1_1.h>
#include <d2d1_1helper.h>
#include <wrl.h>
#include <Windows.h>
#include <fstream>
#include <ppltasks.h>
#include <string>  
#include "MacroUtils.h"
export module D3D11Renderer;

using namespace Microsoft::WRL;
using namespace D2D1;

import ErrorHandling;
import Window;
import Shader;

struct Vertex
{
	float x, y, z;
	struct Color
	{
		float r, g, b, a;
	} color;
};

export class D3D11Renderer
{
	static constexpr auto DEFAULT_DXGI_FORMAT = DXGI_FORMAT_B8G8R8A8_UNORM;
	static constexpr auto APP_MIN_FEATURE_LEVEL = D3D_FEATURE_LEVEL_9_1;
	static constexpr auto DPI = 96;

	static constexpr D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1,
	};

	D3D_FEATURE_LEVEL selectedFeatureLevel;

	ComPtr<ID3D11Device> d3d11Device;
	ComPtr<ID3D11DeviceContext> d3d11Context;
	 
	ComPtr<IDXGIAdapter2> dxgiAdapter;
	ComPtr<IDXGIFactory2> dxgiFactory;
	ComPtr<IDXGISwapChain1> swapchain;
	ComPtr<ID3D11RenderTargetView> renderTarget;

	Window& renderingWindow;
	Window::Size currentWindowSize;

	std::vector<concurrency::task<void>> shadersLoading;
	std::vector<Shader<ID3D11VertexShader>> vertexShaders;
	std::vector<Shader<ID3D11PixelShader>> pixelShaders;
	const Shader<ID3D11VertexShader>* activeVertexShader = nullptr;
	const Shader<ID3D11PixelShader>* activePixelShader = nullptr;

	ComPtr<ID3D11Buffer> vertexBuffer;
	struct CurrentlyDrawingInfo
	{
		unsigned int vertexCount;
		unsigned int vertexStart;
		unsigned int indexCount;
		unsigned int indexStart;
	} currentlyDrawingInfo; 

	// Select the best adapter based on the most dedicated video memory. (a little primitive)
	void SelectBestAdapter() {
		int i = 0;
		IDXGIAdapter* bestAdapter = nullptr;
		DXGI_ADAPTER_DESC bestAdapterDesc{0};
		IDXGIAdapter* currentAdapter;
		while (true) {
			const auto result = dxgiFactory->EnumAdapters(i++, &currentAdapter);
			if (result == DXGI_ERROR_NOT_FOUND)
				break;
			ASSERT(result);

			DXGI_ADAPTER_DESC adapterDesc{0};
			ASSERT(currentAdapter->GetDesc(&adapterDesc));
			if (!bestAdapter || adapterDesc.DedicatedVideoMemory > bestAdapterDesc.DedicatedVideoMemory) {
				bestAdapter = currentAdapter;
				bestAdapterDesc = adapterDesc;
				continue;
			}
			currentAdapter->Release();
		}

		if (!bestAdapter) throw std::exception("No suitable adapter was found!");

		ASSERT(bestAdapter->QueryInterface<IDXGIAdapter2>(&dxgiAdapter));
	}

	void InitDevice() NOEXCEPT_RELEASE {
		UINT deviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(DEBUG) || defined(_DEBUG)
		deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

		ASSERT(D3D11CreateDevice(dxgiAdapter.Get(), D3D_DRIVER_TYPE_UNKNOWN, 0, deviceFlags, featureLevels, ARRAYSIZE(featureLevels), D3D11_SDK_VERSION, &d3d11Device, &selectedFeatureLevel, &d3d11Context));

		if (d3d11Device->GetFeatureLevel() < APP_MIN_FEATURE_LEVEL) {
			ErrorPopUp(L"Device feature level below app minimum!");
			return;
		} 
	}

	void InitFullscreenSwapchain() NOEXCEPT_RELEASE {
		DXGI_SWAP_CHAIN_DESC1 swapchainDesc{0};
		swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
		swapchainDesc.BufferCount = 2;
		swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapchainDesc.Scaling = DXGI_SCALING_NONE;
		swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
		swapchainDesc.Format = DEFAULT_DXGI_FORMAT;
		swapchainDesc.Stereo = FALSE;
		swapchainDesc.Width = currentWindowSize.width;
		swapchainDesc.Height = currentWindowSize.height;
		swapchainDesc.SampleDesc.Count = 1;
		swapchainDesc.SampleDesc.Quality = 0;
		swapchainDesc.Flags = NULL;

		DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreenDesc{
			.RefreshRate = {.Numerator = GetDisplayRefreshHz(), .Denominator = 1},
			.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
			.Scaling = DXGI_MODE_SCALING_UNSPECIFIED,
			.Windowed = DEBUG_VALUE(TRUE, FALSE),
		};
		  
		ASSERT(dxgiFactory->CreateSwapChainForHwnd(d3d11Device.Get(), renderingWindow.GetHandle(), &swapchainDesc, &fullscreenDesc, NULL, &swapchain));

		ComPtr<ID3D11Texture2D> backbufferTexture;
		ASSERT(swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), &backbufferTexture));
		ASSERT(d3d11Device->CreateRenderTargetView(backbufferTexture.Get(), NULL, &renderTarget)); 
		d3d11Context->OMSetRenderTargets(1, renderTarget.GetAddressOf(), NULL);

		D3D11_VIEWPORT viewport{
			.TopLeftX = 0,
			.TopLeftY = 0,
			.Width = static_cast<float>(currentWindowSize.width),
			.Height = static_cast<float>(currentWindowSize.height)
		};
		d3d11Context->RSSetViewports(1, &viewport);
	} 

public:
	D3D11Renderer(Window& window) : renderingWindow(window) {
		currentWindowSize = window.GetSize();
		ASSERT(CreateDXGIFactory2(0, __uuidof(IDXGIFactory2), &dxgiFactory));
		SelectBestAdapter();
		InitDevice();
		InitFullscreenSwapchain();
	}

private:
	void SetInputLayout(const Shader<ID3D11VertexShader>& vertexShader) NOEXCEPT_RELEASE {
		D3D11_INPUT_ELEMENT_DESC vertexProps[] = {
			{"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(float) * 3, D3D11_INPUT_PER_VERTEX_DATA, 0}
		}; 

		ComPtr<ID3D11InputLayout> inputLayout;
		ASSERT(d3d11Device->CreateInputLayout(vertexProps, static_cast<UINT>(std::size(vertexProps)), vertexShader.code.data, vertexShader.code.dataLength, &inputLayout));
		d3d11Context->IASetInputLayout(inputLayout.Get()); 
	}

	void LoadShaderAsync(const ShaderLoadInfo& info) NOEXCEPT_RELEASE {
		auto shaderTask = concurrency::create_task([this, &info]() {
			const void* data = info.code.data;
			const auto size = info.code.dataLength;
			const auto isFile = info.loadType == ShaderLoadType::File;
			if (isFile) {
				std::ifstream shaderFile(reinterpret_cast<const char*>(info.code.data), std::ios::binary);
				shaderFile.seekg(0, std::ios::end);
				const auto size = shaderFile.gcount();
				shaderFile.seekg(0, std::ios::beg);
				auto buf = new char[size]; // Do not delete, this will be handled by shader objects.
				shaderFile.read(buf, size);
				data = buf;
			}

			switch (info.type) {
				case ShaderType::Vertex:
				{
					ComPtr<ID3D11VertexShader> shader;
					ASSERT(d3d11Device->CreateVertexShader(data, size, nullptr, &shader));
					vertexShaders.push_back({std::move(shader), std::move(info.code)});
					break;
				}
				case ShaderType::Pixel:
				{
					ComPtr<ID3D11PixelShader> shader;
					ASSERT(d3d11Device->CreatePixelShader(data, size, nullptr, &shader));
					pixelShaders.push_back({std::move(shader), std::move(info.code)});
					break;
				}
			}

			// If the loadType is RawCode then the data will be deleted in its destructor.
			if (isFile) {
				delete data;
			}
		});
		shadersLoading.push_back(std::move(shaderTask));
	}

	inline void EnsureShadersLoaded() const noexcept {
		auto waitAllTask = concurrency::when_all(shadersLoading.begin(), shadersLoading.end());
		waitAllTask.wait();
	}

public:
	template<auto N>
	inline void LoadShadersAsync(ShaderLoadInfo(&shaders)[N]) noexcept {
		for (size_t i = 0; i < N; i++) {
			const auto& shader = shaders[i];
			LoadShaderAsync(shader);
		}
	}

	inline void SetActiveLoadedVertexShader(size_t index) noexcept {
		EnsureShadersLoaded();
		const auto& shader = vertexShaders[index];
		d3d11Context->VSSetShader(shader.shader.Get(), NULL, NULL);
		SetInputLayout(shader);
		activeVertexShader = &shader;
	}

	inline void SetActiveLoadedPixelShader(size_t index) noexcept {
		EnsureShadersLoaded();
		const auto& shader = pixelShaders[index];
		d3d11Context->PSSetShader(shader.shader.Get(), NULL, NULL);
		activePixelShader = &shader;
	}

	ComPtr<ID3D11Texture2D> GrabScreenContent(UINT outputNum, UINT timeout = 100) const NOEXCEPT_RELEASE {
		ComPtr<IDXGIOutput> output;
		ASSERT(dxgiAdapter->EnumOutputs(outputNum, &output));

		ComPtr<IDXGIOutput5> outputV5;
		ASSERT(output->QueryInterface<IDXGIOutput5>(&outputV5));

		ComPtr<IDXGIOutputDuplication> duplication;
		DXGI_FORMAT supportedFormats[] = {
			DEFAULT_DXGI_FORMAT
		};
		ASSERT(outputV5->DuplicateOutput1(d3d11Device.Get(), NULL, ARRAYSIZE(supportedFormats), supportedFormats, &duplication));

		ComPtr<IDXGIResource> desktopResource;
		DXGI_OUTDUPL_FRAME_INFO duplFrameInfo{0};
		while (true) {
			duplication->ReleaseFrame();
			const auto result = duplication->AcquireNextFrame(timeout, &duplFrameInfo, &desktopResource);
			if (result == S_OK) break;
			ASSERT(result);
		}

		ComPtr<IDXGISurface> desktopImage;
		ASSERT(desktopResource->QueryInterface<IDXGISurface>(&desktopImage));

		ComPtr<ID3D11Texture2D> desktopTexture;
		ASSERT(desktopImage->QueryInterface<ID3D11Texture2D>(&desktopTexture));

		D3D11_TEXTURE2D_DESC desktopTextureDesc{0};
		desktopTexture->GetDesc(&desktopTextureDesc);
		D3D11_TEXTURE2D_DESC textureDesc{
			.Width = desktopTextureDesc.Width,
			.Height = desktopTextureDesc.Height,
			.MipLevels = desktopTextureDesc.MipLevels,
			.ArraySize = desktopTextureDesc.ArraySize,
			.Format = desktopTextureDesc.Format,
			.SampleDesc = desktopTextureDesc.SampleDesc,
			.Usage = D3D11_USAGE_STAGING,
			.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE,
			.MiscFlags = 0,
		};
		ComPtr<ID3D11Texture2D> stagingTexture;
		ASSERT(d3d11Device->CreateTexture2D(&textureDesc, NULL, &stagingTexture));
		d3d11Context->CopyResource(stagingTexture.Get(), desktopTexture.Get());

		return stagingTexture;
	}

	void CreateFullscreenRect() NOEXCEPT_RELEASE {
		constexpr Vertex vertices[] = {
			{-1.0f, 1.0f, 0.0f, {1.0f, 0.0f, 0.0f, 1.0f}},
			{1.0f, 1.0f, 0.0f, {0.0f, 1.0f, 0.0f, 1.0f}},
			{1.0f, -1.0f, 0.0f, {0.0f, 0.0f, 1.0f, 1.0f}},
			{-1.0f, -1.0f, 0.0f, {1.0f, 1.0f, 1.0f, 1.0f}},
		};

		D3D11_BUFFER_DESC vertBufferDesc{
			.ByteWidth = static_cast<UINT>(sizeof(Vertex) * std::size(vertices)),
			.Usage = D3D11_USAGE_DYNAMIC,
			.BindFlags = D3D11_BIND_VERTEX_BUFFER,
			.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
			.StructureByteStride = 0,
		};

		ASSERT(d3d11Device->CreateBuffer(&vertBufferDesc, NULL, &vertexBuffer));

		D3D11_MAPPED_SUBRESOURCE subresource;
		ASSERT(d3d11Context->Map(vertexBuffer.Get(), NULL, D3D11_MAP_WRITE_DISCARD, NULL, &subresource));
		memcpy(subresource.pData, vertices, sizeof(vertices));
		d3d11Context->Unmap(vertexBuffer.Get(), NULL);

		UINT strides[] = {sizeof(Vertex)};
		UINT offsets[] = {0};
		d3d11Context->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), strides, offsets);
		d3d11Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		ComPtr<ID3D11Buffer> indexBuffer;
		unsigned int indices[] = {0, 1, 2, 2, 3, 0};

		D3D11_BUFFER_DESC bufferDesc {
			.ByteWidth = ARRAYSIZE(indices) * sizeof(unsigned int),
			.Usage = D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11_BIND_INDEX_BUFFER,
			.CPUAccessFlags = NULL,
			.MiscFlags = NULL,
		};

		D3D11_SUBRESOURCE_DATA initData{
			.pSysMem = indices,
			.SysMemPitch = 0,
			.SysMemSlicePitch = 0,
		};

		ASSERT(d3d11Device->CreateBuffer(&bufferDesc, &initData, &indexBuffer));
		d3d11Context->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
		currentlyDrawingInfo = {
			.vertexCount = static_cast<UINT>(std::size(vertices)),
			.vertexStart = 0,
			.indexCount = static_cast<UINT>(std::size(indices)),
			.indexStart = 0,
		};
	}

	void Draw() NOEXCEPT_RELEASE { 
		static constexpr const FLOAT backbufferColor[4] = {0.0f, 0.0f, 0.0f, 1.0f}; 
		d3d11Context->OMSetRenderTargets(1, renderTarget.GetAddressOf(), NULL);
		d3d11Context->ClearRenderTargetView(renderTarget.Get(), backbufferColor);
		d3d11Context->DrawIndexed(currentlyDrawingInfo.indexCount, currentlyDrawingInfo.indexStart,currentlyDrawingInfo.vertexStart);
		ASSERT(swapchain->Present(1, NULL));
	}
};