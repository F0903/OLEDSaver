module;
#include <string>
#include <unordered_set>
#include <Windows.h> 
#include <optional>

export module Window;

import Optional; 

export class Window {
public:
	enum class Style {
		Normal, //TODO
		Fullscreen
	};

private:
	HINSTANCE hInstance;
	std::wstring title;
	Style style;

	std::unordered_set<std::wstring> classes;

	HWND windowHandle;

	Window(HINSTANCE hInstance, const std::wstring& title, Style style) : hInstance(hInstance), title(title), style(style), classes() {
		switch (style)
		{
		case Window::Style::Normal:
			//TODO
			break;
		case Window::Style::Fullscreen:
			createFullscreenWindow();
			break;
		}
	} 

public:
	~Window() {
		DestroyWindow(windowHandle);
	}

	static Window Normal(HINSTANCE hInstance, const std::wstring& title, int width, int height, int x, int y) {
		//TODO
		throw std::exception("Not implemented.");
	}

	static Window Fullscreen(HINSTANCE hInstance, const std::wstring& title) {
		return Window(hInstance, title, Window::Style::Fullscreen);
	}

private:
	static LPARAM CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
		switch (msg)
		{
		case WM_CLOSE:
			DestroyWindow(hwnd);
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hwnd, msg, wParam, lParam);
		}
	}

	const Optional<std::wstring> registerWindowClass(const std::wstring& windowTitle) {
		auto className = windowTitle;
		className.append(L"Window");
		if (classes.contains(className)) {
			return Optional<std::wstring>::None();
		}
		
		const auto icon = LoadIcon(hInstance, IDI_APPLICATION);
		WNDCLASSEX windowClass{ 0 };
		windowClass.cbSize = sizeof(WNDCLASSEX);
		windowClass.style = 0;
		windowClass.lpfnWndProc = WndProc;
		windowClass.hInstance = hInstance;
		windowClass.hIcon = icon;
		windowClass.hCursor = LoadCursor(hInstance, IDC_ARROW);
		windowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		windowClass.lpszMenuName = NULL;
		windowClass.lpszClassName = className.c_str();
		windowClass.hIconSm = icon;

		if (!RegisterClassEx(&windowClass)) {
			throw std::exception("Could not register window class!");
		}

		classes.insert(className);

		return Optional<std::wstring>::Value(std::move(className));
	}

	HWND createFullscreenWindow() {
		const auto className = registerWindowClass(title).unwrap_value();

		windowHandle = CreateWindowEx(WS_EX_TOPMOST, className.c_str(), title.c_str(), WS_MAXIMIZE | WS_POPUPWINDOW | WS_VISIBLE, 0, 0, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);
		if (!windowHandle) {
			throw std::exception("Unable to create window");
		}
	}

public:
	
};