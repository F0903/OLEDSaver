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
using namespace concurrency;

import ErrorHandling;
import Window;
import Shader;

struct Vertex
{
	float x, y, z;
	struct Color {
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

	ComPtr<IDXGIDevice2> dxgiDevice;
	ComPtr<IDXGIAdapter2> dxgiAdapter;
	ComPtr<IDXGIFactory2> dxgiFactory;
	ComPtr<IDXGISurface> dxgiBackBuffer;
	ComPtr<IDXGISwapChain1> swapchain;

	Window& renderingWindow;

	std::vector<task<void>> shadersLoading;
	std::vector<VertexShader> vertexShaders;
	std::vector<PixelShader> pixelShaders;

	ComPtr<ID3D11Buffer> vertexBuffer;

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

public:
	D3D11Renderer(Window& window) : renderingWindow(window) {
		ASSERT(CreateDXGIFactory2(0, __uuidof(IDXGIFactory2), &dxgiFactory));
		SelectBestAdapter();

		UINT deviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(DEBUG) || defined(_DEBUG)
		deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

		ASSERT(D3D11CreateDevice(dxgiAdapter.Get(), D3D_DRIVER_TYPE_UNKNOWN, 0, deviceFlags, featureLevels, ARRAYSIZE(featureLevels), D3D11_SDK_VERSION, &d3d11Device, &selectedFeatureLevel, &d3d11Context));

		if (d3d11Device->GetFeatureLevel() < APP_MIN_FEATURE_LEVEL) {
			ErrorPopUp(L"Device feature level below app minimum!");
			return;
		}

		ASSERT(d3d11Device.As(&dxgiDevice));
		ASSERT(dxgiDevice->GetParent(__uuidof(IDXGIAdapter2), &dxgiAdapter));
		ASSERT(dxgiAdapter->GetParent(__uuidof(IDXGIFactory2), &dxgiFactory));

		DXGI_SWAP_CHAIN_DESC1 swapchainDesc{0};
		swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
		swapchainDesc.BufferCount = 2;
		swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapchainDesc.Scaling = DXGI_SCALING_NONE;
		swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
		swapchainDesc.Format = DEFAULT_DXGI_FORMAT;
		swapchainDesc.Stereo = FALSE;
		swapchainDesc.Width = 0;
		swapchainDesc.Height = 0;
		swapchainDesc.SampleDesc.Count = 1;
		swapchainDesc.SampleDesc.Quality = 0;
		swapchainDesc.Flags = 0;

		ASSERT(dxgiFactory->CreateSwapChainForHwnd(d3d11Device.Get(), window.getHandle(), &swapchainDesc, NULL, NULL, &swapchain));
	}

private:
	ComPtr<ID3D11Texture2D> GrabScreenContent(UINT outputNum, UINT timeout = 100) const {
		ComPtr<IDXGIOutput> output;
		ASSERT(dxgiAdapter->EnumOutputs(outputNum, &output));

		ComPtr<IDXGIOutput5> outputV5;
		ASSERT(output->QueryInterface<IDXGIOutput5>(&outputV5));

		ComPtr<IDXGIOutputDuplication> duplication;
		DXGI_FORMAT supportedFormats[] = {
			DEFAULT_DXGI_FORMAT
		};
		ASSERT(outputV5->DuplicateOutput1(d3d11Device.Get(), NULL, sizeof(supportedFormats) / sizeof(supportedFormats[0]), supportedFormats, &duplication));

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

	void LoadShaderAsync(const ShaderInfo& info) {
		auto shaderTask = concurrency::create_task([this, info]() {
			std::ifstream shaderFile(info.name, std::ios::binary);
			shaderFile.seekg(0, std::ios::end);
			const auto size = shaderFile.gcount();
			shaderFile.seekg(0, std::ios::beg);

			auto buf = new char[size];
			shaderFile.read(buf, size);

			switch (info.type) {
				case ShaderInfo::ShaderType::Vertex:
				{
					const auto vertexShaderInfo = static_cast<VertexShaderInfo>(info);
					ComPtr<ID3D11VertexShader> shader;
					ASSERT(d3d11Device->CreateVertexShader(buf, size, nullptr, &shader));
					const auto& descriptors = vertexShaderInfo.inputDescriptors;

					ComPtr<ID3D11InputLayout> inputLayout;
					ASSERT(d3d11Device->CreateInputLayout(reinterpret_cast<const D3D11_INPUT_ELEMENT_DESC*>(&descriptors), static_cast<UINT>(descriptors.size()), buf, size, &inputLayout));
					vertexShaders.push_back({std::move(vertexShaderInfo), std::move(shader), std::move(inputLayout)});
					break;
				}
				case ShaderInfo::ShaderType::Pixel:
				{
					ComPtr<ID3D11PixelShader> shader;
					ASSERT(d3d11Device->CreatePixelShader(buf, size, nullptr, &shader));
					pixelShaders.push_back({std::move(info), std::move(shader)});
					break;
				}
			}

			delete[] buf;
		});
		shadersLoading.push_back(std::move(shaderTask));
	}

	inline void EnsureShadersLoaded() const { 
	}

public:
	template<auto n>
	inline void LoadShadersAsync(ShaderInfo shaders[n]) {
		for (size_t i = 0; i < n; i++) {
			const auto& shader = shaders[i];
			LoadShaderAsync(shader);
		}
	}

	inline void SetActiveLoadedVertexShader(size_t index) const noexcept {
		const auto& shader = vertexShaders[index];
		d3d11Context->VSSetShader(shader.vertexShader.Get(), NULL, NULL);
	}

	inline void SetActiveLoadedPixelShader(size_t index) const noexcept {
		const auto& shader = pixelShaders[index];
		d3d11Context->PSSetShader(shader.pixelShader.Get(), NULL, NULL);
	}

	void CreateFullscreenRect() {
		constexpr Vertex vertices[] = {
			{0.0f, 0.5f, 0.0f, {1.0f, 0.0f, 0.0f, 1.0f}},
			{0.5f, -0.5f, 0.0f, {0.0f, 1.0f, 0.0f, 1.0f}},
			{-0.5f, -0.5f, 0.0f, {0.0f, 0.0f, 1.0f, 1.0f}},
		};

		D3D11_BUFFER_DESC vertBufferDesc{
			.ByteWidth = sizeof(Vertex) * std::size(vertices),
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
		//TODO
	}

	void DrawFullscreenRect() const {
		const auto windowSize = renderingWindow.getSize();



		ASSERT(swapchain->Present(1, 0));
	}
};