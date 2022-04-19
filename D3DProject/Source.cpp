#include <iostream>
#include "Window.h"
#include "dx12core.h"

int main()
{
	std::cout << "Hello World\n";

	Window window(1280, 720, L"Project", L"Project");

	

	bool window_exist = true;
	while (window_exist)
	{
		window_exist = window.WinMsg();
		if (!window_exist)
			break;
	}

	//dx12core::GetDx12Core();

	return 0;
}