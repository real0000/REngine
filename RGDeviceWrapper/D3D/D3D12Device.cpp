// D3D12Device.cpp
//
// 2017/05/18 Ian Wu/Real0000
//

#include "RGDeviceWrapper.h"

namespace R
{

#pragma region D3D12HeapManager
//
// HeapManager
//
D3D12HeapManager::D3D12HeapManager(ID3D12Device *a_pDevice, unsigned int a_ExtendSize, D3D12_DESCRIPTOR_HEAP_TYPE a_Type, D3D12_DESCRIPTOR_HEAP_FLAGS a_Flag)
	: m_CurrHeapSize(0), m_ExtendSize(a_ExtendSize)
	, m_pHeap(nullptr), m_pHeapCache(nullptr)
	, m_pRefDevice(a_pDevice)
	, m_Type(a_Type)
	, m_Flag(a_Flag)
	, m_HeapUnitSize(m_pRefDevice->GetDescriptorHandleIncrementSize(m_Type))
{
	extend();
}

D3D12HeapManager::~D3D12HeapManager()
{
	SAFE_RELEASE(m_pHeap)
	SAFE_RELEASE(m_pHeapCache)
}

unsigned int D3D12HeapManager::newHeap(ID3D12Resource *a_pResource, const D3D12_RENDER_TARGET_VIEW_DESC *a_pDesc)
{
	unsigned int l_Res = newHeap();
	m_pRefDevice->CreateRenderTargetView(a_pResource, a_pDesc, getCpuHandle(l_Res));
	if( 0 != (m_Flag & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE) ) m_pRefDevice->CreateRenderTargetView(a_pResource, a_pDesc, getCpuHandle(l_Res, m_pHeapCache));
	return l_Res;
}

unsigned int D3D12HeapManager::newHeap(ID3D12Resource *a_pResource, const D3D12_SHADER_RESOURCE_VIEW_DESC *a_pDesc)
{
	unsigned int l_Res = newHeap();
	m_pRefDevice->CreateShaderResourceView(a_pResource, a_pDesc, getCpuHandle(l_Res));
	if( 0 != (m_Flag & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE) ) m_pRefDevice->CreateShaderResourceView(a_pResource, a_pDesc, getCpuHandle(l_Res, m_pHeapCache));
	return l_Res;
}

unsigned int D3D12HeapManager::newHeap(ID3D12Resource *a_pResource, const D3D12_DEPTH_STENCIL_VIEW_DESC *a_pDesc)
{
	unsigned int l_Res = newHeap();
	m_pRefDevice->CreateDepthStencilView(a_pResource, a_pDesc, getCpuHandle(l_Res));
	if( 0 != (m_Flag & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE) ) m_pRefDevice->CreateDepthStencilView(a_pResource, a_pDesc, getCpuHandle(l_Res, m_pHeapCache));
	return l_Res;
}

unsigned int D3D12HeapManager::newHeap(D3D12_CONSTANT_BUFFER_VIEW_DESC *a_pDesc)
{
	unsigned int l_Res = newHeap();
	m_pRefDevice->CreateConstantBufferView(a_pDesc, getCpuHandle(l_Res));
	if( 0 != (m_Flag & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE) ) m_pRefDevice->CreateConstantBufferView(a_pDesc, getCpuHandle(l_Res, m_pHeapCache));
	return l_Res;
}

unsigned int D3D12HeapManager::newHeap(ID3D12Resource *a_pResource, ID3D12Resource *a_pCounterResource, const D3D12_UNORDERED_ACCESS_VIEW_DESC *a_pDesc)
{
	unsigned int l_Res = newHeap();
	m_pRefDevice->CreateUnorderedAccessView(a_pResource, a_pCounterResource, a_pDesc, getCpuHandle(l_Res));
	if( 0 != (m_Flag & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE) ) m_pRefDevice->CreateUnorderedAccessView(a_pResource, a_pCounterResource, a_pDesc, getCpuHandle(l_Res, m_pHeapCache));
	return l_Res;
}

void D3D12HeapManager::recycle(unsigned int a_HeapID)
{
	assert(m_FreeSlot.end() == std::find(m_FreeSlot.begin(), m_FreeSlot.end(), a_HeapID));
	m_FreeSlot.push_back(a_HeapID);
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12HeapManager::getCpuHandle(unsigned int a_HeapID, ID3D12DescriptorHeap *a_pTargetHeap)
{
	D3D12_CPU_DESCRIPTOR_HANDLE l_Res((nullptr == a_pTargetHeap ? m_pHeap : a_pTargetHeap)->GetCPUDescriptorHandleForHeapStart());
	l_Res.ptr += a_HeapID * m_HeapUnitSize;
	return l_Res;
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12HeapManager::getGpuHandle(unsigned int a_HeapID, ID3D12DescriptorHeap *a_pTargetHeap)
{
	D3D12_GPU_DESCRIPTOR_HANDLE l_Res((nullptr == a_pTargetHeap ? m_pHeap : a_pTargetHeap)->GetGPUDescriptorHandleForHeapStart());
	l_Res.ptr += a_HeapID * m_HeapUnitSize;
	return l_Res;
}

unsigned int D3D12HeapManager::newHeap()
{
	if( m_FreeSlot.empty() ) extend();
	unsigned int l_Res = m_FreeSlot.front();
	m_FreeSlot.pop_front();
	return l_Res;
}

void D3D12HeapManager::extend()
{
	ID3D12DescriptorHeap *l_pNewHeap = nullptr;

	D3D12_DESCRIPTOR_HEAP_DESC l_HeapDesc;
	l_HeapDesc.Flags = m_Flag;
	l_HeapDesc.NodeMask = 0;// no multi gpu
	l_HeapDesc.NumDescriptors = m_CurrHeapSize + m_ExtendSize;
	l_HeapDesc.Type = m_Type;

	HRESULT l_Res = m_pRefDevice->CreateDescriptorHeap(&l_HeapDesc, IID_PPV_ARGS(&l_pNewHeap));
	assert(S_OK == l_Res);
	
	if( 0 == (m_Flag & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE) )
	{
		if( nullptr != m_pHeap )
		{
			m_pRefDevice->CopyDescriptorsSimple(m_CurrHeapSize, l_pNewHeap->GetCPUDescriptorHandleForHeapStart(), m_pHeap->GetCPUDescriptorHandleForHeapStart(), m_Type);
			m_pHeap->Release();
		}
		m_pHeap = l_pNewHeap;
	}
	else
	{
		ID3D12DescriptorHeap *l_pNewHeapCache = nullptr;
		l_HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		l_Res = m_pRefDevice->CreateDescriptorHeap(&l_HeapDesc, IID_PPV_ARGS(&l_pNewHeapCache));
		assert(S_OK == l_Res);

		if( nullptr != m_pHeap )
		{
			m_pRefDevice->CopyDescriptorsSimple(m_CurrHeapSize, l_pNewHeapCache->GetCPUDescriptorHandleForHeapStart(), m_pHeapCache->GetCPUDescriptorHandleForHeapStart(), m_Type);
			m_pRefDevice->CopyDescriptorsSimple(m_CurrHeapSize, l_pNewHeap->GetCPUDescriptorHandleForHeapStart(), m_pHeapCache->GetCPUDescriptorHandleForHeapStart(), m_Type);
			m_pHeap->Release();
			m_pHeapCache->Release();
		}
		m_pHeap = l_pNewHeap;
		m_pHeapCache = l_pNewHeapCache;
	}
	
	for( unsigned int i=0 ; i<m_ExtendSize ; ++i ) m_FreeSlot.push_back(m_CurrHeapSize + i);
	m_CurrHeapSize += m_ExtendSize;
}
#pragma endregion

#pragma region D3D12Commander
//
// D3D12Commander
//
thread_local D3D12GpuThread g_CurrThread = std::make_pair(nullptr, nullptr);
thread_local HLSLProgram12 *g_pCurrProgram = nullptr;
D3D12Commander::D3D12Commander(D3D12HeapManager *a_pHeapOwner)
	: m_bFullScreen(false)
	, m_pSwapChain(nullptr)
	, m_pDrawCmdQueue(nullptr), m_pBundleCmdQueue(nullptr)
	, m_Handle(0)
	, m_FenceEvent(0), m_SyncVal(0), m_pSynchronizer(nullptr)
	, m_pRefDevice(TYPED_GDEVICE(D3D12Device))
	, m_pRefHeapOwner(a_pHeapOwner)
{
	memset(m_BackBuffer, 0, sizeof(unsigned int) * NUM_BACKBUFFER);
	memset(m_pBackbufferRes, 0, sizeof(ID3D12Resource *) * NUM_BACKBUFFER);
}

D3D12Commander::~D3D12Commander()
{
	SAFE_RELEASE(m_pDrawCmdQueue)
	SAFE_RELEASE(m_pBundleCmdQueue)
	for( unsigned int i=0 ; i<NUM_BACKBUFFER ; ++i )
	{
		m_pRefHeapOwner->recycle(m_BackBuffer[i]);
		SAFE_RELEASE(m_pBackbufferRes[i])
	}
	SAFE_RELEASE(m_pSynchronizer)
	SAFE_RELEASE(m_pSwapChain)
}

void D3D12Commander::init(WXWidget a_Handle, glm::ivec2 a_Size, bool a_bFullScr)
{
	m_Handle = a_Handle;
	ID3D12Device *l_pDevInst = m_pRefDevice->getDeviceInst();
	IDXGIFactory4 *l_pDevFactory = m_pRefDevice->getDeviceFactory();

	D3D12_COMMAND_QUEUE_DESC l_QueueDesc;
	l_QueueDesc.NodeMask = 0;
	l_QueueDesc.Priority = 0;
	l_QueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	l_QueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	HRESULT l_Res = l_pDevInst->CreateCommandQueue(&l_QueueDesc, IID_PPV_ARGS(&m_pDrawCmdQueue));
	assert(S_OK == l_Res);
	
	l_QueueDesc.Type = D3D12_COMMAND_LIST_TYPE_BUNDLE;
	l_Res = l_pDevInst->CreateCommandQueue(&l_QueueDesc, IID_PPV_ARGS(&m_pBundleCmdQueue));
	assert(S_OK == l_Res);

	DXGI_SWAP_CHAIN_DESC l_SwapChainDesc;
	l_SwapChainDesc.BufferCount = NUM_BACKBUFFER;
	l_SwapChainDesc.BufferDesc.Width = a_Size.x;
	l_SwapChainDesc.BufferDesc.Height = a_Size.y;
	l_SwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	l_SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	l_SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	l_SwapChainDesc.OutputWindow = (HWND)a_Handle;
	l_SwapChainDesc.SampleDesc.Count = 1;
	l_SwapChainDesc.Windowed = (m_bFullScreen = a_bFullScr);

	if( S_OK != l_pDevFactory->CreateSwapChain(m_pDrawCmdQueue, &l_SwapChainDesc, &m_pSwapChain) )
	{
		wxMessageBox(wxT("swap chain create failed"), wxT("D3D12Commander::init"));
		return;
	}

	for( unsigned int i=0 ; i<NUM_BACKBUFFER ; ++i )
	{
		if( S_OK != m_pSwapChain->GetBuffer(i, IID_PPV_ARGS(&m_pBackbufferRes[i])) )
		{
			wxMessageBox(wxString::Format(wxT("can't get rtv[%d]"), i), wxT("D3D12Commander::init"));
			return;
		}

		m_BackBuffer[i] = m_pRefHeapOwner->newHeap(m_pBackbufferRes[i], (D3D12_RENDER_TARGET_VIEW_DESC *)nullptr);
	}

	if( S_OK != l_pDevInst->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pSynchronizer)) )
	{
		wxMessageBox("can't create synchronizer", wxT("D3D12Commander::init"));
		return;
	}
}

void D3D12Commander::resize(glm::ivec2 a_Size, bool a_bFullScr)
{
	m_pSwapChain = nullptr;
	for( unsigned int i=0 ; i<NUM_BACKBUFFER ; ++i )
	{
		m_pBackbufferRes[i] = nullptr;
		m_pRefHeapOwner->recycle(m_BackBuffer[i]);
	}

	init(m_Handle, a_Size, a_bFullScr);
}

void D3D12Commander::useProgram(ShaderProgram *a_pProgram)
{
	D3D12GpuThread l_Thread = getThisThread();
	g_pCurrProgram = dynamic_cast<HLSLProgram12 *>(a_pProgram);
	if( g_pCurrProgram->isCompute() ) l_Thread.second->SetComputeRootSignature(g_pCurrProgram->getRegDesc());
	else l_Thread.second->SetGraphicsRootSignature(g_pCurrProgram->getRegDesc());
	l_Thread.second->SetPipelineState(g_pCurrProgram->getPipeline());
}

void D3D12Commander::bindVertex(VertexBuffer *a_pBuffer)
{
	assert(nullptr != a_pBuffer);
	D3D12GpuThread l_Thread = getThisThread();

	D3D12_VERTEX_BUFFER_VIEW l_Buffers[VTXSLOT_COUNT] = {};
	for( unsigned int i=0 ; i<VTXSLOT_COUNT ; ++i )
	{
		int l_BuffID = a_pBuffer->getBufferID(i);
		if( -1 != l_BuffID ) l_Buffers[i] = m_pRefDevice->getVertexBufferView(l_BuffID);
	}
	l_Thread.second->IASetVertexBuffers(0, VTXSLOT_COUNT, l_Buffers);
}

void D3D12Commander::bindIndex(IndexBuffer *a_pBuffer)
{
	assert(nullptr != a_pBuffer);
	D3D12GpuThread l_Thread = getThisThread();
	l_Thread.second->IASetIndexBuffer(&(m_pRefDevice->getIndexBufferView(a_pBuffer->getBufferID())));
}

void D3D12Commander::bindTexture(int a_TextureID, unsigned int a_Stage)
{/*
	assert(nullptr != g_pCurrProgram);
	int l_RootSlot = g_pCurrProgram->getTextureRootSlot(a_Stage);
	if( -1 == l_RootSlot ) return;

	D3D12GpuThread l_Thread = getThisThread();

	l_Thread.second->SetComputeRootDescriptorTable(l_RootSlot, );*/
}

void D3D12Commander::bindVtxFlag(unsigned int a_VtxFlag)
{
}

void D3D12Commander::bindUniformBlock(int a_HeapID, int a_BlockStage)
{
}

void D3D12Commander::bindUavBlock(int a_HeapID, int a_BlockStage)
{
}

void D3D12Commander::clearRenderTarget(int a_RTVHandle, glm::vec4 a_Color)
{
}

void D3D12Commander::clearBackBuffer(int a_Idx, glm::vec4 a_Color)
{
}

void D3D12Commander::clearDepthTarget(int a_DSVHandle, bool a_bClearDepth, float a_Depth, bool a_bClearStencil, unsigned char a_Stencil)
{
}

void D3D12Commander::drawVertex(int a_NumVtx, int a_BaseVtx)
{
}

void D3D12Commander::drawElement(int a_BaseIdx, int a_NumIdx, int a_BaseVtx)
{
}

void D3D12Commander::drawIndirect(ShaderProgram *a_pProgram, unsigned int a_MaxCmd, void *a_pResPtr, void *a_pCounterPtr, unsigned int a_BufferOffset)
{
}
	
void D3D12Commander::setTopology(Topology::Key a_Key)
{
}

void D3D12Commander::setRenderTarget(int a_DSVHandle, std::vector<int> &a_RTVHandle)
{
}

void D3D12Commander::setRenderTargetWithBackBuffer(int a_DSVHandle, unsigned int a_BackIdx)
{
}

void D3D12Commander::setViewPort(int a_NumViewport, ...)
{
}

void D3D12Commander::setScissor(int a_NumScissor, ...)
{
}

D3D12GpuThread D3D12Commander::getThisThread()
{
	if( nullptr != g_CurrThread.first ) return g_CurrThread;

	g_CurrThread = m_pRefDevice->requestThread();
	g_CurrThread.first->Reset();
	g_CurrThread.second->Reset(g_CurrThread.first, nullptr);
	ID3D12DescriptorHeap *l_ppHeaps[] = {m_pRefDevice->getShaderBindingHeap()};
	g_CurrThread.second->SetDescriptorHeaps(1, l_ppHeaps);
	ThreadEventCallback::getThreadLocal().addEndEvent(std::bind(&D3D12Commander::threadEnd, this));

	return g_CurrThread;
}

void D3D12Commander::flush()
{
	WaitForSingleObject(m_FenceEvent, INFINITE);
	m_pRefDevice->waitForResourceUpdate();
	for( unsigned int i=0 ; i<m_BusyThread.size() ; ++i ) m_pRefDevice->recycleThread(m_BusyThread[i]);

	m_BusyThread.resize(m_ReadyThread.size());
	std::copy(m_ReadyThread.begin(), m_ReadyThread.end(), m_BusyThread.begin());
	m_ReadyThread.clear();

	std::vector<ID3D12CommandList *> l_CmdArray(m_BusyThread.size());
	for( unsigned int i=0 ; i<m_BusyThread.size() ; ++i ) l_CmdArray[i] = m_BusyThread[i].second;

	m_pDrawCmdQueue->ExecuteCommandLists(l_CmdArray.size(), l_CmdArray.data());

	m_pDrawCmdQueue->Signal(m_pSynchronizer, m_SyncVal);
	m_pSynchronizer->SetEventOnCompletion(m_SyncVal, m_FenceEvent);
	++m_SyncVal;
}

void D3D12Commander::present()
{
	m_pRefDevice->waitForResourceUpdate();
	uint64 l_SyncVal = m_SyncVal;
	if( S_OK != m_pDrawCmdQueue->Signal(m_pSynchronizer, l_SyncVal) )
	{
		wxMessageBox(wxT("command signal error"), wxT("error"));
		return;
	}
	++m_SyncVal;

	// Wait until the previous frame is finished.
	if( m_pSynchronizer->GetCompletedValue() < l_SyncVal )
	{
		if( S_OK != m_pSynchronizer->SetEventOnCompletion(l_SyncVal, m_FenceEvent) )
		{
			wxMessageBox(wxT("frame sync error"), wxT("error"));
			return;
		}
		WaitForSingleObject(m_FenceEvent, INFINITE);
	}
}

void D3D12Commander::threadEnd()
{
	g_CurrThread.second->Close();
	m_ReadyThread.push_back(g_CurrThread);
	g_CurrThread.first = nullptr;
	g_CurrThread.second = nullptr;
	g_pCurrProgram = nullptr;
}
#pragma endregion

#pragma region D3D12Device
//
// D3D12Device
//
D3D12Device::D3D12Device()
	: m_pShaderResourceHeap(nullptr), m_pSamplerHeap(nullptr), m_pRenderTargetHeap(nullptr), m_pDepthHeap(nullptr)
	, m_pGraphicInterface(nullptr)
	, m_pDevice(nullptr)
	, m_MsaaSetting({1, 0})
	, m_pResCmdQueue(nullptr), m_pComputeQueue(nullptr)
	, m_IdleResThread(0)
	, m_FenceEvent(0), m_SyncVal(0), m_pSynchronizer(nullptr)
	, m_ManagedTexture(), m_ManagedVertexBuffer(), m_ManagedIndexBuffer()
{
	BIND_DEFAULT_ALLOCATOR(TextureBinder, m_ManagedTexture);
	BIND_DEFAULT_ALLOCATOR(VertexBinder, m_ManagedVertexBuffer);
	BIND_DEFAULT_ALLOCATOR(IndexBinder, m_ManagedIndexBuffer);

	for( unsigned int i=0 ; i<D3D12_NUM_COPY_THREAD ; ++i ) m_ResThread[i] = std::make_pair(nullptr, nullptr);
}

D3D12Device::~D3D12Device()
{
	waitForResourceUpdate();
	SAFE_RELEASE(m_pSynchronizer)

	SAFE_DELETE(m_pShaderResourceHeap);
	SAFE_DELETE(m_pSamplerHeap);
	SAFE_DELETE(m_pRenderTargetHeap);
	SAFE_DELETE(m_pDepthHeap);
	
	m_ManagedTexture.clear();
	m_ManagedVertexBuffer.clear();
	m_ManagedIndexBuffer.clear();

	SAFE_RELEASE(m_pGraphicInterface)
	SAFE_RELEASE(m_pResCmdQueue)
	SAFE_RELEASE(m_pComputeQueue)
	
	for( unsigned int i=0 ; i<D3D12_NUM_COPY_THREAD ; ++i )
	{
		SAFE_RELEASE(m_ResThread[i].first)
		SAFE_RELEASE(m_ResThread[i].second)
	}

	// all thread should be idle
	while( !m_GraphicThread.empty() )
	{
		D3D12GpuThread l_Thread = m_GraphicThread.front();
		SAFE_RELEASE(l_Thread.first)
		SAFE_RELEASE(l_Thread.second)
		m_GraphicThread.pop_front();
	}
	while( !m_ComputeThread.empty() )
	{
		D3D12GpuThread l_Thread = m_ComputeThread.front();
		SAFE_RELEASE(l_Thread.first)
		SAFE_RELEASE(l_Thread.second)
		m_ComputeThread.pop_front();
	}
	SAFE_RELEASE(m_pDevice)
}

void D3D12Device::initDeviceMap()
{
	HRESULT l_Res = S_OK;
	if( nullptr == m_pGraphicInterface )
	{
		l_Res = CreateDXGIFactory1(IID_PPV_ARGS(&m_pGraphicInterface));
		if( S_OK != l_Res )
		{
			wxMessageBox(wxT("init gi object failed"), wxT("D3D12Device::initDeviceMap"));
			return;
		}
	}

	unsigned int l_Idx = 0;
	while( S_OK == l_Res )
	{
		IDXGIAdapter1 *l_pAdapter = nullptr;
		l_Res = m_pGraphicInterface->EnumAdapters1(l_Idx, &l_pAdapter);
		++l_Idx;
		if( S_OK == l_Res )
		{
			DXGI_ADAPTER_DESC1 l_Desc;
			l_pAdapter->GetDesc1(&l_Desc);
			m_DeviceMap[l_Desc.DeviceId] = l_Desc.Description;
			l_pAdapter->Release();
		}
	}
}

void D3D12Device::initDevice(unsigned int a_DeviceID)
{
	HRESULT l_Res = S_OK;
	if( nullptr == m_pGraphicInterface )
	{
		l_Res = CreateDXGIFactory1(IID_PPV_ARGS(&m_pGraphicInterface));
		if( S_OK != l_Res )
		{
			wxMessageBox(wxT("init gi object failed"), wxT("D3D12Device::initDevice"));
			return;
		}
	}

	IDXGIAdapter1 *l_pTargetAdapter = nullptr;
	l_Res = S_OK;
	unsigned int l_Idx = 0;
	while( S_OK == l_Res )
	{
		l_Res = m_pGraphicInterface->EnumAdapters1(l_Idx, &l_pTargetAdapter);
		++l_Idx;
		if( S_OK == l_Res )
		{
			DXGI_ADAPTER_DESC1 l_Desc;
			l_pTargetAdapter->GetDesc1(&l_Desc);
			l_pTargetAdapter->Release();
			if( l_Desc.DeviceId == a_DeviceID ) break;
		}
	}
	assert(S_OK == l_Res);// means adapter not found

	l_Res = D3D12CreateDevice(l_pTargetAdapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_pDevice));
	
