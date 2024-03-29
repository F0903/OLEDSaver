module;
#include <Windows.h>
#include <d3d11.h>
#include <dxgi1_5.h>
#include <wrl.h>
#include <ppl.h>
#include <dcomp.h>
#include <fstream>
#include <string>
#include <mutex>
#include <chrono>
#include <thread>
#include "MacroUtils.h"
export module D3D11Renderer;

import ErrorHandling;
import Window;
import Shader;
import Shader.PixelShader;
import Shader.VertexShader;
import Time;

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
		D3D_FEATURE_LEVEL_11_1,
	};

	D3D_FEATURE_LEVEL selectedFeatureLevel;

	// IntelliSense is currently bugged with namespace aliasing in C++20 modules, so we have to use the full name.
	Microsoft::WRL::ComPtr<IDXGIDevice1> dxgiDevice;
	Microsoft::WRL::ComPtr<ID3D11Device> d3d11Device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> d3d11Context;

	Microsoft::WRL::ComPtr<IDXGIAdapter2> dxgiAdapter;
	Microsoft::WRL::ComPtr<IDXGIFactory2> dxgiFactory;
	Microsoft::WRL::ComPtr<IDXGISwapChain1> swapchain;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderTarget;

	Microsoft::WRL::ComPtr<IDCompositionDevice> compDevice;
	Microsoft::WRL::ComPtr<IDCompositionTarget> compTarget;

	Window& renderingWindow;
	Window::Size currentWindowSize;

	std::vector<VertexShader> vertexShaders;
	std::mutex vertexShadersMutex;

	std::vector<PixelShader> pixelShaders;
	std::mutex pixelShadersMutex;

	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
	struct CurrentlyDrawingInfo
	{
		unsigned int vertexCount;
		unsigned int vertexStart;
		unsigned int indexCount;
		unsigned int indexStart;
	} currentlyDrawingInfo;

	Duration<std::milli> targetFrameTime;

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
		constexpr UINT deviceFlags = DEBUG_VALUE(D3D11_CREATE_DEVICE_DEBUG | D3D11_CREATE_DEVICE_BGRA_SUPPORT, D3D11_CREATE_DEVICE_BGRA_SUPPORT);

		ASSERT(D3D11CreateDevice(dxgiAdapter.Get(), D3D_DRIVER_TYPE_UNKNOWN, 0, deviceFlags, DIRECTX_FEATURE_LEVELS, ARRAYSIZE(DIRECTX_FEATURE_LEVELS), D3D11_SDK_VERSION, &d3d11Device, &selectedFeatureLevel, &d3d11Context));

		if (d3d11Device->GetFeatureLevel() < DIRECTX_FEATURE_LEVELS[0]) {
			ErrorPopUp(L"Device feature level below app minimum!");
			return;
		}

		ASSERT(d3d11Device.As(&dxgiDevice));
	}

	void InitSwapchain() NOEXCEPT_RELEASE {
		DXGI_SWAP_CHAIN_DESC1 swapchainDesc{
			.Width = static_cast<UINT>(currentWindowSize.width),
			.Height = static_cast<UINT>(currentWindowSize.height),
			.Format = DEFAULT_DXGI_FORMAT,
			.Stereo = FALSE,
			.SampleDesc = {.Count = 1, .Quality = 0},
			.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
			.BufferCount = 2,
			.Scaling = DXGI_SCALING_STRETCH,
			.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL,
			.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED,
			.Flags = VRR ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0,
		};

		ASSERT(dxgiFactory->CreateSwapChainForComposition(d3d11Device.Get(), &swapchainDesc, NULL, &swapchain));

		Microsoft::WRL::ComPtr<ID3D11Texture2D> backbufferTexture;
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

	void InitDirectComposition() NOEXCEPT_RELEASE {
		ASSERT(DCompositionCreateDevice(dxgiDevice.Get(), __uuidof(compDevice), &compDevice));
		ASSERT(compDevice->CreateTargetForHwnd(renderingWindow.GetHandle(), TRUE, &compTarget));
		Microsoft::WRL::ComPtr<IDCompositionVisual> compVisual;
		ASSERT(compDevice->CreateVisual(&compVisual));
		ASSERT(compVisual->SetContent(swapchain.Get()));
		ASSERT(compTarget->SetRoot(compVisual.Get()));
		ASSERT(compDevice->Commit());
	}

public:
	D3D11Renderer(Window& window) : 
		renderingWindow(window), 
		currentWindowSize(window.GetSize()),
		targetFrameTime(static_cast<long long>((1.0f / static_cast<double>(GetDisplayRefreshHz())) * 1000)
	) {
		ASSERT(CreateDXGIFactory2(0, __uuidof(IDXGIFactory2), &dxgiFactory));
		SelectBestAdapter();
		InitDevice();
		InitSwapchain();
		InitDirectComposition();
	}

public:
	Shader& LoadShader(const ShaderCode& code, const ShaderType type) NOEXCEPT_RELEASE {
		switch (type) {
			case ShaderType::Vertex:
			{
				auto shader = VertexShader(*d3d11Device.Get(), *d3d11Context.Get(), std::move(code));
				{
					std::lock_guard lock(vertexShadersMutex);
					vertexShaders.push_back(std::move(shader));
				}
				return vertexShaders.back();
			}
			case ShaderType::Pixel:
			{
				auto shader = PixelShader(*d3d11Device.Get(), *d3d11Context.Get(), std::move(code));
				{
					std::lock_guard lock(pixelShadersMutex);
					pixelShaders.push_back(std::move(shader));
				}
				return pixelShaders.back();
			}
		}
		throw std::exception("Cannot load unknown shader type!");
	}

	inline void LoadShadersParallel(const std::vector<std::pair<const ShaderCode, const ShaderType>>& shaders) NOEXCEPT_RELEASE {
		// Can't get parallel_for_each to work, so using this.
		concurrency::parallel_for(size_t(0), shaders.size(), [&](size_t i) {
			auto& [code, type] = shaders[i];
			LoadShader(code, type);
		});
	}

	inline VertexShader& GetLoadedVertexShader(size_t index) noexcept {
		return vertexShaders[index];
	}

	inline PixelShader& GetLoadedPixelShader(size_t index) noexcept {
		return pixelShaders[index];
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

		Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
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
		VertexShader::GetActive()->SetInputLayout();
		PixelShader::GetActive()->InitConstantBuffer({
			0.0f,
			{
				static_cast<float>(currentWindowSize.width),
				static_cast<float>(currentWindowSize.height)
			}
		}, ConstantBufferSlot::Default);
	}

	void Draw(PixelShader& pixelShader, const Timepoint& frameStartTime) NOEXCEPT_RELEASE {
		pixelShader.SetConstantBuffer({
			(Timepoint() - frameStartTime).Cast<std::milli>().GetCount<float>() / 1000.0f,
			{
				static_cast<float>(currentWindowSize.width),
				static_cast<float>(currentWindowSize.height)
			}
		}, ConstantBufferSlot::Default);

		d3d11Context->OMSetRenderTargets(1, renderTarget.GetAddressOf(), NULL);
		static constexpr const FLOAT backbufferColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
		d3d11Context->ClearRenderTargetView(renderTarget.Get(), backbufferColor);
		
		d3d11Context->DrawIndexed(currentlyDrawingInfo.indexCount, currentlyDrawingInfo.indexStart, currentlyDrawingInfo.vertexStart);

		const auto timeTaken = Timepoint() - frameStartTime;
		if (timeTaken < targetFrameTime) {
			const auto sleepDuration = targetFrameTime.Get() - timeTaken.Get();
			std::this_thread::sleep_for(sleepDuration);
		}
		DEBUG_EXPRESSION(const auto debugTotalTimeTaken = (Timepoint() - frameStartTime).Cast<std::milli>().GetCount<float>();,);
		ASSERT(swapchain->Present(VRR ? 0 : 1, VRR ? DXGI_PRESENT_ALLOW_TEARING : 0));
		VSTUDIO_DEBUG_OUTPUT("Presented frame took ~" << debugTotalTimeTaken << "ms " << "[~" << 1/(debugTotalTimeTaken/1000.0f) << " FPS]");
	}
};