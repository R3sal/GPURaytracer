#pragma once

//include the C++ file stream for loading teh shader files
#include <fstream>
#include <iostream>

//include the dx12 devcie class
#include "DX12Device.h"



namespace RT::GraphicsAPI
{

	class Raytracer
	{
	private:

		//static variables
		static const unsigned int s_iNumQueueSlots = 3;


		//private member variables
		DX12Device* m_rtDevice;
		ID3D12GraphicsCommandList6*	m_d3dCommandList;
		ID3D12CommandAllocator*		m_d3dCommandAllocators[s_iNumQueueSlots];
		ID3D12CommandAllocator*		m_d3dComputeCommandAllocator;
		ID3D12Fence1*				m_d3dFrameFences[s_iNumQueueSlots];
		ID3D12Fence1*				m_d3dComputeFence;
		UINT64						m_iFenceValues[s_iNumQueueSlots];
		UINT64						m_iComputeFenceValue;
		UINT64						m_iCurrentFenceValue;
		ID3D12DescriptorHeap*		m_d3dRTVsDescriptorHeap;
		ID3D12DescriptorHeap*		m_d3dUAVDescriptorHeap;
		D3D12_VIEWPORT			m_d3dViewport;
		D3D12_RECT				m_d3dScissorRect;
		ID3D12PipelineState*		m_d3dPipelineState;
		ID3D12PipelineState*		m_d3dComputePipelineState;
		ID3D12RootSignature*			m_d3dRootSignature;
		ID3D12RootSignature*			m_d3dComputeRootSignature;
		ID3D12Resource2*				m_d3dBackBuffers[s_iNumQueueSlots];
		ID3D12Resource2*				m_d3dIntersectionTarget;
		ID3D12Resource2*				m_d3dVertexBuffer;
		D3D12_VERTEX_BUFFER_VIEW		m_d3dVertexBufferView;
		unsigned int					m_iCurrentQueueSlot;


		//private functions
		//wait for a fence to finish
		void WaitForFence(ID3D12Fence1* d3dFenceToWaitFor, UINT64 iCompletionValue, DWORD iMaxWaitingTime = 0xffffffff);


	public: // = usable outside of the class

		//constructor and destructor
		Raytracer();
		~Raytracer();


		//public class functions
		bool Initialize(DX12Device& rtDevice);

		//render a frame
		bool Render();


		//helper functions

	};
}