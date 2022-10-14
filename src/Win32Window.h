#pragma once

#define WND Win32

//include the windows header
#include <Windows.h>



namespace RT::GraphicsAPI::Win32
{

	class Window
	{
	private:

		//something???
		HWND		m_hWindow;
		int		m_iWidth;
		int		m_iHeight;
		bool		m_bWindowShouldClose;


	public: // = usable outside of the class

		//constructor and destructor
		Window();
		~Window();


		//public class functions
		bool Initialize(int iWidth = 800, int iHeight = 600);
		//bool ResizeBuffers();
		//void SetFullscreen(bool bGoFullscreen);
		void Update(); // ????
		void Release();


		//helper functions
		HWND GetHWND() { return m_hWindow; };
		int GetWidth() const { return m_iWidth; };
		int GetHeight() const { return m_iHeight; };
		bool CloseWindow() { m_bWindowShouldClose = true; }
		bool ShouldClose() { return m_bWindowShouldClose; };

	};
}