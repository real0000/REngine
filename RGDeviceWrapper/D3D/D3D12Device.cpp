// D3D12Device.cpp
//
// 2017/05/18 Ian Wu/Real0000
//

#include "RGDeviceWrapper.h"

namespace R
{

static void assignTransitionStruct(D3D12_RESOURCE_BARRIER &a_Dst, ID3D12Resource *a_pResource, int a_Before, int a_After, int a_SubResource = -1)
{
	a_Dst.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	a_Dst.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	a_Dst.Transition.pResource = a_pResource;
	a_Dst.Transition.Subresource = -1 == a_SubResource ? D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES : a_SubResource;
	a_Dst.Transition.StateBefore = (D3D12_RESOURCE_STATES)a_Before;
	a_Dst.Transition.StateAfter = (D3D12_RESOURCE_STATES)a_After;
}

//D3D12_RESOURCE_STATES
static void resourceTransition(ID3D12GraphicsCommandList *a_pCmdList, ID3D12Resource *a_pResource, int a_Before, int a_After, int a_SubResource = -1)
{
	D3D12_RESOURCE_BARRIER l_TransSetting = {};
	assignTransitionStruct(l_TransSetting, a_pResource, a_Before, a_After, a_SubResource);
	a_pCmdList->ResourceBarrier(1, &l_TransSetting);
}

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

unsigned int D3D12HeapManager::newHeap(D3D12_SAMPLER_DESC *a_pDesc)
{
	unsigned int l_Res = newHeap();
	m_pRefDevice->CreateSampler(a_pDesc, getCpuHandle(l_Res));
	if( 0 != (m_Flag & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE) ) m_pRefDevice->CreateSampler(a_pDesc, getCpuHandle(l_Res, m_pHeapCache));
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

#pragma region D3D12Fence
//
// D3D12Fence
//
D3D12Fence::D3D12Fence(ID3D12Device *a_pDevice, ID3D12CommandQueue *a_pQueue)
	: m_pRefQueue(a_pQueue), m_FenceEvent(0), m_SyncVal(0), m_pSynchronizer(nullptr)
{
	if( S_OK != a_pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pSynchronizer)) )
	{
		wxMessageBox("can't create synchronizer", wxT("D3D12Fence::D3D12Fence"));
		return;
	}
	m_FenceEvent = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
}

D3D12Fence::~D3D12Fence()
{
	SAFE_RELEASE(m_pSynchronizer)
	CloseHandle(m_FenceEvent);
}

void D3D12Fence::signal()
{
	m_pRefQueue->Signal(m_pSynchronizer, m_SyncVal);
	m_pSynchronizer->SetEventOnCompletion(m_SyncVal, m_FenceEvent);
	++m_SyncVal;
}

void D3D12Fence::wait()
{
	WaitForSingleObject(m_FenceEvent, INFINITE);
}
#pragma endregion

#pragma region D3D12Commander
//
// D3D12Commander
//
D3D12Commander::ComputeCmdComponent D3D12Commander::m_ComputeComponent;
D3D12Commander::GraphicCmdComponent D3D12Commander::m_GraphicComponent;
D3D12Commander::D3D12Commander(D3D12HeapManager *a_pHeapOwner)
	: m_pRefDevice(TYPED_GDEVICE(D3D12Device))
	, m_pRefHeapOwner(a_pHeapOwner)
	, m_bCompute(false)
	, m_pComponent(nullptr)
	, m_CurrThread(nullptr, nullptr)
	, m_pCurrProgram(nullptr)
	, m_pRefBackBuffer(nullptr)
	, m_bNeedResetBackBufferState(false)
{
}

D3D12Commander::~D3D12Commander()
{
}

void D3D12Commander::begin(bool a_bCompute)
{
	assert(nullptr == m_CurrThread.first && nullptr == m_CurrThread.second);
	m_ResourceState.clear();

	m_bCompute = a_bCompute;
	m_CurrThread = m_pRefDevice->requestThread();
	m_pComponent = a_bCompute ? reinterpret_cast<CmdComponent*>(&m_ComputeComponent) : reinterpret_cast<CmdComponent*>(&m_GraphicComponent);
	m_pCurrProgram = nullptr;

	ID3D12DescriptorHeap *l_ppHeaps[] = {m_pRefDevice->getShaderBindingHeap(), m_pRefDevice->getSamplerBindingHeap()};
	m_CurrThread.second->SetDescriptorHeaps(2, l_ppHeaps);
}

