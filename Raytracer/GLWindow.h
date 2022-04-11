#pragma once

//include the glfw and dxgi headers
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#define WND GLFW

//link to the glfw library
#ifdef x86
#pragma comment(lib, "x86/GLFW/glfw3")
#endif
#ifdef x64
#pragma comment(lib, "x64/GLFW/glfw3")
#endif



namespace RT::GraphicsAPI::GLFW
{
	
	class Window
	{
	private:

		//something???
		GLFWmonitor*		m_glMonitor;
		GLFWwindow*		m_glWindow;
		int		m_iWidth;
		int		m_iHeight;
		

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
		GLFWwindow* GetGLFWwindow() { return m_glWindow; };
		HWND GetHWND() { return glfwGetWin32Window(m_glWindow); };
		int GetWidth() const { return m_iWidth; };
		int GetHeight() const { return m_iHeight; };
		bool ShouldClose() { return glfwWindowShouldClose(m_glWindow); };

	};
}