module;
#include <d3d11.h> 
#include <dxgi1_5.h> 
#include <d2d1_1.h>
#include <d2d1_1helper.h>
#include <wrl.h>
#include <Windows.h>
#include <fstream>
#include <chrono>
#include <ppl.h>
#include <string>  
#include <mutex>
#include <span>
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
};

export class D3D11Renderer
{
	static constexpr auto DEFAULT_DXGI_FORMAT = DXGI_FORMAT_B8G8R8A8_UNORM;
	static constexpr auto DPI = 96;
	static constexpr auto VRR = true;

	static constexpr D3D_FEATURE_LEVEL DIRECTX_FEATURE_LEVELS[] = {
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_11_1,
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

	std::vector<VertexShader> vertexShaders;
	std::mutex vertexShadersMutex;

	std::vector<PixelShader> pixelShaders;
	std::mutex pixelShadersMutex;

	ComPtr<ID3D11Buffer> vertexBuffer;
	struct CurrentlyDrawingInfo
	{
		unsigned int vertexCount;
		unsigned int vertexStart;
		unsigned int indexCount;
		unsigned int indexStart;
	} currentlyDrawingInfo;

	std::chrono::time_point<std::chrono::high_resolution_clock> startTime;

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

		ASSERT(D3D11CreateDevice(dxgiAdapter.Get(), D3D_DRIVER_TYPE_UNKNOWN, 0, deviceFlags, DIRECTX_FEATURE_LEVELS, ARRAYSIZE(DIRECTX_FEATURE_LEVELS), D3D11_SDK_VERSION, &d3d11Device, &selectedFeatureLevel, &d3d11Context));

		if (d3d11Device->GetFeatureLevel() < DIRECTX_FEATURE_LEVELS[0]) {
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
		swapchainDesc.Flags = VRR ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

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
		startTime = std::chrono::high_resolution_clock::now();
		ASSERT(CreateDXGIFactory2(0, __uuidof(IDXGIFactory2), &dxgiFactory));
		SelectBestAdapter();
		InitDevice();
		InitFullscreenSwapchain();
	}

private:
	void LoadShader(const std::pair<const ShaderCode&, const ShaderType>& info) NOEXCEPT_RELEASE {
		const auto& [code, type] = info;
		switch (type) {
			case ShaderType::Vertex:
			{
				auto shader = VertexShader(*d3d11Device.Get(), *d3d11Context.Get(), std::move(code));
				{
					std::lock_guard lock(vertexShadersMutex);
					vertexShaders.push_back(std::move(shader));
				}
				break;
			}
			case ShaderType::Pixel:
			{
				auto shader = PixelShader(*d3d11Device.Get(), *d3d11Context.Get(), std::move(code));
				{
					std::lock_guard lock(pixelShadersMutex);
					pixelShaders.push_back(std::move(shader));
				}
				break;
			}
		}
	}

public:
	inline void LoadShadersParallel(const std::vector<std::pair<const ShaderCode&, const ShaderType>>& shaders) NOEXCEPT_RELEASE {
		// Can't get parallel_for_each to work, so using this.
		concurrency::parallel_for(size_t(0), shaders.size(), [&](size_t i) {
			auto& shaderInfo = shaders[i];
			LoadShader(shaderInfo);
		});
	}

	inline VertexShader& GetLoadedVertexShader(size_t index) noexcept {
		return vertexShaders[index];
	}

	inline PixelShader& GetLoadedPixelShader(size_t index) noexcept {
		return pixelShaders[index];
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
			{-1.0f, 1.0f, 0.0f},
			{1.0f, 1.0f, 0.0f},
			{1.0f, -1.0f, 0.0f},
			{-1.0f, -1.0f, 0.0f},
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

		D3D11_BUFFER_DESC bufferDesc{
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

	void Initialize() NOEXCEPT_RELEASE {
		VertexShader::active->SetInputLayout();
		PixelShader::active->InitConstantBuffer({
			0.0f,
			{
				static_cast<float>(currentWindowSize.width),
				static_cast<float>(currentWindowSize.height)
			}
		});
	}

	void Draw() NOEXCEPT_RELEASE {
		const auto timeFrameStart = std::chrono::high_resolution_clock::now();
		const auto timeSinceStart = std::chrono::duration_cast<std::chrono::milliseconds>(timeFrameStart - startTime);
		PixelShader::active->SetConstantBuffer({
			static_cast<float>(timeSinceStart.count()) / 1000.0f,
			{
				static_cast<float>(currentWindowSize.width),
				static_cast<float>(currentWindowSize.height)
			}
		});

		static constexpr const FLOAT backbufferColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
		d3d11Context->OMSetRenderTargets(1, renderTarget.GetAddressOf(), NULL);
		d3d11Context->ClearRenderTargetView(renderTarget.Get(), backbufferColor);
		d3d11Context->DrawIndexed(currentlyDrawingInfo.indexCount, currentlyDrawingInfo.indexStart, currentlyDrawingInfo.vertexStart);
		ASSERT(swapchain->Present(VRR ? 0 : 1, VRR ? DXGI_PRESENT_ALLOW_TEARING : 0));
	}
};