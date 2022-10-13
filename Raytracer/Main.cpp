
#include <iostream>
#include <chrono>
#include <thread>

#include "GPUDevice.h"
#include "RaytracerPipeline.h"



using namespace std::chrono_literals;


//std::atomic_bool bAppShouldRun = true;
//std::atomic_bool bAppRunning = true;



int main()
{
	RT::GraphicsAPI::WND::Window rtWindow = RT::GraphicsAPI::WND::Window();
	std::cout << rtWindow.Initialize(RT::GraphicsAPI::WINDOW_WIDTH, RT::GraphicsAPI::WINDOW_HEIGHT) << "\n";

	RT::GraphicsAPI::DX12Device rtDevice = RT::GraphicsAPI::DX12Device();
	std::cout << rtDevice.Initialize(&rtWindow, RT::GraphicsAPI::NUM_BUFFERS) << "\n";

	RT::GraphicsAPI::RaytracerPipeline rtTracer = RT::GraphicsAPI::RaytracerPipeline();
	std::cout << rtTracer.Initialize(&rtDevice, RT::GraphicsAPI::LoadMeshFromFile("testobject5_1.obj")) << "\n";
	
	//std::thread(RenderFunction);
	std::chrono::steady_clock stdClock;

	while (!(rtWindow.ShouldClose()))
	{
		rtWindow.Update();
		//std::chrono::time_point<std::chrono::steady_clock, std::chrono::milliseconds> tp;
		auto stdFirstTimePoint = stdClock.now();
		if (!rtTracer.Render()) std::cout << "error\n";
		auto stdSecondTimePoint = stdClock.now();
		unsigned int iElapsedTime = (std::chrono::duration_cast<std::chrono::microseconds>(stdSecondTimePoint - stdFirstTimePoint)).count();
		//std::cout << (std::chrono::duration_cast<std::chrono::milliseconds>(stdSecondTimePoint - stdFirstTimePoint)) << "\n";
		std::cout << (float)(iElapsedTime / 1000000.0f) << "\n";
	}
	//bAppShouldRun = false;

	//wait for the rendering to finish
	//while (bAppRunning) {}

	return 0;
}


//void RenderFunction(RT::GraphicsAPI::RaytracerPipeline& rtTracer)
//{
//	while (bAppShouldRun)
//	{
//		if (!rtTracer.Render()) std::cout << "error\n";
//	}
//	bAppRunning = false;
//}