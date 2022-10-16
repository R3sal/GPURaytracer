
#include <iostream>
#include <chrono>
#include <thread>

#include "Settings.h"
#include "GPUDevice.h"
#include "RaytracerPipeline.h"



using namespace std::chrono_literals;


std::atomic_bool bAppShouldRun = true;

//run the rendering on a separate thread to increase application responsiveness
void RenderFunction(RT::GraphicsAPI::RaytracerPipeline* rtTracer)
{
	while (bAppShouldRun)
	{
		if (!rtTracer->Render()) std::cout << "Error while rendering\n";
	}
	std::cout << "\nThe raytracer successfully finished computing the image\n";
}



int main()
{
	RT::GraphicsAPI::WND::Window rtWindow = RT::GraphicsAPI::WND::Window();
	RT::GraphicsAPI::DX12Device rtDevice = RT::GraphicsAPI::DX12Device();
	RT::GraphicsAPI::RaytracerPipeline rtTracer = RT::GraphicsAPI::RaytracerPipeline();

	//the initialization
	if (!(rtWindow.Initialize(RT_WINDOW_WIDTH, RT_WINDOW_HEIGHT)))
	{
		std::cout << "An error occured during window initialization\n";
		std::cin.get();
		return 0;
	}
	std::cout << "The window was initialized successfully\n";
	if (!(rtDevice.Initialize(&rtWindow)))
	{
		std::cout << "An error occured during device initialization\n";
		std::cin.get();
		return 0;
	}
	std::cout << "DirectX was initialized successfully\n";
	if (!(rtTracer.Initialize(&rtDevice, RT::GraphicsAPI::LoadMeshFromFile(RT_SCENE_FILENAME))))
	{
		std::cout << "An error occured during pipeline initialization\n";
		std::cin.get();
		return 0;
	}
	std::cout << "The raytracer pipeline was initialized successfully\n\n";

	//get a time point to measur the time, that passed since the start of the program
	std::chrono::steady_clock stdClock;
	auto stdStartTime = stdClock.now();

	
	//the main loop
	std::thread stdRenderThread(RenderFunction, &rtTracer);
	while (!(rtWindow.ShouldClose()))
	{
		//react to window events
		rtWindow.Update();
		std::this_thread::sleep_for(1ms);

		//stop the raytracer if we exceeded the time limit
		auto stdCurrentTime = stdClock.now();
		auto iElapsedTime = (std::chrono::duration_cast<std::chrono::microseconds>(stdCurrentTime - stdStartTime)).count();
		if (((float)iElapsedTime * 0.000001f) >= RT_MAX_SECONDS)
			bAppShouldRun = false;
	}

	//indicate, that the render thread should finish
	bAppShouldRun = false;

	//wait for the rendering to finish
	stdRenderThread.join();

	return 0;
}