void D3D12Commander::end()
{
	if( m_bNeedResetBackBufferState )
	{
		resourceTransition(m_CurrThread.second, m_pRefBackBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		m_pRefBackBuffer = nullptr;
		m_bNeedResetBackBufferState = false;
	}

	std::vector<D3D12_RESOURCE_BARRIER> l_Barriers;
	for( auto it = m_ResourceState.begin() ; it != m_ResourceState.end() ; ++it )
	{
		if( it->second.first != it->second.second )
		{
			D3D12_RESOURCE_BARRIER l_Barrier = {};
			assignTransitionStruct(l_Barrier, it->first, it->second.second, it->second.first);
			l_Barriers.push_back(l_Barrier);
		}
	}
	if( !l_Barriers.empty() ) m_CurrThread.second->ResourceBarrier(l_Barriers.size(), l_Barriers.data());

	m_CurrThread.second->Close();
	m_pRefDevice->recycleThread(m_CurrThread);

	m_CurrThread = std::make_pair(nullptr, nullptr);
	m_pCurrProgram = nullptr;
	m_pComponent = nullptr;
	m_bCompute = false;
}

void D3D12Commander::useProgram(std::shared_ptr<ShaderProgram> a_pProgram)
{
	assert(m_bCompute == a_pProgram->isCompute());
	m_pCurrProgram = dynamic_cast<HLSLProgram12 *>(a_pProgram.get());
	
	m_pComponent->setRootSignature(m_CurrThread.second, m_pCurrProgram->getRegDesc());
	m_CurrThread.second->SetPipelineState(m_pCurrProgram->getPipeline());
}

void D3D12Commander::bindVertex(VertexBuffer *a_pBuffer, int a_InstanceBuffer)
{
	assert(nullptr != a_pBuffer);

	D3D12_VERTEX_BUFFER_VIEW l_Buffers[VTXSLOT_COUNT] = {};
	for( unsigned int i=0 ; i<VTXSLOT_COUNT ; ++i )
	{
		int l_BuffID = a_pBuffer->getBufferID(i);
		if( -1 != l_BuffID ) l_Buffers[i] = m_pRefDevice->getVertexBufferView(l_BuffID);
	}
	m_CurrThread.second->IASetVertexBuffers(0, VTXSLOT_COUNT, l_Buffers);

	if( -1 != a_InstanceBuffer )
	{
		D3D12_VERTEX_BUFFER_VIEW &l_View = m_pRefDevice->getVertexBufferView(a_InstanceBuffer);
		m_CurrThread.second->IASetVertexBuffers(VTXSLOT_INSTANCE, 1, &l_View);
	}
}

void D3D12Commander::bindIndex(IndexBuffer *a_pBuffer)
{
	assert(nullptr != a_pBuffer);
	m_CurrThread.second->IASetIndexBuffer(&(m_pRefDevice->getIndexBufferView(a_pBuffer->getBufferID())));
}

void D3D12Commander::bindTexture(int a_ID, unsigned int a_Stage, TextureBindType a_Type)
{
	int l_RootSlot = m_pCurrProgram->getTextureSlot(a_Stage);
	if( -1 == l_RootSlot ) return;

	m_pComponent->setRootDescriptorTable(m_CurrThread.second, l_RootSlot, m_pRefDevice->getTextureGpuHandle(a_ID, a_Type));
}

void D3D12Commander::bindSampler(int a_ID, unsigned int a_Stage)
{
	int l_RootSlot = m_pCurrProgram->getTextureSlot(a_Stage);
	if( -1 == l_RootSlot ) return;
	
	// behind texture
	m_pComponent->setRootDescriptorTable(m_CurrThread.second, l_RootSlot + 1, m_pRefDevice->getSamplerGpuHandle(a_ID));
}

void D3D12Commander::bindConstant(std::string a_Name, unsigned int a_SrcData)
{
	std::pair<int, int> l_ConstSlot = m_pCurrProgram->getConstantSlot(a_Name);
	if( -1 == l_ConstSlot.first ) return;

	m_pComponent->setRoot32BitConstant(m_CurrThread.second, l_ConstSlot.first, a_SrcData, l_ConstSlot.second >> 2);
}

void D3D12Commander::bindConstant(std::string a_Name, void* a_pSrcData, unsigned int a_SizeInUInt)
{
	std::pair<int, int> l_ConstSlot = m_pCurrProgram->getConstantSlot(a_Name);
	if( -1 == l_ConstSlot.first ) return;

	m_pComponent->setRoot32BitConstants(m_CurrThread.second, l_ConstSlot.first, a_SizeInUInt, a_pSrcData, l_ConstSlot.second >> 2);
}

void D3D12Commander::bindConstBlock(int a_ID, int a_BlockStage)
{
	int l_SlotInfo = m_pCurrProgram->getConstBufferSlot(a_BlockStage);
	if( -1 == l_SlotInfo ) return;

	m_pComponent->setRootConstantBufferView(m_CurrThread.second, l_SlotInfo, m_pRefDevice->getConstBufferGpuAddress(a_ID));
}

void D3D12Commander::bindUavBlock(int a_ID, int a_BlockStage)
{
	int l_SlotInfo = m_pCurrProgram->getUavSlot(a_BlockStage);
	if( -1 == l_SlotInfo ) return;

	m_pComponent->setRootUnorderedAccessView(m_CurrThread.second, l_SlotInfo, m_pRefDevice->getUnorderAccessBufferGpuAddress(a_ID));
}

void D3D12Commander::clearRenderTarget(int a_ID, glm::vec4 a_Color)
{
	m_CurrThread.second->ClearRenderTargetView(m_pRefDevice->getRenderTargetCpuHandle(a_ID), reinterpret_cast<const float *>(&a_Color), 0, nullptr);
}

void D3D12Commander::clearBackBuffer(GraphicCanvas *a_pCanvas, glm::vec4 a_Color)
{
	m_CurrThread.second->ClearRenderTargetView(m_pRefHeapOwner->getCpuHandle(a_pCanvas->getBackBuffer()), reinterpret_cast<const float *>(&a_Color), 0, nullptr);
}

void D3D12Commander::clearDepthTarget(int a_ID, bool a_bClearDepth, float a_Depth, bool a_bClearStencil, unsigned char a_Stencil)
{
	D3D12_CLEAR_FLAGS l_ClearFlag = (D3D12_CLEAR_FLAGS)((a_Depth ? D3D12_CLEAR_FLAG_DEPTH : 0) | (a_Stencil ? D3D12_CLEAR_FLAG_STENCIL : 0));
	m_CurrThread.second->ClearDepthStencilView(m_pRefDevice->getRenderTargetCpuHandle(a_ID, true), l_ClearFlag, a_Depth, a_Stencil, 0, nullptr);
}

void D3D12Commander::drawVertex(int a_NumVtx, int a_BaseVtx)
{
	m_CurrThread.second->DrawInstanced(a_NumVtx, 1, a_BaseVtx, 0);
}

void D3D12Commander::drawElement(int a_BaseIdx, int a_NumIdx, int a_BaseVtx, unsigned int a_NumInstance, unsigned int a_BaseInstance)
{
	m_CurrThread.second->DrawIndexedInstanced(a_NumIdx, a_NumInstance, a_BaseIdx, a_BaseVtx, a_BaseInstance);
}

void D3D12Commander::drawIndirect(unsigned int a_MaxCmd, void *a_pResPtr, void *a_pCounterPtr, unsigned int a_BufferOffset)
{
	ID3D12Resource *l_pArgBuffer = static_cast<ID3D12Resource *>(a_pResPtr);
	ID3D12Resource *l_pCounterBuffer = static_cast<ID3D12Resource *>(a_pCounterPtr);
	m_CurrThread.second->ExecuteIndirect(m_pCurrProgram->getCommandSignature(false), a_MaxCmd, l_pArgBuffer, a_BufferOffset, l_pCounterBuffer, 0);
}

void D3D12Commander::drawIndirect(unsigned int a_MaxCmd, int a_BuffID)
{
	ID3D12Resource *l_pArgBuffer = TYPED_GDEVICE(D3D12Device)->getIndirectCommandBuffer(a_BuffID);
	m_CurrThread.second->ExecuteIndirect(m_pCurrProgram->getCommandSignature(true), a_MaxCmd, l_pArgBuffer, 0, nullptr, 0);
}

void D3D12Commander::compute(unsigned int a_CountX, unsigned int a_CountY, unsigned int a_CountZ)
{
	m_CurrThread.second->Dispatch(a_CountX, a_CountY, a_CountZ);
}

void D3D12Commander::setTopology(Topology::Key a_Key)
{
	m_CurrThread.second->IASetPrimitiveTopology((D3D12_PRIMITIVE_TOPOLOGY)m_pRefDevice->getTopology(a_Key));
}

void D3D12Commander::setRenderTarget(int a_DepthID, std::vector<int> &a_RederTargetIDList)
{
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> l_RTVHandle(a_RederTargetIDList.size());
	
	std::vector<D3D12_RESOURCE_BARRIER> l_Barriers;
	for( unsigned int i=0 ; i<a_RederTargetIDList.size() ; ++i )
	{
		ID3D12Resource *l_pRTResource = m_pRefDevice->getRenderTargetResource(a_RederTargetIDList[i]);
		auto it = m_ResourceState.find(l_pRTResource);
		if( m_ResourceState.end() == it ) m_ResourceState[l_pRTResource] = std::make_pair(D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_RENDER_TARGET);
		else if( D3D12_RESOURCE_STATE_RENDER_TARGET != it->second.second )
		{
			D3D12_RESOURCE_BARRIER l_Barrier = {};
			assignTransitionStruct(l_Barrier, l_pRTResource, it->second.second, D3D12_RESOURCE_STATE_RENDER_TARGET);
			l_Barriers.push_back(l_Barrier);
			it->second.second = D3D12_RESOURCE_STATE_RENDER_TARGET;
		}
		l_RTVHandle[i] = m_pRefDevice->getRenderTargetCpuHandle(a_RederTargetIDList[i]);
	}

	if( -1 != a_DepthID )
	{
		ID3D12Resource *l_pDepthResource = m_pRefDevice->getRenderTargetResource(a_DepthID);
		auto it = m_ResourceState.find(l_pDepthResource);
		if( it == m_ResourceState.end() ) m_ResourceState[l_pDepthResource] = std::make_pair(D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		else if( D3D12_RESOURCE_STATE_DEPTH_WRITE != it->second.second )
		{
			D3D12_RESOURCE_BARRIER l_Barrier = {};
			assignTransitionStruct(l_Barrier, l_pDepthResource, it->second.second, D3D12_RESOURCE_STATE_DEPTH_WRITE);
			l_Barriers.push_back(l_Barrier);
		}
	}
	if( !l_Barriers.empty() ) m_CurrThread.second->ResourceBarrier(l_Barriers.size(), l_Barriers.data());
	m_CurrThread.second->OMSetRenderTargets(l_RTVHandle.size(), l_RTVHandle.data(), FALSE, -1 == a_DepthID ? nullptr : &(m_pRefDevice->getRenderTargetCpuHandle(a_DepthID, true)));
}

void D3D12Commander::setRenderTargetWithBackBuffer(int a_DepthID, GraphicCanvas *a_pCanvas)
{
	m_pRefBackBuffer = reinterpret_cast<ID3D12Resource *>(a_pCanvas->getBackBufferResource());
	m_bNeedResetBackBufferState = true;

	D3D12_CPU_DESCRIPTOR_HANDLE l_RtvHandle(m_pRefHeapOwner->getCpuHandle(a_pCanvas->getBackBuffer()));
	resourceTransition(m_CurrThread.second, m_pRefBackBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_CurrThread.second->OMSetRenderTargets(1, &l_RtvHandle, FALSE, -1 == a_DepthID ? nullptr : &(m_pRefDevice->getRenderTargetCpuHandle(a_DepthID)));
}

void D3D12Commander::setViewPort(int a_NumViewport, ...)
{
	std::vector<glm::viewport> l_Viewports(a_NumViewport);
	{
		va_list l_Arglist;
		va_start(l_Arglist, a_NumViewport);
		for( int i=0 ; i<a_NumViewport ; ++i ) l_Viewports[i] = va_arg(l_Arglist, glm::viewport);
		va_end(l_Arglist);
	}
	
	m_CurrThread.second->RSSetViewports(a_NumViewport, reinterpret_cast<const D3D12_VIEWPORT *>(&(l_Viewports.front())));
}

void D3D12Commander::setScissor(int a_NumScissor, ...)
{
	std::vector<glm::ivec4> l_Scissors(a_NumScissor);
	{
		va_list l_Arglist;
		va_start(l_Arglist, a_NumScissor);
		for( int i=0 ; i<a_NumScissor ; ++i ) 	l_Scissors[i] = va_arg(l_Arglist, glm::ivec4);
		va_end(l_Arglist);
	}
	m_CurrThread.second->RSSetScissorRects(a_NumScissor, reinterpret_cast<const D3D12_RECT *>(&(l_Scissors.front())));
}
#pragma endregion

#pragma region D3D12Canvas
//
// D3D12Canvas
//
D3D12Canvas::D3D12Canvas(D3D12HeapManager *a_pHeapOwner, wxWindow *a_pParent, wxWindowID a_ID)
	: GraphicCanvas(a_pParent, a_ID)
	, m_pRefHeapOwner(a_pHeapOwner)
{
	memset(m_BackBuffer, 0, sizeof(unsigned int) * NUM_BACKBUFFER);
	memset(m_pBackbufferRes, 0, sizeof(ID3D12Resource *) * NUM_BACKBUFFER);
}

D3D12Canvas::~D3D12Canvas()
{
	GDEVICE()->wait();

	for( unsigned int i=0 ; i<NUM_BACKBUFFER ; ++i )
	{
		m_pRefHeapOwner->recycle(m_BackBuffer[i]);
		SAFE_RELEASE(m_pBackbufferRes[i])
	}
	
	SAFE_RELEASE(m_pSwapChain)
}

void D3D12Canvas::init(bool a_bFullScr)
{
	D3D12Device* l_pDevInteface = TYPED_GDEVICE(D3D12Device);
	ID3D12Device *l_pDevInst = l_pDevInteface->getDeviceInst();
	IDXGIFactory4 *l_pDevFactory = l_pDevInteface->getDeviceFactory();

	DXGI_SWAP_CHAIN_DESC l_SwapChainDesc = {};
	l_SwapChainDesc.BufferCount = NUM_BACKBUFFER;
	l_SwapChainDesc.BufferDesc.Width = GetClientSize().x;
	l_SwapChainDesc.BufferDesc.Height = GetClientSize().y;
	l_SwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	l_SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	l_SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	l_SwapChainDesc.OutputWindow = (HWND)GetHandle();
	l_SwapChainDesc.SampleDesc.Count = 1;
	l_SwapChainDesc.SampleDesc.Quality = 0;
	
	l_SwapChainDesc.Windowed = !a_bFullScr;
	
	IDXGISwapChain *l_pSwapChain = nullptr;
	if( S_OK != l_pDevFactory->CreateSwapChain(l_pDevInteface->getDrawCommandQueue(), &l_SwapChainDesc, &l_pSwapChain) )
	{
		wxMessageBox(wxT("swap chain create failed"), wxT("D3D12Commander::init"));
		return;
	}
	m_pSwapChain = static_cast<IDXGISwapChain3 *>(l_pSwapChain);

	for( unsigned int i=0 ; i<NUM_BACKBUFFER ; ++i )
	{
		if( S_OK != m_pSwapChain->GetBuffer(i, IID_PPV_ARGS(&m_pBackbufferRes[i])) )
		{
			wxMessageBox(wxString::Format(wxT("can't get rtv[%d]"), i), wxT("D3D12Commander::init"));
			return;
		}

		m_BackBuffer[i] = m_pRefHeapOwner->newHeap(m_pBackbufferRes[i], static_cast<D3D12_RENDER_TARGET_VIEW_DESC *>(nullptr));
	}
	setInitialed();
}

unsigned int D3D12Canvas::getBackBuffer()
{
	return m_BackBuffer[m_pSwapChain->GetCurrentBackBufferIndex()];
}

void* D3D12Canvas::getBackBufferResource()
{
	return m_pBackbufferRes[m_pSwapChain->GetCurrentBackBufferIndex()];
}

void D3D12Canvas::present()
{
	// to do : add fps setting
	TYPED_GDEVICE(D3D12Device)->addPresentCanvas(this);
}

void D3D12Canvas::presentImp()
{
	m_pSwapChain->Present(1, 0);
}

void D3D12Canvas::resizeBackBuffer()
{
	if( !getNeedResize() ) return;
	for( unsigned int i=0 ; i<NUM_BACKBUFFER ; ++i )
	{
		m_pBackbufferRes[i]->Release();
		m_pBackbufferRes[i] = nullptr;
		m_pRefHeapOwner->recycle(m_BackBuffer[i]);
	}
	m_pSwapChain->ResizeBuffers(NUM_BACKBUFFER, GetClientSize().x, GetClientSize().y, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
	for( unsigned int i=0 ; i<NUM_BACKBUFFER ; ++i )
	{
		if( S_OK != m_pSwapChain->GetBuffer(i, IID_PPV_ARGS(&m_pBackbufferRes[i])) )
		{
			wxMessageBox(wxString::Format(wxT("can't get rtv[%d]"), i), wxT("D3D12Commander::init"));
			return;
		}

		m_BackBuffer[i] = m_pRefHeapOwner->newHeap(m_pBackbufferRes[i], static_cast<D3D12_RENDER_TARGET_VIEW_DESC *>(nullptr));
	}
}
#pragma endregion

#pragma region D3D12Device
//
// D3D12Device
//
D3D12Device::D3D12Device()
	: m_pShaderResourceHeap(nullptr), m_pSamplerHeap(nullptr), m_pRenderTargetHeap(nullptr), m_pDepthHeap(nullptr)
	, m_ManagedTexture(), m_ManagedSampler(), m_ManagedRenderTarget(), m_ManagedVertexBuffer(), m_ManagedIndexBuffer(), m_ManagedConstBuffer(), m_ManagedUavBuffer(), m_ManagedIndirectCommandBuffer()
	, m_pGraphicInterface(nullptr)
	, m_pDevice(nullptr)
	, m_MsaaSetting({1, 0})
	, m_pResCmdQueue(nullptr), m_pComputeQueue(nullptr), m_pDrawCmdQueue(nullptr)
	, m_IdleResThread(0)
	, m_pResFence(nullptr), m_pComputeFence(nullptr), m_pGraphicFence(nullptr)
	, m_bLooping(true)
{
	BIND_DEFAULT_ALLOCATOR(TextureBinder, m_ManagedTexture);
	BIND_DEFAULT_ALLOCATOR(SamplerBinder, m_ManagedSampler);
	BIND_DEFAULT_ALLOCATOR(RenderTargetBinder, m_ManagedRenderTarget);
	BIND_DEFAULT_ALLOCATOR(VertexBinder, m_ManagedVertexBuffer);
	BIND_DEFAULT_ALLOCATOR(IndexBinder, m_ManagedIndexBuffer);
	BIND_DEFAULT_ALLOCATOR(ConstBufferBinder, m_ManagedConstBuffer);
	BIND_DEFAULT_ALLOCATOR(UnorderAccessBufferBinder, m_ManagedUavBuffer);
	BIND_DEFAULT_ALLOCATOR(IndirectCommandBinder, m_ManagedIndirectCommandBuffer);

	for( unsigned int i=0 ; i<D3D12_NUM_COPY_THREAD ; ++i ) m_ResThread[i] = std::make_pair(nullptr, nullptr);
}

D3D12Device::~D3D12Device()
{
	wait();

	SAFE_DELETE(m_pResFence)
	SAFE_DELETE(m_pComputeFence)
	SAFE_DELETE(m_pGraphicFence)

	SAFE_DELETE(m_pShaderResourceHeap);
	SAFE_DELETE(m_pSamplerHeap);
	SAFE_DELETE(m_pRenderTargetHeap);
	SAFE_DELETE(m_pDepthHeap);
	
	m_ManagedTexture.clear();
	m_ManagedRenderTarget.clear();
	m_ManagedVertexBuffer.clear();
	m_ManagedIndexBuffer.clear();
	m_ManagedConstBuffer.clear();
	m_ManagedUavBuffer.clear();
	m_ManagedIndirectCommandBuffer.clear();

	SAFE_RELEASE(m_pGraphicInterface)
	SAFE_RELEASE(m_pResCmdQueue)
	SAFE_RELEASE(m_pComputeQueue)
	SAFE_RELEASE(m_pDrawCmdQueue)
	
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
	for( unsigned int i=0 ; S_OK == l_Res ; ++i )
	{
		l_Res = m_pGraphicInterface->EnumAdapters1(i, &l_pTargetAdapter);
		if( S_OK == l_Res )
		{
			DXGI_ADAPTER_DESC1 l_Desc;
			l_pTargetAdapter->GetDesc1(&l_Desc);
			if( l_Desc.DeviceId == a_DeviceID ) break;
			l_pTargetAdapter->Release();
		}
	}
	assert(S_OK == l_Res);// means adapter not found

	l_Res = D3D12CreateDevice(l_pTargetAdapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_pDevice));
	l_pTargetAdapter->Release();
	
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

	if( m_DeviceMap.empty() ) initDeviceMap();
	if( nullptr == m_pDevice )
	{
		wxArrayString l_Devices;
		for( auto it = m_DeviceMap.begin() ; it != m_DeviceMap.end() ; ++it ) l_Devices.Add(it->second);
		wxSingleChoiceDialog l_Dlg(nullptr, wxT("please select target device"), wxT("D3D12Device::init"), l_Devices, static_cast<void **>(nullptr), wxOK);
		l_Dlg.ShowModal();
		auto it = m_DeviceMap.begin();
		for( int i=0 ; i<l_Dlg.GetSelection() ; ++i ) ++it;
		initDevice(it->first);
	}

	// Describe and create the command queue.
	D3D12_COMMAND_QUEUE_DESC l_QueueDesc;
	l_QueueDesc.NodeMask = 0;
	l_QueueDesc.Priority = 0;
	l_QueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;//D3D12_COMMAND_LIST_TYPE_COPY;
	l_QueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	HRESULT l_Res = m_pDevice->CreateCommandQueue(&l_QueueDesc, IID_PPV_ARGS(&m_pResCmdQueue));
	assert(S_OK == l_Res);
	
	l_QueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
	l_Res = m_pDevice->CreateCommandQueue(&l_QueueDesc, IID_PPV_ARGS(&m_pComputeQueue));
	assert(S_OK == l_Res);

	l_QueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	l_Res = m_pDevice->CreateCommandQueue(&l_QueueDesc, IID_PPV_ARGS(&m_pDrawCmdQueue));
	assert(S_OK == l_Res);
		
	m_pResFence = new D3D12Fence(m_pDevice, m_pResCmdQueue);
	m_pComputeFence = new D3D12Fence(m_pDevice, m_pComputeQueue);
	m_pGraphicFence = new D3D12Fence(m_pDevice, m_pDrawCmdQueue);

	// Create descriptor heaps.
	{
		m_pShaderResourceHeap = new D3D12HeapManager(m_pDevice, 256, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
		m_pSamplerHeap = new D3D12HeapManager(m_pDevice, 8, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
		m_pRenderTargetHeap = new D3D12HeapManager(m_pDevice, 16, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		m_pDepthHeap = new D3D12HeapManager(m_pDevice, 8, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	}

	for( unsigned int i=0 ; i<D3D12_NUM_COPY_THREAD ; ++i ) m_ResThread[i] = newThread();//D3D12_COMMAND_LIST_TYPE_COPY);
	ID3D12DescriptorHeap *l_ppHeaps[] = { m_pShaderResourceHeap->getHeapInst(), m_pSamplerHeap->getHeapInst() };
	m_ResThread[m_IdleResThread].second->SetDescriptorHeaps(2, l_ppHeaps);
	m_ResourceLoop = std::thread(&D3D12Device::resourceThread, this);
	m_GraphicLoop = std::thread(&D3D12Device::graphicThread, this);

	unsigned int l_NumCore = std::thread::hardware_concurrency();
	if( 0 == l_NumCore ) l_NumCore = DEFAULT_D3D_THREAD_COUNT;
	l_NumCore *= 2;
	for( unsigned int i=0 ; i<l_NumCore ; ++i )
	{
		m_GraphicThread.push_back(newThread());
	}
	
	ProgramManager::init(new HLSLComponent());
}

void D3D12Device::shutdown()
{
	m_bLooping = false;
	m_ResourceLoop.join();
	m_GraphicLoop.join();
}

GraphicCommander* D3D12Device::commanderFactory()
{
	return new D3D12Commander(m_pRenderTargetHeap);
}

GraphicCanvas* D3D12Device::canvasFactory(wxWindow *a_pParent, wxWindowID a_ID)
{
	return new D3D12Canvas(m_pRenderTargetHeap, a_pParent, a_ID);
}

std::pair<int, int> D3D12Device::maxShaderModel()
{
	return std::make_pair(5, 1);//no version check for dx12
}

void D3D12Device::wait()
{
	for( unsigned int i=0 ; i<D3D12_NUM_COPY_THREAD ; ++i )
	{
		if( !m_TempResources[i].empty() )
		{
			m_pResFence->signal();
			m_pResFence->wait();
		}
	}

	while( !m_GraphicReadyThread.empty() || !m_GraphicBusyThread.empty() )
	{
		m_pGraphicFence->signal();
		m_pGraphicFence->wait();
	}
}

// convert part
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

unsigned int D3D12Device::getFilter(Filter::Key a_Key)
{
	switch( a_Key )
	{
		case Filter::min_mag_mip_point:								return D3D12_FILTER_MIN_MAG_MIP_POINT;
		case Filter::min_mag_point_mip_linear:						return D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR;
		case Filter::min_point_mag_linear_mip_point:				return D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
		case Filter::min_point_mag_mip_linear:						return D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR;
		case Filter::min_linear_mag_mip_point:						return D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT;
		case Filter::min_linear_mag_point_mip_linear:				return D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
		case Filter::min_mag_linear_mip_point:						return D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
		case Filter::min_mag_mip_linear:							return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		case Filter::anisotropic:									return D3D12_FILTER_ANISOTROPIC;
		case Filter::comparison_min_mag_mip_point:					return D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
		case Filter::comparison_min_mag_point_mip_linear:			return D3D12_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR;
		case Filter::comparison_min_point_mag_linear_mip_point:		return D3D12_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT;
		case Filter::comparison_min_point_mag_mip_linear:			return D3D12_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR;
		case Filter::comparison_min_linear_mag_mip_point:			return D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT;
		case Filter::comparison_min_linear_mag_point_mip_linear:	return D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
		case Filter::comparison_min_mag_linear_mip_point:			return D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
		case Filter::comparison_min_mag_mip_linear:					return D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
		case Filter::comparison_anisotropic:						return D3D12_FILTER_COMPARISON_ANISOTROPIC;
		case Filter::minimum_min_mag_mip_point:						return D3D12_FILTER_MINIMUM_MIN_MAG_MIP_POINT;
		case Filter::minimum_min_mag_point_mip_linear:				return D3D12_FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR;
		case Filter::minimum_min_point_mag_linear_mip_point:		return D3D12_FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT;
		case Filter::minimum_min_point_mag_mip_linear:				return D3D12_FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR;
		case Filter::minimum_min_linear_mag_mip_point:				return D3D12_FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT;
		case Filter::minimum_min_linear_mag_point_mip_linear:		return D3D12_FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
		case Filter::minimum_min_mag_linear_mip_point:				return D3D12_FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT;
		case Filter::minimum_min_mag_mip_linear:					return D3D12_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR;
		case Filter::minimum_anisotropic:							return D3D12_FILTER_MINIMUM_ANISOTROPIC;
		case Filter::maximum_min_mag_mip_point:						return D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_POINT;
		case Filter::maximum_min_mag_point_mip_linear:				return D3D12_FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR;
		case Filter::maximum_min_point_mag_linear_mip_point:		return D3D12_FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT;
		case Filter::maximum_min_point_mag_mip_linear:				return D3D12_FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR;
		case Filter::maximum_min_linear_mag_mip_point:				return D3D12_FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT;
		case Filter::maximum_min_linear_mag_point_mip_linear:		return D3D12_FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
		case Filter::maximum_min_mag_linear_mip_point:				return D3D12_FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT;
		case Filter::maximum_min_mag_mip_linear:					return D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR;
		case Filter::maximum_anisotropic:							return D3D12_FILTER_MAXIMUM_ANISOTROPIC;
		default:break;
	}
	assert(false && "invalid filter type");
	return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
}

unsigned int D3D12Device::getAddressMode(AddressMode::Key a_Key)
{
	switch( a_Key )
	{
		case AddressMode::wrap:			return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		case AddressMode::mirror:		return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
		case AddressMode::clamp:		return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		case AddressMode::border:		return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		case AddressMode::mirror_once:	return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
		default:break;
	}
	assert(false && "invalid address mode");
	return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
}

// texture part
int D3D12Device::allocateTexture(glm::ivec2 a_Size, PixelFormat::Key a_Format, unsigned int a_ArraySize, bool a_bCube)
{
	assert(a_Size.x >= MIN_TEXTURE_SIZE && a_Size.y >= MIN_TEXTURE_SIZE);
	unsigned int l_MipmapLevel = std::log2(std::max(a_Size.x, a_Size.y)) + 1;
	return allocateTexture(glm::ivec3(a_Size.x, a_Size.y, std::max<int>(a_ArraySize, 1)), a_Format, D3D12_RESOURCE_DIMENSION_TEXTURE2D, l_MipmapLevel, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, a_bCube && 0 == (a_ArraySize % 6));
}

int D3D12Device::allocateTexture(glm::ivec3 a_Size, PixelFormat::Key a_Format)
{
	assert(a_Size.x >= MIN_TEXTURE_SIZE && a_Size.y >= MIN_TEXTURE_SIZE && a_Size.z >= MIN_TEXTURE_SIZE);
	unsigned int l_MipmapLevel = std::log2(std::max(std::max(a_Size.x, a_Size.y), a_Size.z)) + 1;
	return allocateTexture(a_Size, a_Format, D3D12_RESOURCE_DIMENSION_TEXTURE3D, l_MipmapLevel, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
}

void D3D12Device::updateTexture(int a_ID, unsigned int a_MipmapLevel, glm::ivec2 a_Size, glm::ivec2 a_Offset, unsigned int a_Idx, void *a_pSrcData)
{
#ifdef _DEBUG
	std::shared_ptr<TextureBinder> l_pTargetTexture = m_ManagedTexture[a_ID];
	assert(nullptr != l_pTargetTexture);
	assert(TEXTYPE_SIMPLE_2D == l_pTargetTexture->m_Type);
	assert(a_Size.x >= 1 && a_Size.y >= 1);
	assert(a_Offset.x < (l_pTargetTexture->m_Size.x >> a_MipmapLevel) && a_Offset.x + a_Size.x <= (l_pTargetTexture->m_Size.x >> a_MipmapLevel) &&
		   a_Offset.y < (l_pTargetTexture->m_Size.y >> a_MipmapLevel) && a_Offset.y + a_Size.y <= (l_pTargetTexture->m_Size.y >> a_MipmapLevel) &&
		   a_Idx < (unsigned int)l_pTargetTexture->m_Size.z);
#endif
	updateTexture(a_ID, a_MipmapLevel, glm::ivec3(a_Size.x, a_Size.y, 1), glm::ivec3(a_Offset.x, a_Offset.y, a_Idx), a_pSrcData, 0xff);
}

void D3D12Device::updateTexture(int a_ID, unsigned int a_MipmapLevel, glm::ivec3 a_Size, glm::ivec3 a_Offset, void *a_pSrcData)
{
#ifdef _DEBUG
	std::shared_ptr<TextureBinder> l_pTargetTexture = m_ManagedTexture[a_ID];
	assert(nullptr != l_pTargetTexture);
	assert(TEXTYPE_SIMPLE_3D == l_pTargetTexture->m_Type);
	assert(a_Size.x >= 1 && a_Size.y >= 1 && a_Size.z >= 1);
	assert(a_Offset.x < (l_pTargetTexture->m_Size.x >> a_MipmapLevel) && a_Offset.x + a_Size.x <= (l_pTargetTexture->m_Size.x >> a_MipmapLevel) &&
		   a_Offset.y < (l_pTargetTexture->m_Size.y >> a_MipmapLevel) && a_Offset.y + a_Size.y <= (l_pTargetTexture->m_Size.y >> a_MipmapLevel) &&
		   a_Offset.z < (l_pTargetTexture->m_Size.z >> a_MipmapLevel) && a_Offset.z + a_Size.z <= (l_pTargetTexture->m_Size.z >> a_MipmapLevel));
#endif
	updateTexture(a_ID, a_MipmapLevel, a_Size, a_Offset, a_pSrcData, 0xff);
}

void D3D12Device::generateMipmap(int a_ID, unsigned int a_Level, std::shared_ptr<ShaderProgram> a_pProgram)
{
	D3D12GpuThread l_Thread = requestThread();
	ID3D12DescriptorHeap *l_ppHeaps[] = { m_pShaderResourceHeap->getHeapInst(), m_pSamplerHeap->getHeapInst() };
	l_Thread.second->SetDescriptorHeaps(2, l_ppHeaps);

	std::shared_ptr<TextureBinder> l_pTargetBinder = m_ManagedTexture[a_ID];
	assert(nullptr != l_pTargetBinder);
	
	HLSLProgram12 *l_pProgram = static_cast<HLSLProgram12 *>(a_pProgram.get());
	unsigned int l_NumConst = 0;
	std::function<D3D12_UNORDERED_ACCESS_VIEW_DESC(unsigned int)> l_UavStructFunc = nullptr;
	std::function<D3D12_SHADER_RESOURCE_VIEW_DESC(unsigned int)> l_SrvStructFunc = nullptr;
	glm::ivec3 l_Devide(8, 8, 1);
	switch( l_pTargetBinder->m_Type )
	{
		case TEXTYPE_SIMPLE_2D:
		case TEXTYPE_RENDER_TARGET_VIEW:
			l_NumConst = 2;
			l_UavStructFunc = [](unsigned int a_MipLevel)->D3D12_UNORDERED_ACCESS_VIEW_DESC
			{
				D3D12_UNORDERED_ACCESS_VIEW_DESC l_Desc = {};
				l_Desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
				l_Desc.Texture2D.MipSlice = a_MipLevel;
				return l_Desc;
			};
			l_SrvStructFunc = [](unsigned int a_MipLevel)->D3D12_SHADER_RESOURCE_VIEW_DESC
			{
				D3D12_SHADER_RESOURCE_VIEW_DESC l_Desc = {};
				l_Desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
				l_Desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
				l_Desc.Texture2D.MipLevels = 1;
				l_Desc.Texture2D.MostDetailedMip = a_MipLevel;
				return l_Desc;
			};
			break;

		case TEXTYPE_SIMPLE_3D:
			l_NumConst = 3;
			l_Devide.x = l_Devide.y = l_Devide.z = 4;
			l_UavStructFunc = [](unsigned int a_MipLevel)->D3D12_UNORDERED_ACCESS_VIEW_DESC
			{
				D3D12_UNORDERED_ACCESS_VIEW_DESC l_Desc = {};
				l_Desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
				l_Desc.Texture3D.MipSlice = a_MipLevel;
				return l_Desc;
			};
			l_SrvStructFunc = [](unsigned int a_MipLevel)->D3D12_SHADER_RESOURCE_VIEW_DESC
			{
				D3D12_SHADER_RESOURCE_VIEW_DESC l_Desc = {};
				l_Desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
				l_Desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
				l_Desc.Texture3D.MipLevels = 1;
				l_Desc.Texture3D.MostDetailedMip = a_MipLevel;
				return l_Desc;
			};
			break;

		default:break;
	}
	assert(nullptr != l_pProgram);

	l_Thread.second->SetPipelineState(l_pProgram->getPipeline());
	l_Thread.second->SetComputeRootSignature(l_pProgram->getRegDesc());
	glm::ivec3 l_Dim(l_pTargetBinder->m_Size);
	unsigned int l_B0 = l_pProgram->getConstantSlot("c_PixelSize").first;
	unsigned int l_T0 = l_pProgram->getTextureSlot(0);
	unsigned int l_U0 = l_pProgram->getUavSlot(0);

	int l_NumStep = 0 == a_Level ? l_pTargetBinder->m_MipmapLevels : a_Level;
	D3D12_RESOURCE_STATES l_OriginState = TEXTYPE_RENDER_TARGET_VIEW == l_pTargetBinder->m_Type ? D3D12_RESOURCE_STATE_RENDER_TARGET : D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	for( int i=1 ; i<(int)l_NumStep ; ++i )
	{
		l_Dim = glm::max(l_Dim / 2, glm::ivec3(1, 1, 1));
		resourceTransition(l_Thread.second, l_pTargetBinder->m_pTexture, l_OriginState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, i);
		
		D3D12_UNORDERED_ACCESS_VIEW_DESC l_UAVDesc(l_UavStructFunc(i));
		l_UAVDesc.Format = (DXGI_FORMAT)getPixelFormat(l_pTargetBinder->m_Format);
		int l_TempUavID = m_pShaderResourceHeap->newHeap(l_pTargetBinder->m_pTexture, nullptr, &l_UAVDesc);

		D3D12_SHADER_RESOURCE_VIEW_DESC l_SrcDesc(l_SrvStructFunc(i-1));
		l_SrcDesc.Format = l_UAVDesc.Format;
		int l_TempSrvID = m_pShaderResourceHeap->newHeap(l_pTargetBinder->m_pTexture, &l_SrcDesc);
		
		glm::vec3 l_PixelSize(1.0f / glm::vec3(l_Dim.x, l_Dim.y, l_Dim.z));
		l_Thread.second->SetComputeRoot32BitConstants(l_B0, l_NumConst, &l_PixelSize, 0);
		l_Thread.second->SetComputeRootDescriptorTable(l_T0, m_pShaderResourceHeap->getGpuHandle(l_TempSrvID));
		l_Thread.second->SetComputeRootDescriptorTable(l_U0, m_pShaderResourceHeap->getGpuHandle(l_TempUavID));
		l_Thread.second->Dispatch(l_Dim.x / l_Devide.x, l_Dim.y / l_Devide.y, l_Dim.z / l_Devide.z);

		m_pShaderResourceHeap->recycle(l_TempUavID);
		m_pShaderResourceHeap->recycle(l_TempSrvID);

		resourceTransition(l_Thread.second, l_pTargetBinder->m_pTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, l_OriginState, i);
	}

	l_Thread.second->Close();
	recycleThread(l_Thread);
}

void D3D12Device::copyTexture(int a_Dst, int a_Src)
{
	std::lock_guard<std::mutex> l_Guard(m_ResourceMutex);

	std::shared_ptr<TextureBinder> l_pDstBinder = m_ManagedTexture[a_Dst];
	std::shared_ptr<TextureBinder> l_pSrcBinder = m_ManagedTexture[a_Src];

	auto l_ConvertFunc = [=](TextureType a_Type) -> D3D12_RESOURCE_STATES
	{
		switch( a_Type )
		{
			case TEXTYPE_DEPTH_STENCIL_VIEW: return D3D12_RESOURCE_STATE_DEPTH_WRITE;
			case TEXTYPE_RENDER_TARGET_VIEW:; return D3D12_RESOURCE_STATE_RENDER_TARGET;
			default: return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		}
	};

	D3D12_RESOURCE_STATES l_SrcState = l_ConvertFunc(l_pSrcBinder->m_Type);
	D3D12_RESOURCE_STATES l_DstState = l_ConvertFunc(l_pDstBinder->m_Type);

	D3D12_RESOURCE_BARRIER l_TransSetting[2];
	l_TransSetting[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	l_TransSetting[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	l_TransSetting[0].Transition.pResource = l_pDstBinder->m_pTexture;
	l_TransSetting[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	l_TransSetting[0].Transition.StateBefore = l_DstState;
	l_TransSetting[0].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;

	l_TransSetting[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	l_TransSetting[1].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	l_TransSetting[1].Transition.pResource = l_pSrcBinder->m_pTexture;
	l_TransSetting[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	l_TransSetting[1].Transition.StateBefore = l_SrcState;
	l_TransSetting[1].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;

	m_ResThread[m_IdleResThread].second->ResourceBarrier(2, l_TransSetting);
	m_ResThread[m_IdleResThread].second->CopyResource(l_pDstBinder->m_pTexture, l_pSrcBinder->m_pTexture);

	l_TransSetting[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	l_TransSetting[0].Transition.StateAfter = l_DstState;
	l_TransSetting[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
	l_TransSetting[1].Transition.StateAfter = l_SrcState;

	m_ResThread[m_IdleResThread].second->ResourceBarrier(2, l_TransSetting);
}

PixelFormat::Key D3D12Device::getTextureFormat(int a_ID)
{
	return m_ManagedTexture[a_ID]->m_Format;
}

glm::ivec3 D3D12Device::getTextureSize(int a_ID)
{
	return m_ManagedTexture[a_ID]->m_Size;
}

TextureType D3D12Device::getTextureType(int a_ID)
{
	return m_ManagedTexture[a_ID]->m_Type;
}

void* D3D12Device::getTextureResource(int a_ID)
{
	return m_ManagedTexture[a_ID]->m_pTexture;
}

void D3D12Device::freeTexture(int a_ID)
{	
	std::shared_ptr<TextureBinder> l_pTargetBinder = m_ManagedTexture[a_ID];
	m_pShaderResourceHeap->recycle(l_pTargetBinder->m_HeapID);
	if( l_pTargetBinder->m_bWriteable ) m_pShaderResourceHeap->recycle(l_pTargetBinder->m_UavHeapID);
	m_ManagedTexture.release(a_ID);
}

int D3D12Device::createSampler(Filter::Key a_Filter, AddressMode::Key a_UMode, AddressMode::Key a_VMode, AddressMode::Key a_WMode, float a_MipLodBias, unsigned int a_MaxAnisotropy, CompareFunc::Key a_Func, float a_MinLod, float a_MaxLod, float a_Border0, float a_Border1, float a_Border2, float a_Border3)
{
	std::shared_ptr<SamplerBinder> l_pNewBinder = nullptr;
	unsigned int l_SamplerID = m_ManagedSampler.retain(&l_pNewBinder);

	D3D12_SAMPLER_DESC l_Desc = {};
	l_Desc.Filter = (D3D12_FILTER)getFilter(a_Filter);
	l_Desc.AddressU = (D3D12_TEXTURE_ADDRESS_MODE)getAddressMode(a_UMode);
	l_Desc.AddressV = (D3D12_TEXTURE_ADDRESS_MODE)getAddressMode(a_VMode);
	l_Desc.AddressW = (D3D12_TEXTURE_ADDRESS_MODE)getAddressMode(a_WMode);
	l_Desc.MipLODBias = a_MipLodBias;
	l_Desc.MaxAnisotropy = a_MaxAnisotropy;
	l_Desc.ComparisonFunc = (D3D12_COMPARISON_FUNC)getComapreFunc(a_Func);
	l_Desc.MinLOD = a_MinLod;
	l_Desc.MaxLOD = a_MaxLod;
	l_Desc.BorderColor[0] = a_Border0;
	l_Desc.BorderColor[1] = a_Border1;
	l_Desc.BorderColor[2] = a_Border2;
	l_Desc.BorderColor[3] = a_Border3;
	
	l_pNewBinder->m_HeapID = m_pSamplerHeap->newHeap(&l_Desc);

	return l_SamplerID;
}

void D3D12Device::freeSampler(int a_ID)
{
	std::shared_ptr<SamplerBinder> l_pTargetBinder = m_ManagedSampler[a_ID];
	m_pSamplerHeap->recycle(l_pTargetBinder->m_HeapID);
	m_ManagedSampler.release(a_ID);
}

// render target part
int D3D12Device::createRenderTarget(glm::ivec3 a_Size, PixelFormat::Key a_Format)
{
	std::shared_ptr<RenderTargetBinder> l_pNewBinder = nullptr;
	int l_Res = m_ManagedRenderTarget.retain(&l_pNewBinder);

	l_pNewBinder->m_TextureID = allocateTexture(a_Size, a_Format, D3D12_RESOURCE_DIMENSION_TEXTURE3D, 1, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	l_pNewBinder->m_pRefBinder = m_ManagedTexture[l_pNewBinder->m_TextureID];
	l_pNewBinder->m_pRefBinder->m_Type = TEXTYPE_RENDER_TARGET_VIEW;

	D3D12_RENDER_TARGET_VIEW_DESC l_RTVDesc;
	l_RTVDesc.Format = (DXGI_FORMAT)getPixelFormat(a_Format);
	l_RTVDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
	l_RTVDesc.Texture2D.MipSlice = 0;
	l_RTVDesc.Texture2D.PlaneSlice = 0;
	l_pNewBinder->m_HeapID = m_pRenderTargetHeap->newHeap(l_pNewBinder->m_pRefBinder->m_pTexture, &l_RTVDesc);

	return l_Res;
}

int D3D12Device::createRenderTarget(glm::ivec2 a_Size, PixelFormat::Key a_Format, unsigned int a_ArraySize, bool a_bCube)
{
	assert(a_ArraySize > 0);
	std::shared_ptr<RenderTargetBinder> l_pNewBinder = nullptr;
	int l_Res = m_ManagedRenderTarget.retain(&l_pNewBinder);

	bool l_bDepthStencilBuffer = a_Format == PixelFormat::d32_float_s8x24_uint ||
								a_Format == PixelFormat::d32_float ||
								a_Format == PixelFormat::d24_unorm_s8_uint ||
								a_Format == PixelFormat::d16_unorm;
	unsigned int l_MipmapLevel = std::ceill(log2f(std::max(a_Size.x, a_Size.y)));
	l_pNewBinder->m_TextureID = allocateTexture(glm::ivec3(a_Size.x, a_Size.y, a_ArraySize), a_Format, D3D12_RESOURCE_DIMENSION_TEXTURE2D, l_MipmapLevel
		, l_bDepthStencilBuffer ? D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL : (D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS | D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET), a_bCube);
	l_pNewBinder->m_pRefBinder = m_ManagedTexture[l_pNewBinder->m_TextureID];
	l_pNewBinder->m_pRefBinder->m_Type = l_bDepthStencilBuffer ? TEXTYPE_DEPTH_STENCIL_VIEW : TEXTYPE_RENDER_TARGET_VIEW;

	if( l_bDepthStencilBuffer )
	{
		D3D12_DEPTH_STENCIL_VIEW_DESC l_DSVDesc;
		l_DSVDesc.Format = (DXGI_FORMAT)getPixelFormat(a_Format);
		l_DSVDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		l_DSVDesc.Texture2D.MipSlice = 0;
		l_DSVDesc.Flags = D3D12_DSV_FLAG_NONE;
		l_pNewBinder->m_HeapID = m_pDepthHeap->newHeap(l_pNewBinder->m_pRefBinder->m_pTexture, &l_DSVDesc);
	}
	else
	{
		D3D12_RENDER_TARGET_VIEW_DESC l_RTVDesc;
		l_RTVDesc.Format = (DXGI_FORMAT)getPixelFormat(a_Format);
		l_RTVDesc.ViewDimension = a_ArraySize == 1 ? D3D12_RTV_DIMENSION_TEXTURE2D : D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
		l_RTVDesc.Texture2D.MipSlice = 0;
		l_RTVDesc.Texture2D.PlaneSlice = 0;
		l_pNewBinder->m_HeapID = m_pRenderTargetHeap->newHeap(l_pNewBinder->m_pRefBinder->m_pTexture, &l_RTVDesc);
	}

	return l_Res;
}

int D3D12Device::getRenderTargetTexture(int a_ID)
{
	return m_ManagedRenderTarget[a_ID]->m_TextureID;
}

void D3D12Device::freeRenderTarget(int a_ID)
{
	std::shared_ptr<RenderTargetBinder> l_pTargetBinder = m_ManagedRenderTarget[a_ID];
	assert(nullptr != l_pTargetBinder);
	freeTexture(l_pTargetBinder->m_TextureID);
	m_ManagedRenderTarget.release(a_ID);
}

// vertex, index buffer part
int D3D12Device::requestVertexBuffer(void *a_pInitData, unsigned int a_Slot, unsigned int a_Count, wxString a_Name)
{
	unsigned int l_Stride = getVertexSlotStride(a_Slot);
	std::shared_ptr<VertexBinder> l_pNewVtxBuffer = nullptr;
	unsigned int l_VtxID = m_ManagedVertexBuffer.retain(&l_pNewVtxBuffer);
	l_pNewVtxBuffer->m_pVtxRes = initSizedResource(l_Stride * a_Count, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
	
	if( !a_Name.empty() ) l_pNewVtxBuffer->m_pVtxRes->SetName(a_Name.wc_str());
	if( nullptr != a_pInitData ) updateResourceData(l_pNewVtxBuffer->m_pVtxRes, a_pInitData, l_Stride * a_Count);

	l_pNewVtxBuffer->m_VtxView.BufferLocation = l_pNewVtxBuffer->m_pVtxRes->GetGPUVirtualAddress();
	l_pNewVtxBuffer->m_VtxView.SizeInBytes = l_Stride * a_Count;
	l_pNewVtxBuffer->m_VtxView.StrideInBytes = l_Stride;

	return l_VtxID;
}

void D3D12Device::updateVertexBuffer(int a_ID, void *a_pData, unsigned int a_SizeInByte)
{
	assert(nullptr != a_pData);
	std::shared_ptr<VertexBinder> l_pVtxBuffer = m_ManagedVertexBuffer[a_ID];
	updateResourceData(l_pVtxBuffer->m_pVtxRes, a_pData, a_SizeInByte);
}

void* D3D12Device::getVertexResource(int a_ID)
{
	return m_ManagedVertexBuffer[a_ID]->m_pVtxRes;
}

void D3D12Device::freeVertexBuffer(int a_ID)
{
	m_ManagedVertexBuffer.release(a_ID);
}

int D3D12Device::requestIndexBuffer(void *a_pInitData, PixelFormat::Key a_Fmt, unsigned int a_Count, wxString a_Name)
{
	unsigned int l_Stride = std::max(getPixelSize(a_Fmt) / 8, 1u);
	assert(PixelFormat::r8_uint == a_Fmt || PixelFormat::r16_uint == a_Fmt || PixelFormat::r32_uint == a_Fmt);

	std::shared_ptr<IndexBinder> l_pNewIndexBuffer = nullptr;
	unsigned int l_ID = m_ManagedIndexBuffer.retain(&l_pNewIndexBuffer);
	l_pNewIndexBuffer->m_pIndexRes = initSizedResource(l_Stride * a_Count, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
	
	if( !a_Name.empty() ) l_pNewIndexBuffer->m_pIndexRes->SetName(a_Name.wc_str());
	if( nullptr != a_pInitData ) updateResourceData(l_pNewIndexBuffer->m_pIndexRes, a_pInitData, l_Stride * a_Count);

	l_pNewIndexBuffer->m_IndexView.BufferLocation = l_pNewIndexBuffer->m_pIndexRes->GetGPUVirtualAddress();
	l_pNewIndexBuffer->m_IndexView.Format = (DXGI_FORMAT)getPixelFormat(a_Fmt);
	l_pNewIndexBuffer->m_IndexView.SizeInBytes = l_Stride * a_Count;

	return l_ID;
}

void D3D12Device::updateIndexBuffer(int a_ID, void *a_pData, unsigned int a_SizeInByte)
{
	assert(nullptr != a_pData);
	std::shared_ptr<IndexBinder> l_pIndexBuffer = m_ManagedIndexBuffer[a_ID];
	updateResourceData(l_pIndexBuffer->m_pIndexRes, a_pData, a_SizeInByte);
}

void* D3D12Device::getIndexResource(int a_ID)
{
	return m_ManagedIndexBuffer[a_ID]->m_pIndexRes;
}

void D3D12Device::freeIndexBuffer(int a_ID)
{
	m_ManagedIndexBuffer.release(a_ID);
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12Device::getTextureGpuHandle(int a_ID, GraphicCommander::TextureBindType a_Type)
{
	switch( a_Type )
	{
		case GraphicCommander::BIND_NORMAL_TEXTURE:
			return m_pShaderResourceHeap->getGpuHandle(m_ManagedTexture[a_ID]->m_HeapID);

		case GraphicCommander::BIND_RENDER_TARGET:
			return m_pShaderResourceHeap->getGpuHandle(m_ManagedRenderTarget[a_ID]->m_pRefBinder->m_HeapID);

		case GraphicCommander::BIND_UAV_TEXTURE:
			return m_pShaderResourceHeap->getGpuHandle(m_ManagedTexture[a_ID]->m_UavHeapID);

		default:break;
	}
	assert(false && "invalid binding type");
	return {};
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12Device::getSamplerGpuHandle(int a_ID)
{
	std::shared_ptr<SamplerBinder> l_pTargetBinder = m_ManagedSampler[a_ID];
	return m_pSamplerHeap->getGpuHandle(l_pTargetBinder->m_HeapID);
}

D3D12_GPU_VIRTUAL_ADDRESS D3D12Device::getTextureGpuAddress(int a_ID, bool a_bRenderTarget)
{
	std::shared_ptr<TextureBinder> l_pTargetBinder = a_bRenderTarget ? m_ManagedRenderTarget[a_ID]->m_pRefBinder : m_ManagedTexture[a_ID];
	return l_pTargetBinder->m_pTexture->GetGPUVirtualAddress();
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12Device::getRenderTargetCpuHandle(int a_ID, bool a_bDepth)
{
	return (a_bDepth ? m_pDepthHeap : m_pRenderTargetHeap)->getCpuHandle(m_ManagedRenderTarget[a_ID]->m_HeapID);
}

ID3D12Resource* D3D12Device::getRenderTargetResource(int a_ID)
{
	return m_ManagedRenderTarget[a_ID]->m_pRefBinder->m_pTexture;
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12Device::getConstBufferGpuHandle(int a_ID)
{
	return m_pShaderResourceHeap->getGpuHandle(m_ManagedConstBuffer[a_ID]->m_HeapID);
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12Device::getUnorderAccessBufferGpuHandle(int a_ID)
{
	return m_pShaderResourceHeap->getGpuHandle(m_ManagedUavBuffer[a_ID]->m_HeapID);
}

D3D12_GPU_VIRTUAL_ADDRESS D3D12Device::getConstBufferGpuAddress(int a_ID)
{
	return m_ManagedConstBuffer[a_ID]->m_pResource->GetGPUVirtualAddress();
}

D3D12_GPU_VIRTUAL_ADDRESS D3D12Device::getUnorderAccessBufferGpuAddress(int a_ID)
{
	return m_ManagedUavBuffer[a_ID]->m_pResource->GetGPUVirtualAddress();
}

D3D12_VERTEX_BUFFER_VIEW& D3D12Device::getVertexBufferView(int a_ID)
{
	return m_ManagedVertexBuffer[a_ID]->m_VtxView;
}

D3D12_INDEX_BUFFER_VIEW& D3D12Device::getIndexBufferView(int a_ID)
{
	return m_ManagedIndexBuffer[a_ID]->m_IndexView;
}

ID3D12Resource* D3D12Device::getIndirectCommandBuffer(int a_ID)
{
	return m_ManagedIndirectCommandBuffer[a_ID]->m_pResource;
}

// cbv part
int D3D12Device::requestConstBuffer(char* &a_pOutputBuff, unsigned int a_Size)
{
	a_Size = (a_Size + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1);// const buffer alignment

	ID3D12Resource *l_pNewResource = initSizedResource(a_Size, D3D12_HEAP_TYPE_UPLOAD);

	std::shared_ptr<ConstBufferBinder> l_pTargetBinder = nullptr;
	unsigned int l_BuffID = m_ManagedConstBuffer.retain(&l_pTargetBinder);

	HRESULT l_Res = l_pNewResource->Map(0, nullptr, reinterpret_cast<void**>(&l_pTargetBinder->m_pCurrBuff));
	if( S_OK != l_Res )
	{
		wxMessageBox(wxT("failed to map shared cbv buff"), wxT("D3D12Device::requestConstBuffer"));
		return -1;
	}
	l_pTargetBinder->m_pResource = l_pNewResource;
	l_pTargetBinder->m_CurrSize = a_Size;
	a_pOutputBuff = l_pTargetBinder->m_pCurrBuff;

	D3D12_CONSTANT_BUFFER_VIEW_DESC l_BuffDesc;
	l_BuffDesc.SizeInBytes = a_Size;
	l_BuffDesc.BufferLocation = l_pTargetBinder->m_pResource->GetGPUVirtualAddress();
	l_pTargetBinder->m_HeapID = m_pShaderResourceHeap->newHeap(&l_BuffDesc);

	return l_BuffID;
}

char* D3D12Device::getConstBufferContainer(int a_ID)
{
	return m_ManagedConstBuffer[a_ID]->m_pCurrBuff;
}

void* D3D12Device::getConstBufferResource(int a_ID)
{
	return m_ManagedConstBuffer[a_ID]->m_pResource;
}

void D3D12Device::freeConstBuffer(int a_ID)
{
	std::shared_ptr<ConstBufferBinder> l_pTargetBinder = m_ManagedConstBuffer[a_ID];
	l_pTargetBinder->m_pResource->Unmap(0, nullptr);
	m_pShaderResourceHeap->recycle(l_pTargetBinder->m_HeapID);
	SAFE_RELEASE(l_pTargetBinder->m_pResource)

	m_ManagedConstBuffer.release(a_ID);
}

// uav part
unsigned int D3D12Device::requestUavBuffer(char* &a_pOutputBuff, unsigned int a_UnitSize, unsigned int a_ElementCount)
{
	unsigned int l_TotalSize = a_UnitSize * a_ElementCount;

	ID3D12Resource *l_pNewResource = initSizedResource(l_TotalSize, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
		
	std::shared_ptr<UnorderAccessBufferBinder> l_pTargetBinder = nullptr;
	unsigned int l_BuffID = m_ManagedUavBuffer.retain(&l_pTargetBinder);

	a_pOutputBuff = new char[l_TotalSize];
	memset(a_pOutputBuff, 0, sizeof(char) * l_TotalSize);

	l_pTargetBinder->m_pResource = l_pNewResource;
	l_pTargetBinder->m_CurrSize = l_TotalSize;
	l_pTargetBinder->m_ElementSize = a_UnitSize;
	l_pTargetBinder->m_ElementCount = a_ElementCount;
	l_pTargetBinder->m_pCurrBuff = a_pOutputBuff;
	l_pTargetBinder->m_pCounterResource = initSizedResource(sizeof(int), D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	D3D12_UNORDERED_ACCESS_VIEW_DESC l_UAVDesc;
	l_UAVDesc.Format = DXGI_FORMAT_UNKNOWN;
	l_UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    l_UAVDesc.Buffer.FirstElement = 0;
	l_UAVDesc.Buffer.NumElements = a_ElementCount;
	l_UAVDesc.Buffer.StructureByteStride = a_UnitSize;
	l_UAVDesc.Buffer.CounterOffsetInBytes = 0;
	l_UAVDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

	l_pTargetBinder->m_HeapID = m_pShaderResourceHeap->newHeap(l_pTargetBinder->m_pResource, l_pTargetBinder->m_pCounterResource, &l_UAVDesc);

	return l_BuffID;
}

void D3D12Device::resizeUavBuffer(int a_ID, char* &a_pOutputBuff, unsigned int a_ElementCount)
{
	std::shared_ptr<UnorderAccessBufferBinder> l_pTargetBinder = m_ManagedUavBuffer[a_ID];
	unsigned int l_TotalSize = a_ElementCount * l_pTargetBinder->m_ElementSize;

	ID3D12Resource *l_pNewResource = initSizedResource(l_TotalSize, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	unsigned int l_NewSize = std::min(l_pTargetBinder->m_CurrSize, a_ElementCount * l_pTargetBinder->m_ElementSize);
	char *l_pNewBuffPtr = new char[l_NewSize];
	memcpy(l_pNewBuffPtr, l_pTargetBinder->m_pCurrBuff, l_NewSize);
	a_pOutputBuff = l_pNewBuffPtr;

	SAFE_RELEASE(l_pTargetBinder->m_pResource)
	l_pTargetBinder->m_pResource = l_pNewResource;
	SAFE_DELETE_ARRAY(l_pTargetBinder->m_pCurrBuff)
	l_pTargetBinder->m_pCurrBuff = l_pNewBuffPtr;
	l_pTargetBinder->m_ElementCount = a_ElementCount;
	l_pTargetBinder->m_CurrSize = a_ElementCount * l_pTargetBinder->m_ElementSize;

	D3D12_UNORDERED_ACCESS_VIEW_DESC l_UAVDesc;
	l_UAVDesc.Format = DXGI_FORMAT_UNKNOWN;
	l_UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    l_UAVDesc.Buffer.FirstElement = 0;
	l_UAVDesc.Buffer.NumElements = a_ElementCount;
	l_UAVDesc.Buffer.StructureByteStride = l_pTargetBinder->m_ElementSize;
	l_UAVDesc.Buffer.CounterOffsetInBytes = 0;
	l_UAVDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
	
	m_pShaderResourceHeap->recycle(l_pTargetBinder->m_HeapID);
	l_pTargetBinder->m_HeapID = m_pShaderResourceHeap->newHeap(l_pTargetBinder->m_pResource, l_pTargetBinder->m_pCounterResource, &l_UAVDesc);

	GraphicDevice::syncUavBuffer(true, 1u, a_ID);
}

char* D3D12Device::getUavBufferContainer(int a_ID)
{
	return m_ManagedUavBuffer[a_ID]->m_pCurrBuff;
}

void* D3D12Device::getUavBufferResource(int a_ID)
{
	return m_ManagedUavBuffer[a_ID]->m_pResource;
}

void D3D12Device::syncUavBuffer(bool a_bToGpu, std::vector<unsigned int> &a_BuffIDList)
{
	std::vector< std::tuple<unsigned int, unsigned int, unsigned int> > l_BuffIDListWithOffset(a_BuffIDList.size());
	for( unsigned int i=0 ; i<a_BuffIDList.size() ; ++i )
	{
		std::shared_ptr<UnorderAccessBufferBinder> l_pTargetBinder = m_ManagedUavBuffer[a_BuffIDList[i]];
		std::get<0>(l_BuffIDListWithOffset[i]) = a_BuffIDList[i];
		std::get<1>(l_BuffIDListWithOffset[i]) = 0;
		std::get<2>(l_BuffIDListWithOffset[i]) = l_pTargetBinder->m_CurrSize;
	}
	syncUavBuffer(a_bToGpu, l_BuffIDListWithOffset);
}

void D3D12Device::syncUavBuffer(bool a_bToGpu, std::vector< std::tuple<unsigned int, unsigned int, unsigned int> > &a_BuffIDList)
{
	assert(!a_BuffIDList.empty());

	if( a_bToGpu )
	{
		D3D12_RESOURCE_BARRIER l_BaseBarrier;
		l_BaseBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		l_BaseBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		l_BaseBarrier.Transition.pResource = nullptr;
		l_BaseBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		l_BaseBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		l_BaseBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
		std::vector<D3D12_RESOURCE_BARRIER> l_TransToSetting(a_BuffIDList.size(), l_BaseBarrier);

		l_BaseBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		l_BaseBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		std::vector<D3D12_RESOURCE_BARRIER> l_TransBackSetting(a_BuffIDList.size(), l_BaseBarrier);

		{
			std::lock_guard<std::mutex> l_Guard(m_ResourceMutex);
			std::vector<ID3D12Resource *> l_ResArray(a_BuffIDList.size(), nullptr);
		
			#pragma	omp parallel for
			for( int i=0 ; i<(int)a_BuffIDList.size() ; ++i )
			{
				D3D12_RANGE l_MapRange;
				l_MapRange.Begin = 0;
				l_MapRange.End = std::get<2>(a_BuffIDList[i]) - std::get<1>(a_BuffIDList[i]);

				std::shared_ptr<UnorderAccessBufferBinder> l_pTargetBinder = m_ManagedUavBuffer[std::get<0>(a_BuffIDList[i])];
				l_ResArray[i] = initSizedResource(l_MapRange.End - l_MapRange.Begin, D3D12_HEAP_TYPE_UPLOAD);
				updateResourceData(l_ResArray[i], l_pTargetBinder->m_pCurrBuff + std::get<1>(a_BuffIDList[i]), l_MapRange.End);
				m_TempResources[m_IdleResThread].push_back(l_ResArray[i]);

				l_TransToSetting[i].Transition.pResource = l_pTargetBinder->m_pResource;
				l_TransBackSetting[i].Transition.pResource = l_pTargetBinder->m_pResource;
			}

			m_ResThread[m_IdleResThread].second->ResourceBarrier(l_TransToSetting.size(), &(l_TransToSetting[0]));
		
			#pragma	omp parallel for
			for( int i=0 ; i<(int)a_BuffIDList.size() ; ++i )
			{
				std::shared_ptr<UnorderAccessBufferBinder> l_pTargetBinder = m_ManagedUavBuffer[std::get<0>(a_BuffIDList[i])];
				m_ResThread[m_IdleResThread].second->CopyBufferRegion(l_pTargetBinder->m_pResource, std::get<1>(a_BuffIDList[i])
					, l_ResArray[i], 0, std::get<2>(a_BuffIDList[i]) - std::get<1>(a_BuffIDList[i]));
			}
	
			m_ResThread[m_IdleResThread].second->ResourceBarrier(l_TransBackSetting.size(), &(l_TransBackSetting[0]));
		}
	}
	else
	{
		D3D12_RESOURCE_BARRIER l_BaseTransSetting;
		l_BaseTransSetting.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		l_BaseTransSetting.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		l_BaseTransSetting.Transition.pResource = nullptr;
		l_BaseTransSetting.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		l_BaseTransSetting.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		l_BaseTransSetting.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
		std::vector<D3D12_RESOURCE_BARRIER> l_TransToSettings(a_BuffIDList.size(), l_BaseTransSetting);
		
		l_BaseTransSetting.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
		l_BaseTransSetting.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		std::vector<D3D12_RESOURCE_BARRIER> l_TransBackSettings(a_BuffIDList.size(), l_BaseTransSetting);

		{
			std::lock_guard<std::mutex> l_Guard(m_ResourceMutex);

			m_ResThread[m_IdleResThread].second->ResourceBarrier(l_TransToSettings.size(), &(l_TransToSettings[0]));
			
			#pragma	omp parallel for
			for( int i=0 ; i<(int)a_BuffIDList.size() ; ++i )
			{
				std::shared_ptr<UnorderAccessBufferBinder> l_pTargetBinder = m_ManagedUavBuffer[std::get<0>(a_BuffIDList[i])];

				ReadBackBuffer l_ReadBackData;
				l_ReadBackData.m_pTempResource = initSizedResource(std::get<2>(a_BuffIDList[i]) - std::get<1>(a_BuffIDList[i]), D3D12_HEAP_TYPE_READBACK, D3D12_RESOURCE_STATE_COPY_DEST);
				l_ReadBackData.m_pTargetBuffer = reinterpret_cast<unsigned char *>(l_pTargetBinder->m_pCurrBuff + std::get<1>(a_BuffIDList[i]));
				l_ReadBackData.m_Size = std::get<2>(a_BuffIDList[i]) - std::get<1>(a_BuffIDList[i]);
				m_ReadBackContainer[m_IdleResThread].push_back(l_ReadBackData);
				m_TempResources[m_IdleResThread].push_back(l_ReadBackData.m_pTempResource);
		
				m_ResThread[m_IdleResThread].second->CopyBufferRegion(l_ReadBackData.m_pTempResource, 0
					, l_pTargetBinder->m_pResource, std::get<1>(a_BuffIDList[i]), std::get<2>(a_BuffIDList[i]) - std::get<1>(a_BuffIDList[i]));
			}

			m_ResThread[m_IdleResThread].second->ResourceBarrier(l_TransBackSettings.size(), &(l_TransBackSettings[0]));
		}
	}
}

void D3D12Device::freeUavBuffer(int a_ID)
{
	std::shared_ptr<UnorderAccessBufferBinder> l_pTargetBinder = m_ManagedUavBuffer[a_ID];
	m_pShaderResourceHeap->recycle(l_pTargetBinder->m_HeapID);
	SAFE_RELEASE(l_pTargetBinder->m_pResource)
	SAFE_RELEASE(l_pTargetBinder->m_pCounterResource)
	SAFE_DELETE_ARRAY(l_pTargetBinder->m_pCurrBuff)

	m_ManagedUavBuffer.release(a_ID);
}

// misc part
int D3D12Device::requestIndrectCommandBuffer(char* &a_pOutputBuff, unsigned int a_Size)
{
	ID3D12Resource *l_pNewResource = initSizedResource(a_Size, D3D12_HEAP_TYPE_UPLOAD);

	std::shared_ptr<IndirectCommandBinder> l_pTargetBinder = nullptr;
	unsigned int l_BuffID = m_ManagedIndirectCommandBuffer.retain(&l_pTargetBinder);

	HRESULT l_Res = l_pNewResource->Map(0, nullptr, reinterpret_cast<void**>(&l_pTargetBinder->m_pTargetBuffer));
	if( S_OK != l_Res )
	{
		wxMessageBox(wxT("failed to indirect buff"), wxT("D3D12Device::requestIndrectCommandBuffer"));
		return -1;
	}
	l_pTargetBinder->m_pResource = l_pNewResource;
	a_pOutputBuff = l_pTargetBinder->m_pTargetBuffer;
	return l_BuffID;
}

void D3D12Device::freeIndrectCommandBuffer(int a_BuffID)
{
	std::shared_ptr<IndirectCommandBinder> l_pTargetBinder = m_ManagedIndirectCommandBuffer[a_BuffID];
	SAFE_RELEASE(l_pTargetBinder->m_pResource);
	m_ManagedIndirectCommandBuffer.release(a_BuffID);
}

// thread part
D3D12GpuThread D3D12Device::requestThread()
{
	std::lock_guard<std::mutex> l_Guard(m_ThreadMutex);
	if( m_GraphicThread.empty() )
	{
		D3D12GpuThread l_Res = newThread();
		return l_Res;
	}

	D3D12GpuThread l_Res = m_GraphicThread.front();
	m_GraphicThread.pop_front();

	return l_Res;
}

void D3D12Device::recycleThread(D3D12GpuThread a_Thread)
{
	std::lock_guard<std::mutex> l_Guard(m_ThreadMutex);
	m_GraphicReadyThread.push_back(a_Thread);
}

void D3D12Device::addPresentCanvas(D3D12Canvas *a_pCanvas)
{
	std::lock_guard<std::mutex> l_Guard(m_ThreadMutex);
	m_PresentCanvas.push_back(a_pCanvas);
}

void D3D12Device::resourceThread()
{
	while( m_bLooping )
	{
		while( m_TempResources[m_IdleResThread].empty() )
		{
			std::this_thread::yield();
			if( !m_bLooping ) return;
		}
		
		{
			std::lock_guard<std::mutex> l_Guard(m_ResourceMutex);

			m_ResThread[m_IdleResThread].second->Close();
			ID3D12CommandList* l_CmdArray[] = {m_ResThread[m_IdleResThread].second};
			m_pResCmdQueue->ExecuteCommandLists(1, l_CmdArray);
			
			m_IdleResThread = 1 - m_IdleResThread;
			ID3D12DescriptorHeap *l_ppHeaps[] = { m_pShaderResourceHeap->getHeapInst(), m_pSamplerHeap->getHeapInst() };
			m_ResThread[m_IdleResThread].second->SetDescriptorHeaps(2, l_ppHeaps);
		}
		
		m_pResFence->signal();
		m_pResFence->wait();

		unsigned int l_Original = 1 - m_IdleResThread;
		m_ResThread[l_Original].first->Reset();
		m_ResThread[l_Original].second->Reset(m_ResThread[l_Original].first, nullptr);
		
		#pragma	omp parallel for
		for( int i=0 ; i<(int)m_ReadBackContainer[l_Original].size() ; ++i ) m_ReadBackContainer[l_Original][i].readback();
		m_ReadBackContainer[l_Original].clear();

		for( unsigned int i=0 ; i<m_TempResources[l_Original].size() ; ++i ) m_TempResources[l_Original][i]->Release();
		m_TempResources[l_Original].clear();
	}
}

void D3D12Device::graphicThread()
{
	while( m_bLooping )
	{
		while( m_GraphicReadyThread.empty() )
		{
			std::this_thread::yield();
			if( !m_bLooping ) return;
		}

		{
			std::lock_guard<std::mutex> l_Guard(m_ThreadMutex);
			m_GraphicBusyThread.resize(m_GraphicReadyThread.size());
			std::copy(m_GraphicReadyThread.begin(), m_GraphicReadyThread.end(), m_GraphicBusyThread.begin());
			m_GraphicReadyThread.clear();

			std::vector<ID3D12CommandList *> l_CmdArray(m_GraphicBusyThread.size());
			#pragma	omp parallel for
			for( int i=0 ; i<(int)l_CmdArray.size() ; ++i ) l_CmdArray[i] = m_GraphicBusyThread[i].second;
			m_pDrawCmdQueue->ExecuteCommandLists(1, l_CmdArray.data());

			for( int i=0 ; i<(int)m_PresentCanvas.size() ; ++i ) m_PresentCanvas[i]->presentImp();

			m_pGraphicFence->signal();
			m_pGraphicFence->wait();
		}

		{
			std::lock_guard<std::mutex> l_Guard(m_ThreadMutex);
			for( unsigned int i=0 ; i<m_GraphicBusyThread.size() ; ++i )
			{
				m_GraphicBusyThread[i].first->Reset();
				m_GraphicBusyThread[i].second->Reset(m_GraphicBusyThread[i].first, nullptr);
				m_GraphicThread.push_back(m_GraphicBusyThread[i]);
			}
			for( int i=0 ; i<(int)m_PresentCanvas.size() ; ++i ) m_PresentCanvas[i]->resizeBackBuffer();
			m_PresentCanvas.clear();
			m_GraphicBusyThread.clear();
		}
	}
}

ID3D12Resource* D3D12Device::initSizedResource(unsigned int a_Size, D3D12_HEAP_TYPE a_HeapType, D3D12_RESOURCE_STATES a_InitState, D3D12_RESOURCE_FLAGS a_Flag)
{
	D3D12_HEAP_PROPERTIES l_HeapProp;
	l_HeapProp.Type = a_HeapType;
	l_HeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	l_HeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	l_HeapProp.CreationNodeMask = 0;
	l_HeapProp.VisibleNodeMask = 0;

	D3D12_RESOURCE_DESC l_ResDesc;
	l_ResDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	l_ResDesc.Alignment = 0;
	l_ResDesc.Width = a_Size;
	l_ResDesc.Height = 1;
	l_ResDesc.DepthOrArraySize = 1;
	l_ResDesc.MipLevels = 1;
	l_ResDesc.Format = DXGI_FORMAT_UNKNOWN;
	l_ResDesc.SampleDesc.Count = 1;
	l_ResDesc.SampleDesc.Quality = 0;
	l_ResDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	l_ResDesc.Flags = a_Flag;

	ID3D12Resource *l_pNewResource = nullptr;
	HRESULT l_Res = m_pDevice->CreateCommittedResource(&l_HeapProp, D3D12_HEAP_FLAG_NONE, &l_ResDesc, a_InitState, nullptr, IID_PPV_ARGS(&l_pNewResource));
	if( S_OK != l_Res )
	{
		wxMessageBox(wxT("failed to create shared constant buffer"), wxT("D3D12Device::initSizedResource"));
		SAFE_RELEASE(l_pNewResource)
		return nullptr;
	}
	return l_pNewResource;
}

void D3D12Device::updateResourceData(ID3D12Resource *a_pRes, void *a_pSrcData, unsigned int a_SizeInByte)
{
	unsigned char* l_pDataBegin = nullptr;
	a_pRes->Map(0, nullptr, reinterpret_cast<void**>(&l_pDataBegin));
	memcpy(l_pDataBegin, a_pSrcData, a_SizeInByte);
	a_pRes->Unmap(0, nullptr);
}

int D3D12Device::allocateTexture(glm::ivec3 a_Size, PixelFormat::Key a_Format, D3D12_RESOURCE_DIMENSION a_Dim, unsigned int a_MipmapLevel, unsigned int a_Flag, bool a_bCube)
{
	std::shared_ptr<TextureBinder> l_pNewBinder = nullptr;
	unsigned int l_TextureID = m_ManagedTexture.retain(&l_pNewBinder);

	l_pNewBinder->m_bWriteable = false;
	l_pNewBinder->m_Size = a_Size;
	l_pNewBinder->m_Format = a_Format;
	switch( a_Dim )
	{
		case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
			if( 1 == a_Size.z ) l_pNewBinder->m_Type = TEXTYPE_SIMPLE_2D;
			else if( a_bCube ) l_pNewBinder->m_Type = a_Size.z == 6 ? TEXTYPE_SIMPLE_CUBE : TEXTYPE_SIMPLE_CUBE_ARRAY;
			else l_pNewBinder->m_Type = TEXTYPE_SIMPLE_2D_ARRAY;
			break;
		case D3D12_RESOURCE_DIMENSION_TEXTURE3D: l_pNewBinder->m_Type = TEXTYPE_SIMPLE_3D; break;
		default:break;
	}
	l_pNewBinder->m_MipmapLevels = a_MipmapLevel;

	D3D12_HEAP_PROPERTIES l_HeapProp = {};
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
		wxMessageBox(wxT("Failed to create texture buffer"), wxT("D3D12Device::allocateTexture"));
		return -1;
	}
	
	if( 0 == (a_Flag & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) )
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC l_SrvDesc = {};
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

		bool l_bUseMsaa = 1 == m_MsaaSetting.Count && 0 == m_MsaaSetting.Quality;
		switch( l_pNewBinder->m_Type )
		{
			case TEXTYPE_SIMPLE_2D:
			case TEXTYPE_RENDER_TARGET_VIEW:
			case TEXTYPE_DEPTH_STENCIL_VIEW:
				if( !l_bUseMsaa )
				{
					l_SrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
					l_SrvDesc.Texture2D.MipLevels = a_MipmapLevel;
					l_SrvDesc.Texture2D.MostDetailedMip = 0;
					l_SrvDesc.Texture2D.PlaneSlice = 0;
					l_SrvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
				}
				else l_SrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
				break;

			case TEXTYPE_SIMPLE_2D_ARRAY:
				if( !l_bUseMsaa )
				{
					l_SrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
					l_SrvDesc.Texture2DArray.ArraySize = a_Size.z;
					l_SrvDesc.Texture2DArray.FirstArraySlice = 0;
					l_SrvDesc.Texture2DArray.MipLevels = a_MipmapLevel;
					l_SrvDesc.Texture2DArray.MostDetailedMip = 0;
					l_SrvDesc.Texture2DArray.PlaneSlice = 0;
					l_SrvDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
				}
				else
				{
					l_SrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
					l_SrvDesc.Texture2DMSArray.ArraySize = a_Size.z;
					l_SrvDesc.Texture2DMSArray.FirstArraySlice = 0;
				}
				break;

			case TEXTYPE_SIMPLE_CUBE:
				l_SrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
				l_SrvDesc.TextureCube.MipLevels = a_MipmapLevel;
				l_SrvDesc.TextureCube.MostDetailedMip = 0;
				l_SrvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
				break;

			case TEXTYPE_SIMPLE_CUBE_ARRAY:
				l_SrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
				l_SrvDesc.TextureCubeArray.First2DArrayFace = 0;
				l_SrvDesc.TextureCubeArray.MipLevels = a_MipmapLevel;
				l_SrvDesc.TextureCubeArray.MostDetailedMip = 0;
				l_SrvDesc.TextureCubeArray.NumCubes = a_Size.z / 6;
				l_SrvDesc.TextureCubeArray.ResourceMinLODClamp = 0.0f;
				break;

			case TEXTYPE_SIMPLE_3D:
				l_SrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
				l_SrvDesc.Texture3D.MipLevels = a_MipmapLevel;
				l_SrvDesc.Texture3D.MostDetailedMip = 0;
				l_SrvDesc.Texture3D.ResourceMinLODClamp = 0.0f;
				break;

			default:break;
		}
		l_pNewBinder->m_HeapID = m_pShaderResourceHeap->newHeap(l_pNewBinder->m_pTexture, &l_SrvDesc);

		if( 0 != (a_Flag & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) )
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC l_UavDesc = {};
			l_UavDesc.Format = l_TexResDesc.Format;
			switch( l_pNewBinder->m_Type )
			{
				case TEXTYPE_SIMPLE_2D:
				case TEXTYPE_RENDER_TARGET_VIEW:
				case TEXTYPE_DEPTH_STENCIL_VIEW:
					l_UavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
					l_UavDesc.Texture2D.MipSlice = 0;
					l_UavDesc.Texture2D.PlaneSlice = 0;
					break;

				case TEXTYPE_SIMPLE_2D_ARRAY:
					l_UavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
					l_UavDesc.Texture2DArray.ArraySize = a_Size.z;
					l_UavDesc.Texture2DArray.FirstArraySlice = 0;
					l_UavDesc.Texture2DArray.MipSlice = 0;
					l_UavDesc.Texture2DArray.PlaneSlice = 0;
					break;

				case TEXTYPE_SIMPLE_3D:
					l_UavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
					l_UavDesc.Texture3D.WSize = a_Size.z;
					l_UavDesc.Texture3D.MipSlice = 0;
					l_UavDesc.Texture3D.FirstWSlice = 0;
					break;
					
				//case TEXTYPE_SIMPLE_CUBE:
				//case TEXTYPE_SIMPLE_CUBE_ARRAY:
				default:
					assert(false && "invalid uav format");
					break;
			}
			l_pNewBinder->m_bWriteable = true;
			l_pNewBinder->m_UavHeapID = m_pShaderResourceHeap->newHeap(l_pNewBinder->m_pTexture, nullptr, &l_UavDesc);
		}
	}

	return l_TextureID;
}

void D3D12Device::updateTexture(int a_ID, unsigned int a_MipmapLevel, glm::ivec3 a_Size, glm::ivec3 a_Offset, void *a_pSrcData, unsigned char a_FillColor)
{
	std::shared_ptr<TextureBinder> l_pTargetTexture = m_ManagedTexture[a_ID];
	
	unsigned int l_PixelSize = std::max(getPixelSize(l_pTargetTexture->m_Format) / 8, 1u);
	std::vector<unsigned char> l_EmptyBuff;
	if( nullptr == a_pSrcData )
	{
		l_EmptyBuff.resize(l_PixelSize * a_Size.x * a_Size.y * a_Size.z, a_FillColor);
		a_pSrcData = static_cast<void*>(&(l_EmptyBuff.front()));
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

		unsigned char *l_pDataBegin = nullptr;
		l_Res = l_pUploader->Map(0, nullptr, reinterpret_cast<void**>(&l_pDataBegin));
		switch( l_pTargetTexture->m_Type )
		{
			case TEXTYPE_SIMPLE_2D:{
				#pragma	omp parallel for
				for( int y=0 ; y<a_Size.y ; ++y )
				{
					unsigned char *l_pDstStart = l_pDataBegin + ((a_Offset.x + y) * l_RowPitch + a_Offset.x * l_PixelSize);
					unsigned char *l_pSrcStart = static_cast<unsigned char *>(a_pSrcData) + (y * a_Size.x * l_PixelSize);
					memcpy(l_pDstStart, l_pSrcStart, a_Size.x * l_PixelSize);
				}
				}break;

			case TEXTYPE_SIMPLE_3D:{
				#pragma	omp parallel for
				for( int z=0 ; z<a_Size.z ; ++z )
				{
					unsigned int l_DstSliceOffset = (a_Offset.z + z) * l_SlicePitch;
					unsigned int l_SrcSliceOffset = z * a_Size.x * a_Size.y * l_PixelSize;
					for( int y=0 ; y<a_Size.y ; ++y )
					{
						unsigned char *l_pDstStart = l_pDataBegin + (l_DstSliceOffset + (a_Offset.x + y) * l_RowPitch + a_Offset.x * l_PixelSize);
						unsigned char *l_pSrcStart = static_cast<unsigned char *>(a_pSrcData) + l_SrcSliceOffset + (y * a_Size.x * l_PixelSize);
						memcpy(l_pDstStart, l_pSrcStart, a_Size.x * l_PixelSize);
					}
				}
				}break;

			default:break;
		}
		l_pUploader->Unmap(0, nullptr);
	}

	{// lock for command assign
		std::lock_guard<std::mutex> l_Guard(m_ResourceMutex);

		m_TempResources[m_IdleResThread].push_back(l_pUploader);
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
}

D3D12GpuThread D3D12Device::newThread()
{
	D3D12GpuThread l_ThreadRes(nullptr, nullptr);
	HRESULT l_Res = m_pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&l_ThreadRes.first));
	assert(S_OK == l_Res);
	l_Res = m_pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, l_ThreadRes.first, nullptr, IID_PPV_ARGS(&l_ThreadRes.second));
	assert(S_OK == l_Res);
	
	return l_ThreadRes;
}

#pragma endregion
}