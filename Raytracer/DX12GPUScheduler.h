#pragma once

//include our dx12 device wrapper
#include "DX12Device.h"



namespace RT::GraphicsAPI
{

	void WaitForFence(ID3D12Fence1* d3dFenceToWaitFor, UINT64 iCompletionValue, DWORD iMaxWaitingTime = 0xffffffff);
	void WaitForFence(ID3D12Fence1* d3dFenceToWaitFor, HANDLE hEventOnFinish, UINT64 iCompletionValue, DWORD iMaxWaitingTime = 0xffffffff);


	
	class GPUScheduler
	{
	private:

		//static member variables
		DX12Device* m_rtDevice;
		ID3D12GraphicsCommandList6*	m_d3dCommandList;
		ID3D12CommandAllocator**		m_d3dCommandAllocators;
		ID3D12Fence1**	m_d3dFences;
		HANDLE*			m_hFenceCompletedEvents;
		UINT64*			m_iFenceValues;
		UINT64			m_iCurrentFenceValue;
		unsigned int		m_iCurrentTaskStack;
		unsigned int		m_iNumMaxTaskRecorders;



	public: // = usable outside of the class

		//constructor and destructor
		GPUScheduler();
		~GPUScheduler();


		//public class functions
		bool Initialize(DX12Device& rtDevice, unsigned int iNumMaxTaskRecorders = 1);
		void Flush();
		bool Record(); //begin the recording
		bool Execute(); //end the recording and send the commands to the GPU
		void Release();


		//helper functions
		DX12Device* GetDX12Device() { return m_rtDevice; };
		ID3D12GraphicsCommandList6* GetCommandList() { return m_d3dCommandList; };

	};
}