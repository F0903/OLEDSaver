module;
#include <d3d11.h>
#include <d2d1.h>
#include <d2d1_1.h>
#include <d2d1_1helper.h>
#include <wrl.h>
#include <exception>
export module D2D1Renderer;
using namespace Microsoft::WRL;
using namespace D2D1;

import ErrorHandling;

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
	
	ComPtr<ID3D11Device> device;
	ComPtr<ID3D11DeviceContext> context;

	ComPtr<ID2D1Factory1> d2d1Factory;
	ComPtr<ID2D1Device> d2d1Device;
	ComPtr<ID2D1DeviceContext> d2d1DeviceContext;
	
	//ComPtr<IDXGIFactory> dxgiFactory;
	ComPtr<IDXGIDevice> dxgiDevice;
	ComPtr<IDXGISurface> dxgiBackBuffer;

	static inline void assertResult(HRESULT result) {
		if (result != S_OK) {
			throw std::exception("Error");
		}
	}

	D2D1Renderer() {
		initD3D11Device();
		initDXGIDevice();

		HRESULT result;

		D2D1_FACTORY_OPTIONS options{};
		result = D2D1CreateFactory<ID2D1Factory1>(D2D1_FACTORY_TYPE_SINGLE_THREADED, options, &d2d1Factory);
		assertResult(result);

		result = d2d1Factory->CreateDevice(dxgiDevice.Get(), &d2d1Device);
		assertResult(result);

		result = d2d1Device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &d2d1DeviceContext);
		assertResult(result);

		auto bitmapProps = BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW, PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), DPI, DPI);
	

	}

	void initD3D11Device() { 
		HRESULT result;

		UINT deviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(DEBUG) || defined(_DEBUG)
		deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
		result = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, 0, deviceFlags, featureLevels, ARRAYSIZE(featureLevels), D3D11_SDK_VERSION, &device, &selectedFeatureLevel, &context);
		assertResult(result);

		if (device->GetFeatureLevel() < APP_MIN_FEATURE_LEVEL) {
			errorPopUp(L"Device feature level below app minimum!");
			return;
		}
	}

	void initDXGIDevice() {
		HRESULT result;

		result = device->QueryInterface<IDXGIDevice>(&dxgiDevice);
		assertResult(result);

		/*for (UINT i = 0;; i++) {
			ComPtr<IDXGIAdapter1> adapter;
			result = dxgiFactory->EnumAdapters1(i, &adapter);
			assertResult(result);

			DXGI_ADAPTER_DESC1 adapterDesc{};
			result = adapter->GetDesc1(&adapterDesc);
			assertResult(result);


		}*/
	} 
};