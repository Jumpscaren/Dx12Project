#include "Window.h"

LRESULT CALLBACK HandleMsg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CLOSE:
    {
        //m_isClosed = true;
        //std::cout << "WM_CLOSE" << std::endl;
        DestroyWindow(hwnd);
        return 0;
    }
    case WM_DESTROY:
    {
        //std::cout << "WM_DESTROY" << std::endl;
        PostQuitMessage(0);
        return 0;
    }
    }

    return DefWindowProc(hwnd, message, wParam, lParam);
}

Window::Window(UINT width, UINT height, std::wstring window_name, std::wstring window_text)
{
    m_window_height = height;
    m_window_width = width;

    WNDCLASSEX wc = { };

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.hInstance = m_window_hinstance;
    wc.lpszClassName = window_name.c_str();
    //Set msg handler to the wndproc
    wc.lpfnWndProc = HandleMsg;//[](HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {return WindowProc(hwnd, uMsg, wParam, lParam);};

    RegisterClassEx(&wc);

    RECT rt = { 0, 0, static_cast<LONG>(m_window_width),
        static_cast<LONG>(m_window_height) };
    AdjustWindowRect(&rt, WS_OVERLAPPEDWINDOW, FALSE);

    m_window_handle = CreateWindowEx(0, window_name.c_str(), window_text.c_str(),
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, rt.right - rt.left, rt.bottom - rt.top,
        nullptr, nullptr, m_window_hinstance, nullptr);

    if (m_window_handle == nullptr)
    {
        DWORD errorCode = GetLastError();
    }

    ShowWindow(m_window_handle, SW_SHOWNORMAL);
}

bool Window::WinMsg()
{
    MSG msg{};
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (msg.message == WM_QUIT)
        {
            return false;
        }
    }
    return true;
}

HWND Window::GetWindowHandle() const
{
    return m_window_handle;
}

float Window::GetWindowHeight() const
{
    return m_window_height;
}

float Window::GetWindowWidth() const
{
    return m_window_width;
}
