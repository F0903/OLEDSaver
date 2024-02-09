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
				auto me = reinterpret_cast<Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
				me->Close();
				break;
			}
			case WM_DESTROY:
			{
				PostQuitMessage(0);
				break;
			}
			default: return DefWindowProc(hwnd, msg, wParam, lParam);
		}
		return NULL;
	}

	const std::wstring RegisterWindowClass(const std::wstring& windowTitle) {
		std::wstring className = windowTitle;
		className.append(L"Window");
		if (classes.contains(className)) {
			return std::move(className);
		}

		const auto icon = LoadIcon(hInstance, IDI_APPLICATION);
		WNDCLASSEX windowClassDesc{0};
		windowClassDesc.cbSize = sizeof(WNDCLASSEX);
		windowClassDesc.lpfnWndProc = WndProc;
		windowClassDesc.hInstance = hInstance;
		windowClassDesc.hIcon = icon;
		windowClassDesc.lpszClassName = className.c_str();
		windowClassDesc.hIconSm = icon;

		if (!RegisterClassEx(&windowClassDesc)) {
			throw std::exception("Could not register window class!");
		}

		classes.insert(className);

		return className;
	}

	void CreateFullscreenWindow() {
		const auto className = RegisterWindowClass(title);

		const auto screenX = GetSystemMetrics(SM_CXSCREEN);
		const auto screenY = GetSystemMetrics(SM_CYSCREEN);

		constexpr auto windowExFlags = WS_EX_NOREDIRECTIONBITMAP;
		windowHandle = CreateWindowEx(DEBUG_VALUE(windowExFlags, windowExFlags | WS_EX_TOPMOST), className.c_str(), title.c_str(), WS_POPUP, 0, 0, screenX, screenY, NULL, NULL, hInstance, NULL);
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