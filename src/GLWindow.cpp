//include-files
#include "GLWindow.h"



namespace RT::GraphicsAPI::GLFW
{

	//AppWindow constructor and destructor
	//constructor: initializes all the variables (at least with "0", "nullptr" or "")
	Window::Window() :
		//initialize the class variables
		m_glMonitor(nullptr),
		m_glWindow(nullptr),
		m_iWidth(0),
		m_iHeight(0)
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
		if (!glfwInit()) return false;

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
		glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
		glfwWindowHint(GLFW_FOCUSED, GLFW_TRUE);
		glfwWindowHint(GLFW_AUTO_ICONIFY, GLFW_TRUE);
		glfwWindowHint(GLFW_FLOATING, GLFW_FALSE);
		glfwWindowHint(GLFW_MAXIMIZED , GLFW_FALSE);
		glfwWindowHint(GLFW_CENTER_CURSOR, GLFW_TRUE);
		glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
		glfwWindowHint(GLFW_FOCUS_ON_SHOW, GLFW_TRUE);
		glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_FALSE);

		m_glMonitor = glfwGetPrimaryMonitor();
		m_iWidth = iWidth;
		m_iHeight = iHeight;
		m_glWindow = glfwCreateWindow(m_iWidth, m_iHeight, "Raytracer", nullptr, nullptr);
		if (!m_glWindow) return false;

		return true;
	}


	//update the window
	void Window::Update()
	{
		glfwPollEvents();
		return;
	}


	//release all the pointers in connection to the DXGI-device
	void Window::Release()
	{
		glfwDestroyWindow(m_glWindow);
		glfwTerminate();
	}
}