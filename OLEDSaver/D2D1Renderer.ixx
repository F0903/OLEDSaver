module;
#include <d3d11.h>
#include <dxgi1_2.h>
#include <d2d1.h>
#include <d2d1_1.h>
#include <d2d1_1helper.h>
#include <wrl.h>
#include <exception> 
#include <format>
#include <Windows.h>
#include <fstream>
export module D2D1Renderer;
using namespace Microsoft::WRL;
using namespace D2D1;

import ErrorHandling;
import Window;

#if defined(DEBUG) || defined(_DEBUG)
#define ASSERT(expr)\
{\
	const auto __result = expr;\
	const auto __msg = std::format("Error at line {} in {}", __LINE__, __FILE__);\
	if (__result != S_OK) {\
		throw std::exception(__msg.c_str());\
	}\
}
#else
#define ASSERT(expr) expr;
#endif 

export class D2D1Renderer
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

	ComPtr<ID2D1Factory1> d2d1Factory;
	ComPtr<ID2D1Device> d2d1Device;
	ComPtr<ID2D1DeviceContext> d2d1Context;
	ComPtr<ID2D1Bitmap1> d2d1TargetBitmap;
	ComPtr<ID2D1SolidColorBrush> d2d1BrushBlack;

	ComPtr<IDXGIDevice2> dxgiDevice;
	ComPtr<IDXGIAdapter2> dxgiAdapter;
	ComPtr<IDXGIFactory2> dxgiFactory;
	ComPtr<IDXGISurface> dxgiBackBuffer;
	ComPtr<IDXGISwapChain1> swapchain;

	Window& renderingWindow;

public:
	D2D1Renderer(Window& window) : renderingWindow(window) {  
		UINT deviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(DEBUG) || defined(_DEBUG)
		deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

		ASSERT(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, 0, deviceFlags, featureLevels, ARRAYSIZE(featureLevels), D3D11_SDK_VERSION, &d3d11Device, &selectedFeatureLevel, &d3d11Context)); 

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

		D2D1_FACTORY_OPTIONS options{};
		ASSERT(D2D1CreateFactory<ID2D1Factory1>(D2D1_FACTORY_TYPE_SINGLE_THREADED, options, &d2d1Factory)); 
		 
		ASSERT(d2d1Factory->CreateDevice(dxgiDevice.Get(), &d2d1Device));
		ASSERT(d2d1Device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &d2d1Context)); 
		ASSERT(swapchain->GetBuffer(0, IID_PPV_ARGS(&dxgiBackBuffer))); 

		const auto bitmapProps = BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW, PixelFormat(DEFAULT_DXGI_FORMAT, D2D1_ALPHA_MODE_PREMULTIPLIED), DPI, DPI);
		ASSERT(d2d1Context->CreateBitmapFromDxgiSurface(dxgiBackBuffer.Get(), bitmapProps, &d2d1TargetBitmap)); 

		d2d1Context->SetTarget(d2d1TargetBitmap.Get());

		ASSERT(d2d1Context->CreateSolidColorBrush(ColorF(ColorF::Red), &d2d1BrushBlack)); 
	}

	void GrabScreenContent(UINT outputNum) const {   
		ComPtr<IDXGIOutput> output;
		ASSERT(dxgiAdapter->EnumOutputs(outputNum, &output)); 

		ComPtr<IDXGIOutput1> outputV1;
		ASSERT(output->QueryInterface<IDXGIOutput1>(&outputV1)); 

		ComPtr<IDXGIOutputDuplication> duplication;
		ASSERT(outputV1->DuplicateOutput(d3d11Device.Get(), &duplication)); 
		 
		ComPtr<IDXGIResource> desktopResource;
		DXGI_OUTDUPL_FRAME_INFO duplFrameInfo{0};
		duplication->ReleaseFrame();
		ASSERT(duplication->AcquireNextFrame(30, &duplFrameInfo, &desktopResource)); 

		ComPtr<IDXGISurface> desktopImage;
		ASSERT(desktopResource->QueryInterface<IDXGISurface>(&desktopImage)); 

		ComPtr<ID3D11Texture2D> desktopTexture;
		ASSERT(desktopImage->QueryInterface<ID3D11Texture2D>(&desktopTexture)); 

		D3D11_TEXTURE2D_DESC desktopTextureDesc{0};
		desktopTexture->GetDesc(&desktopTextureDesc);
		const auto [width, height] = renderingWindow.getSize();
		D3D11_TEXTURE2D_DESC textureDesc{
			.Width = static_cast<UINT>(width),
			.Height = static_cast<UINT>(height),
			.MipLevels = desktopTextureDesc.MipLevels,
			.ArraySize = desktopTextureDesc.ArraySize,
			.Format = desktopTextureDesc.Format,
			.SampleDesc = desktopTextureDesc.SampleDesc,
			.Usage = D3D11_USAGE_STAGING, 
			.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE,
			.MiscFlags = 0,
		};
		ComPtr<ID3D11Texture2D> finalTexture;
		ASSERT(d3d11Device->CreateTexture2D(&textureDesc, NULL, &finalTexture)); 
		
		ComPtr<IDXGIKeyedMutex> desktopMutex;
		ASSERT(desktopTexture.As(&desktopMutex));

		ASSERT(desktopMutex->AcquireSync(0, 30));
		d3d11Context->CopyResource(finalTexture.Get(), desktopTexture.Get());
		desktopMutex->ReleaseSync(0); 

		ComPtr<IDXGISurface> finalTextureSurface;
		ASSERT(finalTexture->QueryInterface<IDXGISurface>(&finalTextureSurface)); 

		DXGI_MAPPED_RECT rawImage{0};
		ASSERT(finalTextureSurface->Map(&rawImage, DXGI_MAP_READ)); 

		std::ofstream debugOut("debug.bmp", std::ios::binary | std::ios::trunc);
		debugOut << rawImage.pBits;

		//return desktopImage;
	} 

	void DrawFullscreenRect() const noexcept {
		const auto windowSize = renderingWindow.getSize();

		//testing
		/*static auto screenTexture = */ GrabScreenContent(0);

		d2d1Context->BeginDraw();
		//d2d1Context->DrawBitmap(screenTexture, RectF(800, 800, 1200, 1200));
		d2d1Context->FillRectangle(RectF(0, 0, windowSize.width - 1500, windowSize.height - 500), d2d1BrushBlack.Get());
		ASSERT(d2d1Context->EndDraw());

		ASSERT(swapchain->Present(1, 0)); 
	}
};