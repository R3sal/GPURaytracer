//include-files
#include "DX12PipelineState.h"



namespace RT::GraphicsAPI
{

	//GPUScheduler constructor and destructor
	//constructor: initializes all the variables (at least with "0", "nullptr" or "")
	PipelineState::PipelineState() :
		//initialize the class variables
		m_rtScheduler(nullptr),
		m_d3dViewport(),
		m_d3dScissorRect(),
		m_d3dRootSignature(nullptr),
		m_d3dPipelineStateObject(nullptr)

	{

	}

	//destructor: uninitializes all our pointers
	PipelineState::~PipelineState()
	{
		Release();
	}



	//private class functions



	//public class functions
	bool PipelineState::Initialize(GPUScheduler& rtScheduler, ID3D12RootSignature* d3dRootSignature)
	{
		//set the member variables to their appropriate values


		return true;
	}


	void PipelineState::Release()
	{
		
	}
}