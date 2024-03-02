module;
#include <string>
#include <unordered_set>
#include <Windows.h>  
#include <exception>
#include <type_traits>
#include "MacroUtils.h"
export module Window;

import Optional;
import Event;

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


	static inline Event<> OnKeyboardMouseEvent;

private:
	inline static std::unordered_set<std::wstring> classes;

	HINSTANCE hInstance;
	std::wstring title;
	Style style;
	std::wstring windowClass;
	HWND windowHandle;
	bool closed = false;

	Size currentSize;

	const DWORD defaultWindowExFlags = WS_EX_NOREDIRECTIONBITMAP | WS_EX_NOACTIVATE | WS_EX_TOPMOST;

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
	// Currently only returns a boolean if keyboard or mouse input was detected.
	static bool ReadRawInput(HRAWINPUT input) {
		UINT dwSize;
		GetRawInputData(input, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
		auto inputBuf = new BYTE[dwSize];
		if (inputBuf == 0) {
			throw;
		}

		if (GetRawInputData(input, RID_INPUT, inputBuf, &dwSize, sizeof(RAWINPUTHEADER)) != dwSize) {
			VSTUDIO_DEBUG_OUTPUT("GetRawInputData did not return correct size!");
		}

		auto rawBuf = reinterpret_cast<RAWINPUT*>(inputBuf);

		if (rawBuf->header.dwType == RIM_TYPEKEYBOARD) {
			//TODO: read input.
			delete[] inputBuf;
			return true;
		}
		else if (rawBuf->header.dwType == RIM_TYPEMOUSE) {
			//TODO: read input
			delete[] inputBuf;
			return true;
		}

		delete[] inputBuf;
		return false;
	}

	static LPARAM CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
		auto me = reinterpret_cast<Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
		switch (msg) {
			case WM_INPUT:
			{
				if (ReadRawInput((HRAWINPUT)lParam)) {
					OnKeyboardMouseEvent.Invoke();
				}
				break;
			}
			case WM_CLOSE:
			{
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

	void RegisterRawInput(HWND target) const {
		RAWINPUTDEVICE mouse, keyboard;

		mouse = {
			.usUsagePage = 0x01, //GENERIC
			.usUsage = 0x02, //GENERIC_MOUSE
			.dwFlags = RIDEV_INPUTSINK | RIDEV_NOLEGACY,
			.hwndTarget = target,
		};

		keyboard = {
			.usUsagePage = 0x01, //GENERIC
			.usUsage = 0x06, //GENERIC_KEYBOARD
			.dwFlags = RIDEV_INPUTSINK | RIDEV_NOLEGACY,
			.hwndTarget = target,
		};

		RAWINPUTDEVICE devices[] = {mouse, keyboard};

		if (!RegisterRawInputDevices(devices, 2, sizeof(RAWINPUTDEVICE))) {
			throw std::exception("Could not register input devices.");
		}
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

		windowHandle = CreateWindowEx(defaultWindowExFlags, className.c_str(), title.c_str(), WS_POPUP, 0, 0, screenX, screenY, NULL, NULL, hInstance, NULL);
		SetWindowLongPtr(windowHandle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
		if (!windowHandle) {
			throw std::exception("Unable to create window");
		}

		currentSize = {
			.width = screenX,
			.height = screenY,
		};

		RegisterRawInput(windowHandle);
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

	inline void Restore() const noexcept {
		ShowWindow(windowHandle, SW_RESTORE);
	}

	inline void Minimize() const noexcept {
		ShowWindow(windowHandle, SW_MINIMIZE);
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

	void DisableMousePassthrough() noexcept {
		SetWindowLongPtr(windowHandle, GWL_EXSTYLE, defaultWindowExFlags & ~(WS_EX_TRANSPARENT | WS_EX_LAYERED));
	}

	void EnableMousePassthrough() noexcept {
		SetWindowLongPtr(windowHandle, GWL_EXSTYLE, defaultWindowExFlags | WS_EX_TRANSPARENT | WS_EX_LAYERED);
	}

	inline void SetCursorVisibility(bool visible) const noexcept {
		auto val = ShowCursor(visible);
		VSTUDIO_DEBUG_OUTPUT("CursorVis value = " << val);
	}

	void Close() noexcept {
		if (closed) return;
		DestroyWindow(windowHandle);
		classes.erase(windowClass);
		closed = true;
	}
};