	if(S_OK != l_Res) wxMessageBox(wxT("device init failed"), wxT("D3D12Device::initDevice"));
}

void D3D12Device::init()
{
#ifdef _DEBUG
	// Enable the D3D12 debug layer.
	{
		ID3D12Debug *l_pDebugController;
		if( SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&l_pDebugController))))
		{
			l_pDebugController->EnableDebugLayer();
		}
	}
#endif

	ProgramManager::singleton().init(new HLSLComponent());
	if( m_DeviceMap.empty() ) initDeviceMap();
	if( nullptr == m_pDevice )
	{
		wxArrayString l_Devices;
		for( auto it = m_DeviceMap.begin() ; it != m_DeviceMap.end() ; ++it ) l_Devices.Add(it->second);
		wxSingleChoiceDialog l_Dlg(nullptr, wxT("please select target device"), wxT("D3D12Device::init"), l_Devices, (void **)nullptr, wxOK);
		l_Dlg.ShowModal();
		auto it = m_DeviceMap.begin();
		for( int i=0 ; i<l_Dlg.GetSelection() ; ++i ) ++it;
		initDevice(it->first);
	}

	// Describe and create the command queue.
	D3D12_COMMAND_QUEUE_DESC l_QueueDesc;
	l_QueueDesc.NodeMask = 0;
	l_QueueDesc.Priority = 0;
	l_QueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
	l_QueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	HRESULT l_Res = m_pDevice->CreateCommandQueue(&l_QueueDesc, IID_PPV_ARGS(&m_pResCmdQueue));
	assert(S_OK == l_Res);
	
	l_QueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
	l_Res = m_pDevice->CreateCommandQueue(&l_QueueDesc, IID_PPV_ARGS(&m_pComputeQueue));
	assert(S_OK == l_Res);
	
	for( unsigned int i=0 ; i<D3D12_NUM_COPY_THREAD ; ++i ) m_ResThread[i] = newThread(D3D12_COMMAND_LIST_TYPE_COPY);
	m_ResThread[m_IdleResThread].first->Reset();
	m_ResThread[m_IdleResThread].second->Reset(m_ResThread[m_IdleResThread].first, nullptr);
	ID3D12DescriptorHeap *l_ppHeaps[] = { m_pShaderResourceHeap->getHeapInst() };
	m_ResThread[m_IdleResThread].second->SetDescriptorHeaps(1, l_ppHeaps);

	unsigned int l_NumCore = std::thread::hardware_concurrency();
	if( 0 == l_NumCore ) l_NumCore = DEFAULT_D3D_THREAD_COUNT;
	l_NumCore *= 2;
	for( unsigned int i=0 ; i<l_NumCore ; ++i )
	{
		m_GraphicThread.push_back(newThread(D3D12_COMMAND_LIST_TYPE_DIRECT));
		m_ComputeThread.push_back(newThread(D3D12_COMMAND_LIST_TYPE_COMPUTE));
	}
	
	if( S_OK != m_pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pSynchronizer)) )
	{
		wxMessageBox("failed to create synchronizer", wxT("D3D12Device::init"));
		return;
	}

	// Create descriptor heaps.
	{
		m_pShaderResourceHeap = new D3D12HeapManager(m_pDevice, 256, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
		m_pSamplerHeap = new D3D12HeapManager(m_pDevice, 8, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
		m_pRenderTargetHeap = new D3D12HeapManager(m_pDevice, 16, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		m_pDepthHeap = new D3D12HeapManager(m_pDevice, 8, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	}
}

void D3D12Device::setupCanvas(wxWindow *a_pWnd, GraphicCanvas *a_pCanvas, glm::ivec2 a_Size, bool a_bFullScr)
{
	assert(nullptr != m_pGraphicInterface);
	assert(m_CanvasContainer.end() == m_CanvasContainer.find(a_pWnd));
	a_pCanvas->setCommander(new D3D12Commander(m_pRenderTargetHeap));
	m_CanvasContainer.insert(std::make_pair(a_pWnd, a_pCanvas));
	a_pCanvas->init(a_pWnd->GetHandle(), a_Size, a_bFullScr);

#ifdef WIN32
	DEVMODE l_DevMode;
	memset(&l_DevMode, 0, sizeof(DEVMODE));
	l_DevMode.dmSize = sizeof(DEVMODE);
	l_DevMode.dmPelsWidth = a_Size.x;
	l_DevMode.dmPelsHeight = a_Size.y;
	l_DevMode.dmBitsPerPel = 32;
	l_DevMode.dmDisplayFrequency = 60;
	l_DevMode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;

	bool l_bSuccess = ChangeDisplaySettings(&l_DevMode, CDS_FULLSCREEN) == DISP_CHANGE_SUCCESSFUL;
	assert( l_bSuccess && "invalid display settings");
#endif
}

void D3D12Device::resetCanvas(wxWindow *a_pCanvas, glm::ivec2 a_Size, bool a_bFullScr)
{
	assert(nullptr != m_pGraphicInterface);

	auto l_CanvasIt = m_CanvasContainer.find(a_pCanvas);
	assert(m_CanvasContainer.end() == l_CanvasIt);

	D3D12Commander *l_pCommander = (D3D12Commander *)l_CanvasIt->second->getCommander();
	bool l_bResetMode = a_bFullScr != l_pCommander->isFullScreen();
	l_CanvasIt->second->init(a_pCanvas->GetHandle(), a_Size, a_bFullScr);

	if( l_bResetMode )
	{
#ifdef WIN32
		DEVMODE l_DevMode;
		memset(&l_DevMode, 0, sizeof(DEVMODE));
		l_DevMode.dmSize = sizeof(DEVMODE);
		l_DevMode.dmPelsWidth = a_Size.x;
		l_DevMode.dmPelsHeight = a_Size.y;
		l_DevMode.dmBitsPerPel = 32;
		l_DevMode.dmDisplayFrequency = 60;
		l_DevMode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;

		bool l_bSuccess = ChangeDisplaySettings(&l_DevMode, CDS_FULLSCREEN) == DISP_CHANGE_SUCCESSFUL;
		assert( l_bSuccess && "invalid display settings");
#endif
	}
}

void D3D12Device::destroyCanvas(wxWindow *a_pCanvas)
{
	auto it = m_CanvasContainer.find(a_pCanvas);
	if( m_CanvasContainer.end() == it ) return;

	delete it->second;
	m_CanvasContainer.erase(it);
}

std::pair<int, int> D3D12Device::maxShaderModel()
{
	return std::make_pair(5, 1);//no version check for dx12
}

unsigned int D3D12Device::getBlendKey(BlendKey::Key a_Key)
{
	switch( a_Key )
	{
		case BlendKey::zero:			return D3D12_BLEND_ZERO;
		case BlendKey::one:				return D3D12_BLEND_ONE;
		case BlendKey::src_color:		return D3D12_BLEND_SRC_COLOR;
		case BlendKey::inv_src_color:	return D3D12_BLEND_INV_SRC_COLOR;
		case BlendKey::src_alpha:		return D3D12_BLEND_SRC_ALPHA;
		case BlendKey::inv_src_alpha:	return D3D12_BLEND_INV_SRC_ALPHA;
		case BlendKey::dst_alpha:		return D3D12_BLEND_DEST_ALPHA;
		case BlendKey::inv_dst_alpha:	return D3D12_BLEND_INV_DEST_ALPHA;
		case BlendKey::dst_color:		return D3D12_BLEND_DEST_COLOR;
		case BlendKey::inv_dst_color:	return D3D12_BLEND_INV_DEST_COLOR;
		case BlendKey::src_alpha_saturate:return D3D12_BLEND_SRC_ALPHA_SAT;
		case BlendKey::blend_factor:	return D3D12_BLEND_BLEND_FACTOR;
		case BlendKey::inv_blend_factor:return D3D12_BLEND_INV_BLEND_FACTOR;
		case BlendKey::src1_color:		return D3D12_BLEND_SRC1_COLOR;
		case BlendKey::inv_src1_color:	return D3D12_BLEND_INV_SRC1_COLOR;
		case BlendKey::src1_alpha:		return D3D12_BLEND_SRC1_ALPHA;
		case BlendKey::inv_src1_alpha:	return D3D12_BLEND_INV_SRC1_ALPHA;
		default:break;
	}
	assert(false && "invalid blend type");
	return D3D12_BLEND_ZERO;
}

unsigned int D3D12Device::getBlendOP(BlendOP::Key a_Key)
{
	switch( a_Key )
	{
		case BlendOP::add:		return D3D12_BLEND_OP_ADD;
		case BlendOP::sub:		return D3D12_BLEND_OP_SUBTRACT;
		case BlendOP::rev_sub:	return D3D12_BLEND_OP_REV_SUBTRACT;
		case BlendOP::min:		return D3D12_BLEND_OP_MIN;
		case BlendOP::max:		return D3D12_BLEND_OP_MAX;
		default:break;
	}
	assert(false && "invalid blend operation type");
	return D3D12_BLEND_OP_ADD;
}

unsigned int D3D12Device::getBlendLogic(BlendLogic::Key a_Key)
{
	switch( a_Key )
	{
		case BlendLogic::clear_:	return D3D12_LOGIC_OP_CLEAR;
		case BlendLogic::set_:		return D3D12_LOGIC_OP_SET;
		case BlendLogic::copy_:		return D3D12_LOGIC_OP_COPY;
		case BlendLogic::copy_inv_:	return D3D12_LOGIC_OP_COPY_INVERTED;
		case BlendLogic::none_:		return D3D12_LOGIC_OP_NOOP;
		case BlendLogic::inv_:		return D3D12_LOGIC_OP_INVERT;
		case BlendLogic::and_:		return D3D12_LOGIC_OP_AND;
		case BlendLogic::nand_:		return D3D12_LOGIC_OP_NAND;
		case BlendLogic::or_:		return D3D12_LOGIC_OP_OR;
		case BlendLogic::nor_:		return D3D12_LOGIC_OP_NOR;
		case BlendLogic::xor_:		return D3D12_LOGIC_OP_XOR;
		case BlendLogic::eq_:		return D3D12_LOGIC_OP_EQUIV;
		case BlendLogic::and_rev_:	return D3D12_LOGIC_OP_AND_REVERSE;
		case BlendLogic::and_inv_:	return D3D12_LOGIC_OP_AND_INVERTED;
		case BlendLogic::or_rev_:	return D3D12_LOGIC_OP_OR_REVERSE;
		case BlendLogic::or_inv_:	return D3D12_LOGIC_OP_OR_INVERTED;
		default:break;
	}
	assert(false && "invalid blend logic type");
	return D3D12_LOGIC_OP_CLEAR;
}

unsigned int D3D12Device::getCullMode(CullMode::Key a_Key)
{
	switch( a_Key )
	{
		case CullMode::none:	return D3D12_CULL_MODE_NONE;
		case CullMode::front:	return D3D12_CULL_MODE_FRONT;
		case CullMode::back:	return D3D12_CULL_MODE_BACK;
		default:break;
	}
	assert(false && "invalid cull mode");
	return D3D12_CULL_MODE_NONE;
}

unsigned int D3D12Device::getComapreFunc(CompareFunc::Key a_Key)
{
	switch( a_Key )
	{
		case CompareFunc::never:		return D3D12_COMPARISON_FUNC_NEVER;
		case CompareFunc::less:			return D3D12_COMPARISON_FUNC_LESS;
		case CompareFunc::equal:		return D3D12_COMPARISON_FUNC_EQUAL;
		case CompareFunc::less_equal:	return D3D12_COMPARISON_FUNC_LESS_EQUAL;
		case CompareFunc::greater:		return D3D12_COMPARISON_FUNC_GREATER;
		case CompareFunc::not_equal:	return D3D12_COMPARISON_FUNC_NOT_EQUAL;
		case CompareFunc::greater_equal:return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
		case CompareFunc::always:		return D3D12_COMPARISON_FUNC_ALWAYS;
		default:break;
	}
	assert(false && "invalid compare function");
	return D3D12_COMPARISON_FUNC_NEVER;
}

unsigned int D3D12Device::getStencilOP(StencilOP::Key a_Key)
{
	switch( a_Key )
	{
		case StencilOP::keep:		return D3D12_STENCIL_OP_KEEP;
		case StencilOP::zero:		return D3D12_STENCIL_OP_ZERO;
		case StencilOP::replace:	return D3D12_STENCIL_OP_REPLACE;
		case StencilOP::inc_saturate:return D3D12_STENCIL_OP_INCR_SAT;
		case StencilOP::dec_saturate:return D3D12_STENCIL_OP_DECR_SAT;
		case StencilOP::invert:		return D3D12_STENCIL_OP_INVERT;
		case StencilOP::inc:		return D3D12_STENCIL_OP_INCR;
		case StencilOP::dec:		return D3D12_STENCIL_OP_DECR;
		default:break;
	}
	assert(false && "invalid stencil operator function");
	return D3D12_STENCIL_OP_KEEP;
}

unsigned int D3D12Device::getTopologyType(TopologyType::Key a_Key)
{
	switch( a_Key )
	{
		case TopologyType::point:	return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
		case TopologyType::line:	return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		case TopologyType::triangle:return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		case TopologyType::patch:	return D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
		default:break;
	}
	assert(false && "invalid Topology type");
	return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
}

unsigned int D3D12Device::getTopology(Topology::Key a_Key)
{
	switch( a_Key )
	{
		case Topology::undefined:			return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
		case Topology::point_list:			return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
		case Topology::line_list:			return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
		case Topology::line_strip:			return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
		case Topology::triangle_list:		return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		case Topology::triangle_strip:		return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
		case Topology::line_list_adj:		return D3D_PRIMITIVE_TOPOLOGY_LINELIST_ADJ;
		case Topology::line_strip_adj:		return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ;
		case Topology::triangle_list_adj:	return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ;
		case Topology::triangle_strip_adj:	return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ;
		case Topology::point_patch_list_1:	return D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST;
		case Topology::point_patch_list_2:	return D3D_PRIMITIVE_TOPOLOGY_2_CONTROL_POINT_PATCHLIST;
		case Topology::point_patch_list_3:	return D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
		case Topology::point_patch_list_4:	return D3D_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST;
		case Topology::point_patch_list_5:	return D3D_PRIMITIVE_TOPOLOGY_5_CONTROL_POINT_PATCHLIST;
		case Topology::point_patch_list_6:	return D3D_PRIMITIVE_TOPOLOGY_6_CONTROL_POINT_PATCHLIST;
		case Topology::point_patch_list_7:	return D3D_PRIMITIVE_TOPOLOGY_7_CONTROL_POINT_PATCHLIST;
		case Topology::point_patch_list_8:	return D3D_PRIMITIVE_TOPOLOGY_8_CONTROL_POINT_PATCHLIST;
		case Topology::point_patch_list_9:	return D3D_PRIMITIVE_TOPOLOGY_9_CONTROL_POINT_PATCHLIST;
		case Topology::point_patch_list_10:	return D3D_PRIMITIVE_TOPOLOGY_10_CONTROL_POINT_PATCHLIST;
		case Topology::point_patch_list_11:	return D3D_PRIMITIVE_TOPOLOGY_11_CONTROL_POINT_PATCHLIST;
		case Topology::point_patch_list_12:	return D3D_PRIMITIVE_TOPOLOGY_12_CONTROL_POINT_PATCHLIST;
		case Topology::point_patch_list_13:	return D3D_PRIMITIVE_TOPOLOGY_13_CONTROL_POINT_PATCHLIST;
		case Topology::point_patch_list_14:	return D3D_PRIMITIVE_TOPOLOGY_14_CONTROL_POINT_PATCHLIST;
		case Topology::point_patch_list_15:	return D3D_PRIMITIVE_TOPOLOGY_15_CONTROL_POINT_PATCHLIST;
		case Topology::point_patch_list_16:	return D3D_PRIMITIVE_TOPOLOGY_16_CONTROL_POINT_PATCHLIST;
		case Topology::point_patch_list_17:	return D3D_PRIMITIVE_TOPOLOGY_17_CONTROL_POINT_PATCHLIST;
		case Topology::point_patch_list_18:	return D3D_PRIMITIVE_TOPOLOGY_18_CONTROL_POINT_PATCHLIST;
		case Topology::point_patch_list_19:	return D3D_PRIMITIVE_TOPOLOGY_19_CONTROL_POINT_PATCHLIST;
		case Topology::point_patch_list_20:	return D3D_PRIMITIVE_TOPOLOGY_20_CONTROL_POINT_PATCHLIST;
		case Topology::point_patch_list_21:	return D3D_PRIMITIVE_TOPOLOGY_21_CONTROL_POINT_PATCHLIST;;
		case Topology::point_patch_list_22:	return D3D_PRIMITIVE_TOPOLOGY_22_CONTROL_POINT_PATCHLIST;
		case Topology::point_patch_list_23:	return D3D_PRIMITIVE_TOPOLOGY_23_CONTROL_POINT_PATCHLIST;
		case Topology::point_patch_list_24:	return D3D_PRIMITIVE_TOPOLOGY_24_CONTROL_POINT_PATCHLIST;
		case Topology::point_patch_list_25:	return D3D_PRIMITIVE_TOPOLOGY_25_CONTROL_POINT_PATCHLIST;
		case Topology::point_patch_list_26:	return D3D_PRIMITIVE_TOPOLOGY_26_CONTROL_POINT_PATCHLIST;
		case Topology::point_patch_list_27:	return D3D_PRIMITIVE_TOPOLOGY_27_CONTROL_POINT_PATCHLIST;
		case Topology::point_patch_list_28:	return D3D_PRIMITIVE_TOPOLOGY_28_CONTROL_POINT_PATCHLIST;
		case Topology::point_patch_list_29:	return D3D_PRIMITIVE_TOPOLOGY_29_CONTROL_POINT_PATCHLIST;
		case Topology::point_patch_list_30:	return D3D_PRIMITIVE_TOPOLOGY_30_CONTROL_POINT_PATCHLIST;
		case Topology::point_patch_list_31:	return D3D_PRIMITIVE_TOPOLOGY_31_CONTROL_POINT_PATCHLIST;
		case Topology::point_patch_list_32:	return D3D_PRIMITIVE_TOPOLOGY_32_CONTROL_POINT_PATCHLIST;
		default:break;
	}
	assert(false && "invalid Topology type");
	return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
}

int D3D12Device::allocateTexture1D(unsigned int a_Size, PixelFormat::Key a_Format)
{
	assert(a_Size >= MIN_TEXTURE_SIZE);
	unsigned int l_MipmapLevel = std::log2(a_Size) + 1;
	return allocateTexture(glm::ivec3(a_Size, 1, 1), a_Format, D3D12_RESOURCE_DIMENSION_TEXTURE1D, l_MipmapLevel, 0);
}

int D3D12Device::allocateTexture2D(glm::ivec2 a_Size, PixelFormat::Key a_Format, unsigned int a_ArraySize)
{
	assert(a_Size.x >= MIN_TEXTURE_SIZE && a_Size.y >= MIN_TEXTURE_SIZE);
	unsigned int l_MipmapLevel = std::log2(std::max(a_Size.x, a_Size.y)) + 1;
	return allocateTexture(glm::ivec3(a_Size.x, a_Size.y, std::max<int>(a_ArraySize, 1)), a_Format, D3D12_RESOURCE_DIMENSION_TEXTURE2D, l_MipmapLevel, 0);
}

int D3D12Device::allocateTexture3D(glm::ivec3 a_Size, PixelFormat::Key a_Format)
{
	assert(a_Size.x >= MIN_TEXTURE_SIZE && a_Size.y >= MIN_TEXTURE_SIZE && a_Size.z >= MIN_TEXTURE_SIZE);
	unsigned int l_MipmapLevel = std::log2(std::max(std::max(a_Size.x, a_Size.y), a_Size.z)) + 1;
	return allocateTexture(a_Size, a_Format, D3D12_RESOURCE_DIMENSION_TEXTURE3D, l_MipmapLevel, 0);
}

void D3D12Device::updateTexture1D(int a_ID, unsigned int a_MipmapLevel, unsigned int a_Size, unsigned int a_Offset, void *a_pSrcData)
{
#ifdef _DEBUG
	TextureBinder *l_pTargetTexture = m_ManagedTexture[a_ID];
	assert(nullptr != l_pTargetTexture);
	assert(TEXTYPE_SIMPLE_1D == l_pTargetTexture->m_Type);
	assert(a_Size >= 1);
	assert(a_Offset < (l_pTargetTexture->m_Size.x >> a_MipmapLevel) && a_Offset + a_Size <= (l_pTargetTexture->m_Size.x >> a_MipmapLevel));
#endif
	updateTexture(a_ID, a_MipmapLevel, glm::ivec3(a_Size, 1, 1), glm::ivec3(a_Offset, 0, 0), a_pSrcData);
}

void D3D12Device::updateTexture2D(int a_ID, unsigned int a_MipmapLevel, glm::ivec2 a_Size, glm::ivec2 a_Offset, unsigned int a_Idx, void *a_pSrcData)
{
#ifdef _DEBUG
	TextureBinder *l_pTargetTexture = m_ManagedTexture[a_ID];
	assert(nullptr != l_pTargetTexture);
	assert(TEXTYPE_SIMPLE_2D == l_pTargetTexture->m_Type);
	assert(a_Size.x >= 1 && a_Size.y >= 1);
	assert(a_Offset.x < (l_pTargetTexture->m_Size.x >> a_MipmapLevel) && a_Offset.x + a_Size.x <= (l_pTargetTexture->m_Size.x >> a_MipmapLevel) &&
		   a_Offset.y < (l_pTargetTexture->m_Size.y >> a_MipmapLevel) && a_Offset.y + a_Size.y <= (l_pTargetTexture->m_Size.y >> a_MipmapLevel) &&
		   a_Idx < l_pTargetTexture->m_Size.z);
#endif
	updateTexture(a_ID, a_MipmapLevel, glm::ivec3(a_Size.x, a_Size.y, 1), glm::ivec3(a_Offset.x, a_Offset.y, a_Idx), a_pSrcData);
}

void D3D12Device::updateTexture3D(int a_ID, unsigned int a_MipmapLevel, glm::ivec3 a_Size, glm::ivec3 a_Offset, void *a_pSrcData)
{
#ifdef _DEBUG
	TextureBinder *l_pTargetTexture = m_ManagedTexture[a_ID];
	assert(nullptr != l_pTargetTexture);
	assert(TEXTYPE_SIMPLE_3D == l_pTargetTexture->m_Type);
	assert(a_Size.x >= 1 && a_Size.y >= 1 && a_Size.z >= 1);
	assert(a_Offset.x < (l_pTargetTexture->m_Size.x >> a_MipmapLevel) && a_Offset.x + a_Size.x <= (l_pTargetTexture->m_Size.x >> a_MipmapLevel) &&
		   a_Offset.y < (l_pTargetTexture->m_Size.y >> a_MipmapLevel) && a_Offset.y + a_Size.y <= (l_pTargetTexture->m_Size.y >> a_MipmapLevel) &&
		   a_Offset.z < (l_pTargetTexture->m_Size.z >> a_MipmapLevel) && a_Offset.z + a_Size.z <= (l_pTargetTexture->m_Size.z >> a_MipmapLevel));
#endif
	updateTexture(a_ID, a_MipmapLevel, a_Size, a_Offset, a_pSrcData);
}

/*void D3D12Device::generateMipmap(int a_ID)
{
	// need complete hlsl 12 compute part
}*/

int D3D12Device::getTextureHeapID(int a_ID)
{
	return m_ManagedTexture[a_ID]->m_HeapID;
}

PixelFormat::Key D3D12Device::getTextureFormat(int a_ID)
{
	return m_ManagedTexture[a_ID]->m_Format;
}

glm::ivec3 D3D12Device::getTextureSize(int a_ID)
{
	return m_ManagedTexture[a_ID]->m_Size;
}

void* D3D12Device::getTextureResource(int a_ID)
{
	return m_ManagedTexture[a_ID]->m_pTexture;
}

void D3D12Device::freeTexture(int a_ID)
{	
	TextureBinder *l_pTargetBinder = m_ManagedTexture[a_ID];
	m_pShaderResourceHeap->recycle(l_pTargetBinder->m_HeapID);
	m_ManagedTexture.release(a_ID);
}

D3D12_VERTEX_BUFFER_VIEW D3D12Device::getVertexBufferView(int a_ID)
{
	return m_ManagedVertexBuffer[a_ID]->m_VtxView;
}

D3D12_INDEX_BUFFER_VIEW D3D12Device::getIndexBufferView(int a_ID)
{
	return m_ManagedIndexBuffer[a_ID]->m_IndexView;
}

D3D12GpuThread D3D12Device::requestThread()
{
	if( m_GraphicThread.empty() ) return newThread(D3D12_COMMAND_LIST_TYPE_DIRECT);

	std::lock_guard<std::mutex> l_Guard(m_ThreadMutex);
	D3D12GpuThread l_Res = m_GraphicThread.front();
	m_GraphicThread.pop_front();
	return l_Res;
}

void D3D12Device::recycleThread(D3D12GpuThread a_Thread)
{
	m_GraphicThread.push_back(a_Thread);
}

void D3D12Device::waitForResourceUpdate()
{
	bool l_bNeedExec = false;
	for( unsigned int i=0 ; i<D3D12_NUM_COPY_THREAD ; ++i )
	{
		if( !m_TempResources[i].empty() )
		{
			l_bNeedExec = true;
			break;
		}
	}
	if( !l_bNeedExec ) return;

	WaitForSingleObject(m_FenceEvent, INFINITE);

	if( !m_TempResources[m_IdleResThread].empty() )
	{
		m_ResThread[m_IdleResThread].second->Close();
		ID3D12CommandList* l_CmdArray[] = {m_ResThread[m_IdleResThread].second};
		m_pResCmdQueue->ExecuteCommandLists(1, l_CmdArray);

		m_pResCmdQueue->Signal(m_pSynchronizer, m_SyncVal);
		m_pSynchronizer->SetEventOnCompletion(m_SyncVal, m_FenceEvent);
		++m_SyncVal;
		WaitForSingleObject(m_FenceEvent, INFINITE);
	}

	for( unsigned int i=0 ; i<D3D12_NUM_COPY_THREAD ; ++i )
	{
		for( unsigned int j=0 ; j<m_TempResources[i].size() ; ++j ) m_TempResources[i][j]->Release();
		m_TempResources[i].clear();
	}
	m_IdleResThread = 0;
	m_ResThread[m_IdleResThread].first->Reset();
	m_ResThread[m_IdleResThread].second->Reset(m_ResThread[m_IdleResThread].first, nullptr);
	ID3D12DescriptorHeap *l_ppHeaps[] = { m_pShaderResourceHeap->getHeapInst() };
	m_ResThread[m_IdleResThread].second->SetDescriptorHeaps(1, l_ppHeaps);
}

void D3D12Device::checkResourceQueueState()
{
	if( m_pSynchronizer->GetCompletedValue() <= m_SyncVal ) return;
	if( m_TempResources[m_IdleResThread].empty() ) return;
	
	unsigned int l_TargetList = 1 - m_IdleResThread;
	for( unsigned int i=0 ; i<m_TempResources[l_TargetList].size() ; ++i ) m_TempResources[l_TargetList][i]->Release();
	m_TempResources[l_TargetList].clear();

	m_ResThread[l_TargetList].first->Reset();
	m_ResThread[l_TargetList].second->Reset(m_ResThread[l_TargetList].first, nullptr);
	ID3D12DescriptorHeap *l_ppHeaps[] = { m_pShaderResourceHeap->getHeapInst() };
	m_ResThread[l_TargetList].second->SetDescriptorHeaps(1, l_ppHeaps);

	m_ResThread[m_IdleResThread].second->Close();
	ID3D12CommandList* l_CmdArray[] = {m_ResThread[m_IdleResThread].second};
	m_pResCmdQueue->ExecuteCommandLists(1, l_CmdArray);
		
	m_pResCmdQueue->Signal(m_pSynchronizer, m_SyncVal);
	m_pSynchronizer->SetEventOnCompletion(m_SyncVal, m_FenceEvent);
	++m_SyncVal;

	m_IdleResThread = l_TargetList;
}

int D3D12Device::allocateTexture(glm::ivec3 a_Size, PixelFormat::Key a_Format, D3D12_RESOURCE_DIMENSION a_Dim, unsigned int a_MipmapLevel, unsigned int a_Flag)
{
	TextureBinder *l_pNewBinder = nullptr;
	unsigned int l_TextureID = m_ManagedTexture.retain(&l_pNewBinder);

	l_pNewBinder->m_Size = a_Size;
	l_pNewBinder->m_Format = a_Format;
	switch( a_Dim )
	{
		case D3D12_RESOURCE_DIMENSION_TEXTURE1D: l_pNewBinder->m_Type = TEXTYPE_SIMPLE_1D; break;
		case D3D12_RESOURCE_DIMENSION_TEXTURE2D: l_pNewBinder->m_Type = TEXTYPE_SIMPLE_2D; break;
		case D3D12_RESOURCE_DIMENSION_TEXTURE3D: l_pNewBinder->m_Type = TEXTYPE_SIMPLE_3D; break;
		default:break;
	}
	l_pNewBinder->m_MipmapLevels = a_MipmapLevel;

	D3D12_HEAP_PROPERTIES l_HeapProp;
	l_HeapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
	l_HeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	l_HeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	l_HeapProp.CreationNodeMask = 0;
	l_HeapProp.VisibleNodeMask = 0;

	D3D12_RESOURCE_DESC l_TexResDesc;
	l_TexResDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	l_TexResDesc.Alignment = 0;
	l_TexResDesc.Width = a_Size.x;
	l_TexResDesc.Height = a_Size.y;
	l_TexResDesc.DepthOrArraySize = a_Size.z;
	l_TexResDesc.MipLevels = a_MipmapLevel;
	
	if( PixelFormat::d32_float == a_Format ) l_TexResDesc.Format = (DXGI_FORMAT)getPixelFormat(PixelFormat::r32_typeless);
	else l_TexResDesc.Format = (DXGI_FORMAT)getPixelFormat(a_Format);

	l_TexResDesc.SampleDesc = m_MsaaSetting;
	l_TexResDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	l_TexResDesc.Flags = (D3D12_RESOURCE_FLAGS)a_Flag;

	bool l_bRenderTarget = 0 != (a_Flag & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) ||
							0 != (a_Flag & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
	D3D12_CLEAR_VALUE l_ClearVal;
	D3D12_RESOURCE_STATES l_State = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	if( l_bRenderTarget )
	{
		assert(a_Dim == D3D12_RESOURCE_DIMENSION_TEXTURE2D);
		l_ClearVal.Format = (DXGI_FORMAT)getPixelFormat(a_Format);
		if( PixelFormat::d16_unorm == a_Format ||
			PixelFormat::d24_unorm_s8_uint == a_Format ||
			PixelFormat::d32_float == a_Format ||
			PixelFormat::d32_float_s8x24_uint == a_Format )
		{
			l_ClearVal.DepthStencil.Depth = 1.0f;
			l_ClearVal.DepthStencil.Stencil = 0;
			l_State = D3D12_RESOURCE_STATE_DEPTH_WRITE;
			l_pNewBinder->m_Type = TEXTYPE_DEPTH_STENCIL_VIEW;
		}
		else
		{
			l_ClearVal.Color[0] = l_ClearVal.Color[1] = l_ClearVal.Color[2] = l_ClearVal.Color[3] = 0.0f;
			l_State = D3D12_RESOURCE_STATE_RENDER_TARGET;
			l_pNewBinder->m_Type = TEXTYPE_RENDER_TARGET_VIEW;
		}
	}

	HRESULT l_Res = m_pDevice->CreateCommittedResource(&l_HeapProp, D3D12_HEAP_FLAG_NONE, &l_TexResDesc, l_State, l_bRenderTarget ? &l_ClearVal : nullptr, IID_PPV_ARGS(&l_pNewBinder->m_pTexture));
	if( S_OK != l_Res )
	{
		wxMessageBox(wxT("Failed to create texture buffer"), wxT("D3D12DeviceInstance::allocateTexture"));
		return 0;
	}
	
	if( 0 == (a_Flag & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) )
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC l_SrvDesc = {};
		l_SrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		l_SrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		// switch format if is depth texture
		switch( a_Format )
		{
			case PixelFormat::d32_float_s8x24_uint:	l_SrvDesc.Format = (DXGI_FORMAT)getPixelFormat(PixelFormat::r32_float_x8x24_typeless);	break;
			case PixelFormat::d32_float:		l_SrvDesc.Format = (DXGI_FORMAT)getPixelFormat(PixelFormat::r32_float);				break;
			case PixelFormat::d24_unorm_s8_uint:l_SrvDesc.Format = (DXGI_FORMAT)getPixelFormat(PixelFormat::r24_unorm_x8_typeless);	break;
			case PixelFormat::d16_unorm:		l_SrvDesc.Format = (DXGI_FORMAT)getPixelFormat(PixelFormat::r16_unorm);				break;
			default:							l_SrvDesc.Format = l_TexResDesc.Format;												break;
		}

		l_SrvDesc.Texture2D.MipLevels = a_MipmapLevel;
		l_SrvDesc.Texture2D.MostDetailedMip = 0;
		l_SrvDesc.Texture2D.PlaneSlice = 0;
		l_SrvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		l_pNewBinder->m_HeapID = m_pShaderResourceHeap->newHeap(l_pNewBinder->m_pTexture, &l_SrvDesc);
	}

	return l_TextureID;
}

void D3D12Device::updateTexture(int a_ID, unsigned int a_MipmapLevel, glm::ivec3 a_Size, glm::ivec3 a_Offset, void *a_pSrcData)
{
	checkResourceQueueState();
	
	TextureBinder *l_pTargetTexture = m_ManagedTexture[a_ID];
	
	unsigned int l_PixelSize = getPixelSize(l_pTargetTexture->m_Format);
	std::vector<char> l_EmptyBuff;
	if( nullptr == a_pSrcData )
	{
		l_EmptyBuff.resize(l_PixelSize * a_Size.x * a_Size.y * a_Size.z, 0xff);
		a_pSrcData = (void*)&(l_EmptyBuff.front());
	}
	
	unsigned int l_SubResIdx = a_MipmapLevel;
	if( l_pTargetTexture->m_Type == TEXTYPE_SIMPLE_2D ) l_SubResIdx += (a_Offset.z * l_pTargetTexture->m_MipmapLevels);

	unsigned int l_RowPitch = 0;
	unsigned int l_SlicePitch = 0;
	glm::ivec3 l_MippedSize(std::max(1, l_pTargetTexture->m_Size.x >> a_MipmapLevel),
							std::max(1, l_pTargetTexture->m_Size.y >> a_MipmapLevel),
							std::max(1, l_pTargetTexture->m_Size.z >> a_MipmapLevel));
	{
		uint64 l_RequiredSize = 0;
		m_pDevice->GetCopyableFootprints(&l_pTargetTexture->m_pTexture->GetDesc(), l_SubResIdx, 1, 0, nullptr, nullptr, nullptr, &l_RequiredSize);
		l_RowPitch = l_RequiredSize / (l_MippedSize.z * l_MippedSize.y);
		l_RowPitch = (l_RowPitch + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1);
		l_SlicePitch = l_RowPitch * l_MippedSize.y;
	}
	
	ID3D12Resource *l_pUploader = nullptr;
	{
		D3D12_HEAP_PROPERTIES l_HeapProp;
		l_HeapProp.Type = D3D12_HEAP_TYPE_UPLOAD;
		l_HeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		l_HeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		l_HeapProp.CreationNodeMask = 0;
		l_HeapProp.VisibleNodeMask = 0;

		D3D12_RESOURCE_DESC l_TexResDesc;
		l_TexResDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		l_TexResDesc.Alignment = 0;
		l_TexResDesc.Width = l_pTargetTexture->m_Type == TEXTYPE_SIMPLE_3D ? l_SlicePitch * a_Size.z : l_RowPitch * a_Size.y;
		l_TexResDesc.Height = 1;
		l_TexResDesc.DepthOrArraySize = 1;
		l_TexResDesc.MipLevels = 1;
		l_TexResDesc.Format = DXGI_FORMAT_UNKNOWN;
		l_TexResDesc.SampleDesc = m_MsaaSetting;
		l_TexResDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		l_TexResDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		HRESULT l_Res = m_pDevice->CreateCommittedResource(&l_HeapProp, D3D12_HEAP_FLAG_NONE, &l_TexResDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&l_pUploader));
		if( S_OK != l_Res )
		{
			wxMessageBox(wxT("CreateCommittedResource failed"), wxT("D3D12Device::updateTexture"));
			return;
		}
		m_TempResources[m_IdleResThread].push_back(l_pUploader);

		unsigned char *l_pDataBegin = nullptr;
		l_Res = l_pUploader->Map(0, nullptr, reinterpret_cast<void**>(&l_pDataBegin));
		switch( l_pTargetTexture->m_Type )
		{
			case TEXTYPE_SIMPLE_1D:{
				memcpy(l_pDataBegin + a_Offset.x, a_pSrcData, a_Size.x * l_PixelSize);
				}break;

			case TEXTYPE_SIMPLE_2D:{
				#pragma	omp parallel for
				for( int y=0 ; y<a_Size.y ; ++y )
				{
					unsigned char *l_pDstStart = l_pDataBegin + ((a_Offset.x + y) * l_RowPitch + a_Offset.x * l_PixelSize);
					unsigned char *l_pSrcStart = (unsigned char *)a_pSrcData + (y * a_Size.x * l_PixelSize);
					memcpy(l_pDstStart, l_pSrcStart, a_Size.x * l_PixelSize);
				}
				}break;

			case TEXTYPE_SIMPLE_3D:{
				for( int z=0 ; z<a_Size.z ; ++z )
				{
					unsigned int l_DstSliceOffset = (a_Offset.z + z) * l_SlicePitch;
					unsigned int l_SrcSliceOffset = z * a_Size.x * a_Size.y * l_PixelSize;
					#pragma	omp parallel for
					for( int y=0 ; y<a_Size.y ; ++y )
					{
						unsigned char *l_pDstStart = l_pDataBegin + (l_DstSliceOffset + (a_Offset.x + y) * l_RowPitch + a_Offset.x * l_PixelSize);
						unsigned char *l_pSrcStart = (unsigned char *)a_pSrcData + l_SrcSliceOffset + (y * a_Size.x * l_PixelSize);
						memcpy(l_pDstStart, l_pSrcStart, a_Size.x * l_PixelSize);
					}
				}
				}break;

			default:break;
		}
		l_pUploader->Unmap(0, nullptr);
	}

	D3D12_RESOURCE_BARRIER l_TransSetting;
	l_TransSetting.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	l_TransSetting.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	l_TransSetting.Transition.pResource = l_pTargetTexture->m_pTexture;
	l_TransSetting.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	l_TransSetting.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	l_TransSetting.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
	m_ResThread[m_IdleResThread].second->ResourceBarrier(1, &l_TransSetting);

	// copy data
	D3D12_TEXTURE_COPY_LOCATION l_Src = {}, l_Dst = {};
	l_Dst.pResource = l_pTargetTexture->m_pTexture;
	l_Dst.SubresourceIndex = l_SubResIdx;
	l_Dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	l_Src.PlacedFootprint.Offset = 0;
	l_Src.PlacedFootprint.Footprint.Width = a_Size.x;
	l_Src.PlacedFootprint.Footprint.Height = a_Size.y;
	l_Src.PlacedFootprint.Footprint.Depth = a_Size.z;
	l_Src.PlacedFootprint.Footprint.RowPitch = l_RowPitch;
	l_Src.PlacedFootprint.Footprint.Format = (DXGI_FORMAT)getPixelFormat(l_pTargetTexture->m_Format);
	l_Src.pResource = l_pUploader;
	l_Src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	
	m_ResThread[m_IdleResThread].second->CopyTextureRegion(&l_Dst, a_Offset.x, a_Offset.y, a_Offset.z, &l_Src, nullptr);
	
	l_TransSetting.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	l_TransSetting.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	m_ResThread[m_IdleResThread].second->ResourceBarrier(1, &l_TransSetting);
}

D3D12GpuThread D3D12Device::newThread(D3D12_COMMAND_LIST_TYPE a_Type)
{
	D3D12GpuThread l_ThreadRes(nullptr, nullptr);
	HRESULT l_Res = m_pDevice->CreateCommandAllocator(a_Type, IID_PPV_ARGS(&l_ThreadRes.first));
	assert(S_OK == l_Res);
	l_Res = m_pDevice->CreateCommandList(0, a_Type, l_ThreadRes.first, nullptr, IID_PPV_ARGS(&l_ThreadRes.second));
	assert(S_OK == l_Res);
	return l_ThreadRes;
}

#pragma endregion
}