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
export module D2D1Renderer;
using namespace Microsoft::WRL;
using namespace D2D1;

import ErrorHandling;
import Window;

template<class T>
class GDIObject
{
	HGDIOBJ object;

public:
	GDIObject(const HGDIOBJ& object) : object(object) {
		auto dbg = "a";
		auto _ = dbg;
	}

	GDIObject(HGDIOBJ&& object) : object(object) {
		auto dbg = "a";
		auto _ = dbg;
	}

	~GDIObject() {
		DeleteObject(object);
	}

	T operator*() const noexcept {
		return reinterpret_cast<T>(this);
	}

	T operator->() const noexcept {
		return reinterpret_cast<T>(this);
	}

	T* operator&() const noexcept {
		return reinterpret_cast<T*>(this);
	}
};

inline void assertResult(HRESULT result) {
	const auto msg = std::format("Error!\n Error at line {}", __LINE__);
	if (result != S_OK) {
		throw std::exception(msg.c_str());
	}
}

export class D2D1Renderer
{
	static constexpr auto APP_MIN_FEATURE_LEVEL = D3D_FEATURE_LEVEL_9_1;
	static constexpr auto DPI = 96;

	static constexpr D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_12_2,
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
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
		HRESULT result;

		UINT deviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(DEBUG) || defined(_DEBUG)
		deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

		result = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, 0, deviceFlags, featureLevels, ARRAYSIZE(featureLevels), D3D11_SDK_VERSION, &d3d11Device, &selectedFeatureLevel, &d3d11Context);
		assertResult(result);

		if (d3d11Device->GetFeatureLevel() < APP_MIN_FEATURE_LEVEL) {
			errorPopUp(L"Device feature level below app minimum!");
			return;
		}

		result = d3d11Device.As(&dxgiDevice);
		assertResult(result);

		result = dxgiDevice->GetParent(__uuidof(IDXGIAdapter2), &dxgiAdapter);
		assertResult(result);

		result = dxgiAdapter->GetParent(__uuidof(IDXGIFactory2), &dxgiFactory);
		assertResult(result);

		DXGI_SWAP_CHAIN_DESC1 swapchainDesc{0};
		swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
		swapchainDesc.BufferCount = 2;
		swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapchainDesc.Scaling = DXGI_SCALING_NONE;
		swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
		swapchainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		swapchainDesc.Stereo = FALSE;
		swapchainDesc.Width = 0;
		swapchainDesc.Height = 0;
		swapchainDesc.SampleDesc.Count = 1;
		swapchainDesc.SampleDesc.Quality = 0;
		swapchainDesc.Flags = 0;

		result = dxgiFactory->CreateSwapChainForHwnd(d3d11Device.Get(), window.getHandle(), &swapchainDesc, NULL, NULL, &swapchain);
		assertResult(result);

		D2D1_FACTORY_OPTIONS options{};
		result = D2D1CreateFactory<ID2D1Factory1>(D2D1_FACTORY_TYPE_SINGLE_THREADED, options, &d2d1Factory);
		assertResult(result);

		result = d2d1Factory->CreateDevice(dxgiDevice.Get(), &d2d1Device);
		assertResult(result);

		result = d2d1Device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &d2d1Context);
		assertResult(result);

		result = swapchain->GetBuffer(0, IID_PPV_ARGS(&dxgiBackBuffer));
		assertResult(result);

		const auto bitmapProps = BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW, PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), DPI, DPI);
		result = d2d1Context->CreateBitmapFromDxgiSurface(dxgiBackBuffer.Get(), bitmapProps, &d2d1TargetBitmap);
		assertResult(result);

		d2d1Context->SetTarget(d2d1TargetBitmap.Get());

		result = d2d1Context->CreateSolidColorBrush(ColorF(ColorF::Red), &d2d1BrushBlack);
		assertResult(result);
	}

	ComPtr<ID3D11Texture2D> GrabScreenContent(UINT outputNum) const {
		//const auto [width, height] = renderingWindow.getSize();

		//auto dcScreen = GetDC(NULL);
		//auto dcTarget = CreateCompatibleDC(dcScreen);
		//auto bmpTarget = CreateCompatibleBitmap(dcScreen, width, height);
		//auto oldBmp = SelectObject(dcTarget, bmpTarget);
		//const auto result = BitBlt(dcTarget, 0, 0, width, height, dcScreen, 0, 0, SRCCOPY | CAPTUREBLT);
		//if (!result) {
		//	throw std::exception("Could not blit screen grab!");
		//}

		//SelectObject(dcTarget, oldBmp);
		//DeleteObject(dcTarget);
		//ReleaseDC(NULL, dcScreen);
		////TODO wrap bitmap in smart pointer and return it.

		//auto bitmap = GDIObject<HBITMAP>(bmpTarget);
		//return std::move(bitmap);
		
		HRESULT result;

		ComPtr<IDXGIOutput> output;
		result = dxgiAdapter->EnumOutputs(outputNum, &output);
		assertResult(result);

		ComPtr<IDXGIOutput1> outputV1;
		result = output->QueryInterface<IDXGIOutput1>(&outputV1);
		assertResult(result);

		ComPtr<IDXGIOutputDuplication> duplication;
		result = outputV1->DuplicateOutput(d3d11Device.Get(), &duplication);
		assertResult(result);

		ComPtr<IDXGIResource> desktopResource;
		DXGI_OUTDUPL_FRAME_INFO duplFrameInfo{0};
		result = duplication->AcquireNextFrame(30, &duplFrameInfo, &desktopResource);
		assertResult(result);

		ComPtr<ID3D11Texture2D> desktopImage;
		result = desktopResource->QueryInterface<ID3D11Texture2D>(&desktopImage);
		assertResult(result); 

		return std::move(desktopImage);
	}

	void DrawFullscreenRect() const noexcept {
		const auto windowSize = renderingWindow.getSize();

		//testing
		static auto screenTexture = GrabScreenContent(0);

		d2d1Context->BeginDraw();
		//d2d1Context->DrawBitmap(screenTexture, RectF(800, 800, 1200, 1200));
		d2d1Context->FillRectangle(RectF(0, 0, windowSize.width - 1500, windowSize.height - 500), d2d1BrushBlack.Get()); 

		HRESULT result;
		result = d2d1Context->EndDraw();
		assertResult(result);
		result = swapchain->Present(1, 0);
		assertResult(result);
	}
};