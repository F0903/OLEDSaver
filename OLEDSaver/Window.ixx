module;
#include <string>
#include <unordered_set>
#include <Windows.h>  
#include <exception>
#include <type_traits>
#include "MacroUtils.h"
export module Window;

import Optional; 

export const unsigned int GetDisplayRefreshHz() {
	DEVMODE deviceMode{0};
	deviceMode.dmSize = sizeof(DEVMODE);
	if (!EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &deviceMode)) {
		throw std::exception("Could not get current display configuration!");
	}
	return deviceMode.dmDisplayFrequency;
}

export class Window
{
public:
	enum class Style
	{
		Normal, //TODO
		Fullscreen
	};

	struct Size
	{
		int width;
		int height;
	};

private:
	inline static std::unordered_set<std::wstring> classes;

	HINSTANCE hInstance;
	std::wstring title;
	Style style;
	std::wstring windowClass;
	HWND windowHandle;
	bool closed = false;

	Size currentSize;

public:
	Window(HINSTANCE hInstance, const std::wstring& title, Style style) : hInstance(hInstance), title(title), style(style) {
		switch (style) {
			case Window::Style::Normal:
				//TODO
				break;
			case Window::Style::Fullscreen:
				CreateFullscreenWindow();
				break;
		}
	}

	~Window() {
		Close();
	}

private:
	static LPARAM CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
		switch (msg) {
			case WM_CLOSE:
			{
				auto window = reinterpret_cast<Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
				window->Close();
				break;
			}
			case WM_DESTROY:
			{
				PostQuitMessage(0);
				break;
			}
			default:
				return DefWindowProc(hwnd, msg, wParam, lParam);
		}
	}

	const std::wstring RegisterWindowClass(const std::wstring& windowTitle) {
		std::wstring className = windowTitle;
		className.append(L"Window");
		if (classes.contains(className)) {
			return std::move(className);
		}

		const auto icon = LoadIcon(hInstance, IDI_APPLICATION);
		WNDCLASSEX windowClass{0};
		windowClass.cbSize = sizeof(WNDCLASSEX);
		windowClass.style = 0;
		windowClass.lpfnWndProc = WndProc;
		windowClass.hInstance = hInstance;
		windowClass.hIcon = icon;
		windowClass.hCursor = LoadCursor(hInstance, IDC_ARROW);
		windowClass.lpszMenuName = NULL;
		windowClass.lpszClassName = className.c_str();
		windowClass.hIconSm = icon;

		if (!RegisterClassEx(&windowClass)) {
			throw std::exception("Could not register window class!");
		}

		classes.insert(className);

		return std::move(className);
	}

	void CreateFullscreenWindow() {
		const auto className = RegisterWindowClass(title);

		const auto screenX = GetSystemMetrics(SM_CXSCREEN);
		const auto screenY = GetSystemMetrics(SM_CYSCREEN);

		windowHandle = CreateWindowEx(DEBUG_VALUE(0, WS_EX_TOPMOST), className.c_str(), title.c_str(), WS_POPUP, 0, 0, screenX, screenY, NULL, NULL, hInstance, NULL);
		SetWindowLongPtr(windowHandle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
		if (!windowHandle) {
			throw std::exception("Unable to create window");
		}

		currentSize = {
			.width = screenX,
			.height = screenY,
		};
	}

public:
	void Update() const {
		if (closed) {
			throw std::exception("Window is closed!");
		}
		const auto result = UpdateWindow(windowHandle);
		if (!result) {
			throw std::exception("Could not update window!");
		}
	}

	void Show() const {
		ShowWindow(windowHandle, SW_SHOW);
	}

	inline const bool IsClosed() const noexcept {
		return closed;
	}

	inline HWND GetHandle() const noexcept {
		return windowHandle;
	}

	inline Size GetSize() const noexcept {
		return currentSize;
	}

	inline void SetCursorVisibility(bool visible) const noexcept {
		ShowCursor(visible);
	}

	void Close() {
		if (closed) return;
		DestroyWindow(windowHandle);
		UnregisterClass(windowClass.c_str(), hInstance);
		classes.erase(windowClass);
		closed = true;
	}
};