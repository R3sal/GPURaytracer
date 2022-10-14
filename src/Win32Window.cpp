//include-files
#include "Win32Window.h"



//define a function for processing window events
LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	switch (iMessage)
	{
	case WM_CLOSE:
		break;
	}

	return DefWindowProc(hWnd, iMessage, wParam, lParam);
}



namespace RT::GraphicsAPI::Win32
{

	//AppWindow constructor and destructor
	//constructor: initializes all the variables (at least with "0", "nullptr" or "")
	Window::Window() :
		//initialize the class variables
		m_hWindow(nullptr),
		m_iWidth(0),
		m_iHeight(0),
		m_bWindowShouldClose(false)
	{

	}

	//destructor: uninitializes all our pointers
	Window::~Window()
	{
		Release();
	}



	//private class functions




	//public class functions
	bool Window::Initialize(int iWidth, int iHeight)
	{
		//define teh style (properties) of the window
		UINT iWindowStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
		
		//set the dimensions of the window
		RECT WindowRect{};
		if (!SetRect(&WindowRect, 0, 0, iWidth, iHeight)) return false;
		if (!AdjustWindowRectEx(&WindowRect, iWindowStyle, false, 0)) return false;

		//create a window class
		WNDCLASSEX WndClass{};
		WndClass.style = CS_HREDRAW | CS_VREDRAW;
		WndClass.lpfnWndProc = WndProc;
		WndClass.cbSize = sizeof(WNDCLASSEX);
		WndClass.cbClsExtra = 0;
		WndClass.cbWndExtra = 0;
		WndClass.hInstance = nullptr;
		WndClass.hIcon = LoadIcon(nullptr, IDI_WINLOGO);
		WndClass.hIconSm = LoadIcon(nullptr, IDI_WINLOGO);
		WndClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
		WndClass.hbrBackground =  nullptr;
		WndClass.lpszMenuName = nullptr;
		WndClass.lpszClassName = L"Raytracer";

		//register the window class
		if (!RegisterClassEx(&WndClass)) return false;

		//create the window
		m_hWindow = CreateWindow(L"Raytracer", L"Raytracer", iWindowStyle, CW_USEDEFAULT, CW_USEDEFAULT, iWidth, iHeight, nullptr, nullptr, nullptr, nullptr);
		if (!m_hWindow) return false;


		//show and update the window
		ShowWindow(m_hWindow, SW_SHOWDEFAULT);
		UpdateWindow(m_hWindow);


		return true;
	}


	//update the window
	void Window::Update()
	{
		return;
	}


	//release all the pointers in connection to the DXGI-device
	void Window::Release()
	{
		
	}
}