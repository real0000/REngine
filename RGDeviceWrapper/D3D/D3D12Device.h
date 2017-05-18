// D3D12Device.h
//
// 2017/05/18 Ian Wu/Real0000
//

#ifndef _D3D12DEVICE_H_
#define _D3D12DEVICE_H_

namespace R
{

#define D3D12_NUM_COPY_THREAD 2
typedef std::pair<ID3D12CommandAllocator *, ID3D12GraphicsCommandList *> D3D12GpuThread;

class D3D12HeapManager
{
public:
	D3D12HeapManager(ID3D12Device *a_pDevice, unsigned int a_ExtendSize, D3D12_DESCRIPTOR_HEAP_TYPE a_Type, D3D12_DESCRIPTOR_HEAP_FLAGS a_Flag = D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
	~D3D12HeapManager();

	unsigned int newHeap();
	void recycle(unsigned int a_HeapID);
	D3D12_CPU_DESCRIPTOR_HANDLE getCpuHandle(unsigned int a_HeapID);
	D3D12_GPU_DESCRIPTOR_HANDLE getGpuHandle(unsigned int a_HeapID);

private:
	void extend();

	unsigned int m_CurrHeapSize, m_ExtendSize;
	ID3D12DescriptorHeap *m_pHeap;
	std::list<unsigned int> m_FreeSlot;
	ID3D12Device *m_pRefDevice;
	D3D12_DESCRIPTOR_HEAP_TYPE m_Type;
	D3D12_DESCRIPTOR_HEAP_FLAGS m_Flag;
	unsigned int m_HeapUnitSize;
};

class D3D12Commander : public GraphicCommander
{
public:
	D3D12Commander(D3D12HeapManager *a_pHeapOwner);
	virtual ~D3D12Commander();

	// canvas reference
	virtual void init(WXWidget a_Handle, glm::ivec2 a_Size, bool a_bFullScr);
	virtual void resize(glm::ivec2 a_Size, bool a_bFullScr);

	// draw command
	virtual void useProgram(ShaderProgram *a_pProgram);
	virtual void bindVertex(int a_ID);
	virtual void bindIndex(int a_ID);
	virtual void bindTexture(int a_TextureID, int a_Stage);
	virtual void bindVtxFlag(unsigned int a_VtxFlag);
	virtual void bindUniformBlock(int a_HeapID, int a_BlockStage);
	virtual void bindUavBlock(int a_HeapID, int a_BlockStage);
	virtual void clearRenderTarget(int a_RTVHandle, glm::vec4 a_Color);
	virtual void clearBackBuffer(int a_Idx, glm::vec4 a_Color);
	virtual void clearDepthTarget(int a_DSVHandle, bool a_bClearDepth, float a_Depth, bool a_bClearStencil, unsigned char a_Stencil);
	virtual void drawVertex(int a_NumVtx, int a_BaseVtx);
	virtual void drawElement(int a_BaseIdx, int a_NumIdx, int a_BaseVtx);
	virtual void drawIndirect(ShaderProgram *a_pProgram, unsigned int a_MaxCmd, void *a_pResPtr, void *a_pCounterPtr, unsigned int a_BufferOffset);
	
	bool isFullScreen(){ return m_bFullScreen; }

private:
	bool m_bFullScreen;
	unsigned int m_BackBuffer[NUM_BACKBUFFER];
	ID3D12Resource *m_pBackbufferRes[NUM_BACKBUFFER];
	IDXGISwapChain *m_pSwapChain;
	ID3D12CommandQueue *m_pDrawCmdQueue, *m_pBundleCmdQueue;
	std::vector<D3D12GpuThread> m_Thread;

	WXWidget m_Handle;
	HANDLE m_FenceEvent;
	uint64 m_SyncVal;
	ID3D12Fence *m_pSynchronizer;

	D3D12HeapManager *m_pRefHeapOwner;
};

class D3D12Device : public Dircet3DDevice
{
	friend class HLSLProgram;
public:
	D3D12Device();
	virtual ~D3D12Device();

	virtual void initDeviceMap();
	virtual void initDevice(unsigned int a_DeviceID);
	virtual void init();
	virtual void setupCanvas(wxWindow *a_pWnd, GraphicCanvas *a_pCanvas, glm::ivec2 a_Size, bool a_bFullScr);
	virtual void resetCanvas(wxWindow *a_pWnd, glm::ivec2 a_Size, bool a_bFullScr);
	virtual void destroyCanvas(wxWindow *a_pCanvas);
	virtual std::pair<int, int> maxShaderModel();

	// converter part( *::Key -> d3d, vulkan var )
	virtual unsigned int getBlendKey(BlendKey::Key a_Key);
	virtual unsigned int getBlendOP(BlendOP::Key a_Key);
	virtual unsigned int getBlendLogic(BlendLogic::Key a_Key);
	virtual unsigned int getCullMode(CullMode::Key a_Key);
	virtual unsigned int getComapreFunc(CompareFunc::Key a_Key);
	virtual unsigned int getStencilOP(StencilOP::Key a_Key);
	virtual unsigned int getTopologyType(TopologyType::Key a_Key);
	virtual unsigned int getTopology(Topology::Key a_Key);

	// 
	IDXGIFactory4* getDeviceFactory(){ return m_pGraphicInterface; }
	ID3D12Device* getDeviceInst(){ return m_pDevice; }

private:
	D3D12GpuThread newThread(D3D12_COMMAND_LIST_TYPE a_Type);

	D3D12HeapManager *m_pShaderResourceHeap, *m_pSamplerHeap, *m_pRenderTargetHeap, *m_pDepthHeap;
	std::map<wxWindow *, GraphicCanvas *> m_CanvasContainer;
	
	IDXGIFactory4 *m_pGraphicInterface;
	ID3D12Device *m_pDevice;
	ID3D12CommandQueue *m_pResCmdQueue, *m_pComputeQueue;
	D3D12GpuThread m_ResThread[D3D12_NUM_COPY_THREAD];
	std::deque<D3D12GpuThread> m_GraphicThread, m_ComputeThread;// idle thread
};

}
#endif