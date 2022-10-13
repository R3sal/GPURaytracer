//include-files
#include "GPUScheduler.h"
#include <thread>



namespace RT::GraphicsAPI
{

	/*void WaitForFence(ID3D12Fence1* d3dFenceToWaitFor, UINT64 iCompletionValue, DWORD iMaxWaitingTime)
	{
		//create an event, that is raised upon task completion
		HANDLE hEventOnFinish = CreateEvent(nullptr, false, false, nullptr);
		if (!hEventOnFinish) return; //error

		//wait for GPU to complete its execution
		if (d3dFenceToWaitFor->GetCompletedValue() < iCompletionValue)
		{
			d3dFenceToWaitFor->SetEventOnCompletion(iCompletionValue, hEventOnFinish);
			WaitForSingleObject(hEventOnFinish, iMaxWaitingTime);
		}

		//close the event handle to avoid memory leaks
		CloseHandle(hEventOnFinish);
	}*/


	void WaitForFence(ID3D12Fence1* d3dFenceToWaitFor, UINT64 iCompletionValue, DWORD iMaxWaitingTime)
	{
		using namespace std::chrono_literals;

		//wait for GPU to complete its execution
		while (d3dFenceToWaitFor->GetCompletedValue() < iCompletionValue)
		{
			std::this_thread::sleep_for(1us);
		}
	}


	void WaitForFence(ID3D12Fence1* d3dFenceToWaitFor, HANDLE hEventOnFinish, UINT64 iCompletionValue, DWORD iMaxWaitingTime)
	{
		//wait for GPU to complete its execution
		if (d3dFenceToWaitFor->GetCompletedValue() < iCompletionValue)
		{
			d3dFenceToWaitFor->SetEventOnCompletion(iCompletionValue, hEventOnFinish);
			WaitForSingleObject(hEventOnFinish, iMaxWaitingTime);
		}
	}



	//GPUScheduler constructor and destructor
	//constructor: initializes all the variables (at least with "0", "nullptr" or "")
	GPUScheduler::GPUScheduler() :
		//initialize the class variables
		m_rtDevice(nullptr),
		m_d3dCommandList(nullptr),
		m_d3dCommandAllocators(nullptr),
		m_d3dFences(nullptr),
		m_hFenceCompletedEvents(nullptr),
		m_iFenceValues(nullptr),
		m_iCurrentFenceValue(0),
		m_iCurrentTaskStack(0),
		m_iNumMaxTaskRecorders(0)
	{

	}

	//destructor: uninitializes all our pointers
	GPUScheduler::~GPUScheduler()
	{
		Release();
	}



	//private class functions



	//public class functions
	bool GPUScheduler::Initialize(DX12Device* rtDevice, unsigned int iNumMaxTaskRecorders)
	{
		//set the member variables to their appropriate values
		m_rtDevice = rtDevice;
		m_iNumMaxTaskRecorders = iNumMaxTaskRecorders;
		ID3D12Device8* d3dDevice = m_rtDevice->GetDevice();
		IDXGISwapChain4* dxSwapChain = m_rtDevice->GetSwapChain();


		//create some resources, such as command allocators and lists
		m_d3dCommandAllocators = new ID3D12CommandAllocator*[m_iNumMaxTaskRecorders];
		if (!m_d3dCommandAllocators) return false;
		m_d3dFences = new ID3D12Fence1*[m_iNumMaxTaskRecorders];
		if (!m_d3dFences) return false;
		m_hFenceCompletedEvents = new HANDLE[m_iNumMaxTaskRecorders];
		if (!m_hFenceCompletedEvents) return false;
		m_iFenceValues = new UINT64[m_iNumMaxTaskRecorders];
		if (!m_iFenceValues) return false;

		if (d3dDevice->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS((ID3D12CommandList**)(&m_d3dCommandList))) < 0) return false;
		for (unsigned int i = 0; i < m_iNumMaxTaskRecorders; i++)
		{
			//create the command allocators and command lists
			if (d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&(m_d3dCommandAllocators[i]))) < 0) return false;
			//create the frame fences
			if (d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&(m_d3dFences[i]))) < 0) return false;

			//create empty events which will be set upon fence completion
			m_hFenceCompletedEvents[i] = CreateEvent(nullptr, false, false, nullptr);
			if (!(m_hFenceCompletedEvents[i])) return false;
			//set the fence values to 0 (doesn't really matter as they will be set before being usedanyways)
			m_iFenceValues[i] = 0;
		}


		return true;
	}


	void GPUScheduler::Flush()
	{
		for (unsigned int i = 0; i < m_iNumMaxTaskRecorders; i++)
		{
			WaitForFence(m_d3dFences[i], m_hFenceCompletedEvents[i], m_iFenceValues[i]);
		}
	}


	bool GPUScheduler::Record()
	{
		//wait for the previous commands to finish
		unsigned int iPreviousTaskIndex = GetPreviousTaskIndex();
		WaitForFence(m_d3dFences[iPreviousTaskIndex], m_hFenceCompletedEvents[iPreviousTaskIndex], m_iFenceValues[iPreviousTaskIndex]);

		//start recording commands
		if (m_d3dCommandAllocators[m_iCurrentTaskStack]->Reset() < 0) return false;
		if (m_d3dCommandList->Reset(m_d3dCommandAllocators[m_iCurrentTaskStack], nullptr) < 0) return false;

		return true;
	}


	bool GPUScheduler::Execute()
	{
		//save the command queue in a local variable for convenience
		ID3D12CommandQueue* d3dCommandQueue = m_rtDevice->GetCommandQueue();

		//go to the next command allocator
		unsigned int iLastTaskStack = m_iCurrentTaskStack;
		m_iCurrentTaskStack = (m_iCurrentTaskStack + 1) % m_iNumMaxTaskRecorders;

		//close the command list
		if (m_d3dCommandList->Close() < 0) return false;

		//execute the commands
		ID3D12CommandList* d3dCommandLists[] = { m_d3dCommandList };
		d3dCommandQueue->ExecuteCommandLists(1, d3dCommandLists);

		//make a signal after this frame is finished
		m_iCurrentFenceValue++;
		m_iFenceValues[iLastTaskStack] = m_iCurrentFenceValue;
		if (d3dCommandQueue->Signal(m_d3dFences[iLastTaskStack], m_iFenceValues[iLastTaskStack]) < 0) return false;

		return true;
	}


	void GPUScheduler::Release()
	{
		m_rtDevice = nullptr;

		if (m_d3dCommandList) m_d3dCommandList->Release();
		m_d3dCommandList = nullptr;

		for (unsigned int i = 0; i < m_iNumMaxTaskRecorders; i++)
		{
			if (m_d3dCommandAllocators)
			{
				if (m_d3dCommandAllocators[i]) m_d3dCommandAllocators[i]->Release();
				m_d3dCommandAllocators[i] = nullptr;
			}
			if (m_d3dFences)
			{
				if (m_d3dFences[i]) m_d3dFences[i]->Release();
				m_d3dFences[i] = nullptr;
			}
			if (m_hFenceCompletedEvents)
			{
				if (m_hFenceCompletedEvents[i]) CloseHandle(m_hFenceCompletedEvents[i]);
				m_hFenceCompletedEvents[i] = nullptr;
			}
		}
		if (m_d3dCommandAllocators) delete m_d3dCommandAllocators;
		m_d3dCommandAllocators = nullptr;
		if (m_d3dFences) delete m_d3dFences;
		m_d3dFences = nullptr;
		if (m_hFenceCompletedEvents) delete m_hFenceCompletedEvents;
		m_hFenceCompletedEvents = nullptr;
	}
}