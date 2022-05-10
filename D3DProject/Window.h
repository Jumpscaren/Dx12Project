#pragma once
#include <Windows.h>
#include <string>

class Window
{
private:
	UINT m_window_height, m_window_width;
	HWND m_window_handle;
	HINSTANCE m_window_hinstance;

private:
	//LRESULT CALLBACK HandleMsg(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

public:
	Window() = delete;
	Window(UINT width, UINT height, std::wstring window_name, std::wstring window_text);
	bool WinMsg();
	HWND GetWindowHandle() const;
	float GetWindowHeight() const;
	float GetWindowWidth() const;
};

