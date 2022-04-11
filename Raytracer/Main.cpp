
#include <iostream>
#include <chrono>
#include <thread>


#include "DX12Device.h"
#include "DX12RaytracingPipeline.h"



int main()
{
	RT::GraphicsAPI::WND::Window rtWindow = RT::GraphicsAPI::WND::Window();
	std::cout << rtWindow.Initialize(1920, 1080) << "\n";

	RT::GraphicsAPI::DX12Device rtDevice = RT::GraphicsAPI::DX12Device();
	std::cout << rtDevice.Initialize(rtWindow) << "\n";

	RT::GraphicsAPI::RaytracingPipeline rtTracer = RT::GraphicsAPI::RaytracingPipeline();
	std::cout << rtTracer.Initialize(rtDevice) << "\n";
	//rtTracer.Render();

	while (!(rtWindow.ShouldClose()))
	{
		rtWindow.Update();
		if (!rtTracer.Render()) std::cout << "error\n";
	}
}