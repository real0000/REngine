// D3D12Device.cpp
//
// 2017/05/18 Ian Wu/Real0000
//

#include "RGDeviceWrapper.h"
#include <thread>

namespace R
{

#pragma region D3D12HeapManager
//
// HeapManager
//
D3D12HeapManager::D3D12HeapManager(ID3D12Device *a_pDevice, unsigned int a_ExtendSize, D3D12_DESCRIPTOR_HEAP_TYPE a_Type, D3D12_DESCRIPTOR_HEAP_FLAGS a_Flag)
	: m_CurrHeapSize(0), m_ExtendSize(a_ExtendSize)
	, m_pHeap(nullptr)
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
}

unsigned int D3D12HeapManager::newHeap()
{
	if( m_FreeSlot.empty() ) extend();
	unsigned int l_Res = m_FreeSlot.front();
	m_FreeSlot.pop_front();
	return l_Res;
}

void D3D12HeapManager::recycle(unsigned int a_HeapID)
{
	assert(m_FreeSlot.end() == std::find(m_FreeSlot.begin(), m_FreeSlot.end(), a_HeapID));
	m_FreeSlot.push_back(a_HeapID);
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12HeapManager::getCpuHandle(unsigned int a_HeapID)
{
	D3D12_CPU_DESCRIPTOR_HANDLE l_Res(m_pHeap->GetCPUDescriptorHandleForHeapStart());
	l_Res.ptr += a_HeapID * m_HeapUnitSize;
	return l_Res;
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12HeapManager::getGpuHandle(unsigned int a_HeapID)
{
	D3D12_GPU_DESCRIPTOR_HANDLE l_Res(m_pHeap->GetGPUDescriptorHandleForHeapStart());
	l_Res.ptr += a_HeapID * m_HeapUnitSize;
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

	if( nullptr != m_pHeap ) m_pRefDevice->CopyDescriptorsSimple(m_CurrHeapSize, l_pNewHeap->GetCPUDescriptorHandleForHeapStart(), m_pHeap->GetCPUDescriptorHandleForHeapStart(), m_Type);
	m_pHeap = l_pNewHeap;

	for( unsigned int i=0 ; i<m_ExtendSize ; ++i ) m_FreeSlot.push_back(m_CurrHeapSize + i);
	m_CurrHeapSize += m_ExtendSize;
}
#pragma endregion

#pragma region D3D12Commander
//
// D3D12Commander
//
D3D12Commander::D3D12Commander(D3D12HeapManager *a_pHeapOwner)
	: m_bFullScreen(false)
	, m_pSwapChain(nullptr)
	, m_pDrawCmdQueue(nullptr), m_pBundleCmdQueue(nullptr)
	, m_Handle(0)
	, m_FenceEvent(0), m_SyncVal(0), m_pSynchronizer(nullptr)
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
	D3D12Device *l_pDevClass = TYPED_GDEVICE(D3D12Device);
	ID3D12Device *l_pDevInst = l_pDevClass->getDeviceInst();
	IDXGIFactory4 *l_pDevFactory = l_pDevClass->getDeviceFactory();

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
		wxMessageBox(wxT("swap chain create failed"), wxT("D3D12Device::CanvasItem::init"));
		return;
	}

	for( unsigned int i=0 ; i<NUM_BACKBUFFER ; ++i )
	{
		m_BackBuffer[i] = m_pRefHeapOwner->newHeap();
		if( S_OK != m_pSwapChain->GetBuffer(i, IID_PPV_ARGS(&m_pBackbufferRes[i])) )
		{
			wxMessageBox(wxString::Format(wxT("can't get rtv[%d]"), i), wxT("D3D12Device::CanvasItem::init"));
			return;
		}

		l_pDevInst->CreateRenderTargetView(m_pBackbufferRes[i], nullptr, m_pRefHeapOwner->getCpuHandle(m_BackBuffer[i]));
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

/*void D3D12Commander::waitGpuSync(ID3D12CommandQueue *a_pQueue)
{
	a_pQueue->Signal(m_pSynchronizer, m_SyncVal);
	m_pSynchronizer->SetEventOnCompletion(m_SyncVal, m_FenceEvent);
	WaitForSingleObject(m_FenceEvent, INFINITE);
	++m_SyncVal;
}

void D3D12Commander::waitFrameSync(ID3D12CommandQueue *a_pQueue)
{
	uint64 l_SyncVal = m_SyncVal;
	if( S_OK != a_pQueue->Signal(m_pSynchronizer, l_SyncVal) )
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
}*/
#pragma endregion

#pragma region D3D12Device
//
// D3D12Device
//
D3D12Device::D3D12Device()
	: m_pShaderResourceHeap(nullptr), m_pSamplerHeap(nullptr), m_pRenderTargetHeap(nullptr), m_pDepthHeap(nullptr)
	, m_pGraphicInterface(nullptr)
	, m_pDevice(nullptr)
	, m_pResCmdQueue(nullptr), m_pComputeQueue(nullptr)
{
	for( unsigned int i=0 ; i<D3D12_NUM_COPY_THREAD ; ++i ) m_ResThread[i] = std::make_pair(nullptr, nullptr);
}

D3D12Device::~D3D12Device()
{
	SAFE_DELETE(m_pShaderResourceHeap);
	SAFE_DELETE(m_pSamplerHeap);
	SAFE_DELETE(m_pRenderTargetHeap);
	SAFE_DELETE(m_pDepthHeap);

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
	unsigned int l_NumCore = std::thread::hardware_concurrency();
	if( 0 == l_NumCore ) l_NumCore = DEFAULT_D3D_THREAD_COUNT;
	l_NumCore *= 2;
	for( unsigned int i=0 ; i<l_NumCore ; ++i )
	{
		m_GraphicThread.push_back(newThread(D3D12_COMMAND_LIST_TYPE_DIRECT));
		m_ComputeThread.push_back(newThread(D3D12_COMMAND_LIST_TYPE_COMPUTE));
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
/*
void D3D12Device::waitGpuSync(wxWindow *a_pCanvas)
{
	auto it = m_CanvasContainer.find(a_pCanvas);
	if( m_CanvasContainer.end() == it ) return;

	it->second->waitGpuSync(m_pCmdQueue);
}

void D3D12Device::waitFrameSync(wxWindow *a_pCanvas)
{
	auto it = m_CanvasContainer.find(a_pCanvas);
	if( m_CanvasContainer.end() == it ) return;
	
	it->second->waitFrameSync(m_pCmdQueue);
}*/

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