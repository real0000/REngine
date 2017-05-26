// D3D12Device.h
//
// 2017/05/18 Ian Wu/Real0000
//

#ifndef _D3D12DEVICE_H_
#define _D3D12DEVICE_H_

#include "D3DDevice.h"

namespace R
{
	
class D3D12HeapManager;
class D3D12Commander;
class D3D12Device;

#define D3D12_NUM_COPY_THREAD 2
typedef std::pair<ID3D12CommandAllocator *, ID3D12GraphicsCommandList *> D3D12GpuThread;

class D3D12HeapManager
{
public:
	D3D12HeapManager(ID3D12Device *a_pDevice, unsigned int a_ExtendSize, D3D12_DESCRIPTOR_HEAP_TYPE a_Type, D3D12_DESCRIPTOR_HEAP_FLAGS a_Flag = D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
	~D3D12HeapManager();

	unsigned int newHeap(ID3D12Resource *a_pResource, const D3D12_RENDER_TARGET_VIEW_DESC *a_pDesc);
	unsigned int newHeap(ID3D12Resource *a_pResource, const D3D12_SHADER_RESOURCE_VIEW_DESC *a_pDesc);
	unsigned int newHeap(ID3D12Resource *a_pResource, const D3D12_DEPTH_STENCIL_VIEW_DESC *a_pDesc);
	unsigned int newHeap(D3D12_CONSTANT_BUFFER_VIEW_DESC *a_pDesc);
	unsigned int newHeap(ID3D12Resource *a_pResource, ID3D12Resource *a_pCounterResource, const D3D12_UNORDERED_ACCESS_VIEW_DESC *a_pDesc);
	void recycle(unsigned int a_HeapID);
	D3D12_CPU_DESCRIPTOR_HANDLE getCpuHandle(unsigned int a_HeapID, ID3D12DescriptorHeap *a_pTargetHeap = nullptr);
	D3D12_GPU_DESCRIPTOR_HANDLE getGpuHandle(unsigned int a_HeapID, ID3D12DescriptorHeap *a_pTargetHeap = nullptr);
	ID3D12DescriptorHeap* getHeapInst(){ return m_pHeap; }

private:
	unsigned int newHeap();
	void extend();

	unsigned int m_CurrHeapSize, m_ExtendSize;
	ID3D12DescriptorHeap *m_pHeap, *m_pHeapCache;
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
	virtual void bindVertex(VertexBuffer *a_pBuffer);
	virtual void bindIndex(IndexBuffer *a_pBuffer);
	virtual void bindTexture(int a_TextureID, unsigned int a_Stage);
	virtual void bindVtxFlag(unsigned int a_VtxFlag);
	virtual void bindUniformBlock(int a_HeapID, int a_BlockStage);
	virtual void bindUavBlock(int a_HeapID, int a_BlockStage);
	virtual void clearRenderTarget(int a_RTVHandle, glm::vec4 a_Color);
	virtual void clearBackBuffer(int a_Idx, glm::vec4 a_Color);
	virtual void clearDepthTarget(int a_DSVHandle, bool a_bClearDepth, float a_Depth, bool a_bClearStencil, unsigned char a_Stencil);
	virtual void drawVertex(int a_NumVtx, int a_BaseVtx);
	virtual void drawElement(int a_BaseIdx, int a_NumIdx, int a_BaseVtx);
	virtual void drawIndirect(ShaderProgram *a_pProgram, unsigned int a_MaxCmd, void *a_pResPtr, void *a_pCounterPtr, unsigned int a_BufferOffset);
	
	virtual void setTopology(Topology::Key a_Key);
	virtual void setRenderTarget(int a_DSVHandle, std::vector<int> &a_RTVHandle);
	virtual void setRenderTargetWithBackBuffer(int a_DSVHandle, unsigned int a_BackIdx);
	virtual void setViewPort(int a_NumViewport, ...);// glm::Viewport
	virtual void setScissor(int a_NumScissor, ...);// glm::ivec4

	virtual void flush();
	virtual void present();

	bool isFullScreen(){ return m_bFullScreen; }

private:
	D3D12GpuThread getThisThread();
	void threadEnd();

	bool m_bFullScreen;
	unsigned int m_BackBuffer[NUM_BACKBUFFER];
	ID3D12Resource *m_pBackbufferRes[NUM_BACKBUFFER];
	IDXGISwapChain *m_pSwapChain;
	ID3D12CommandQueue *m_pDrawCmdQueue, *m_pBundleCmdQueue;// to do : add bundle support

	WXWidget m_Handle;
	HANDLE m_FenceEvent;
	uint64 m_SyncVal;
	ID3D12Fence *m_pSynchronizer;

	D3D12Device *m_pRefDevice;
	D3D12HeapManager *m_pRefHeapOwner;
	std::vector<D3D12GpuThread> m_ReadyThread, m_BusyThread;
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
	virtual unsigned int convert(BlendKey::Key a_Key);
	virtual unsigned int convert(BlendOP::Key a_Key);
	virtual unsigned int convert(BlendLogic::Key a_Key);
	virtual unsigned int convert(CullMode::Key a_Key);
	virtual unsigned int convert(CompareFunc::Key a_Key);
	virtual unsigned int convert(StencilOP::Key a_Key);
	virtual unsigned int convert(TopologyType::Key a_Key);
	virtual unsigned int convert(Topology::Key a_Key);

	// texture part
	virtual int allocateTexture(unsigned int a_Size, PixelFormat::Key a_Format);
	virtual int allocateTexture(glm::ivec2 a_Size, PixelFormat::Key a_Format, unsigned int a_ArraySize = 1);
	virtual int allocateTexture(glm::ivec3 a_Size, PixelFormat::Key a_Format);
	virtual void updateTexture(int a_ID, unsigned int a_MipmapLevel, unsigned int a_Size, unsigned int a_Offset, void *a_pSrcData);
	virtual void updateTexture(int a_ID, unsigned int a_MipmapLevel, glm::ivec2 a_Size, glm::ivec2 a_Offset, unsigned int a_Idx, void *a_pSrcData);
	virtual void updateTexture(int a_ID, unsigned int a_MipmapLevel, glm::ivec3 a_Size, glm::ivec3 a_Offset, void *a_pSrcData);
	virtual void generateMipmap(int a_ID);
	virtual int getTextureHeapID(int a_ID);
	virtual PixelFormat::Key getTextureFormat(int a_ID);
	virtual glm::ivec3 getTextureSize(int a_ID);
	virtual void* getTextureResource(int a_ID);
	virtual void freeTexture(int a_ID);
	
	// render target part
	virtual int createRenderTarget(glm::ivec3 a_Size, PixelFormat::Key a_Format);// 3d render target, use uav
	virtual int createRenderTarget(glm::ivec2 a_Size, PixelFormat::Key a_Format, unsigned int a_ArraySize = 1);// texture 2d array
	virtual void freeRenderTarget(int a_ID);

	// vertex, index buffer part
	virtual int requestVertexBuffer(void *a_pInitData, unsigned int a_Slot, unsigned int a_Count, wxString a_Name = wxT(""));
	virtual void updateVertexBuffer(int a_ID, void *a_pData, unsigned int a_SizeInByte);
	virtual void* getVertexResource(int a_ID);
	virtual void freeVertexBuffer(int a_ID);
	virtual int requestIndexBuffer(void *a_pInitData, PixelFormat::Key a_Fmt, unsigned int a_Count, wxString a_Name = wxT(""));
	virtual void updateIndexBuffer(int a_ID, void *a_pData, unsigned int a_SizeInByte);
	virtual void* getIndexResource(int a_ID);
	virtual void freeIndexBuffer(int a_ID);

	// cbv part
	virtual int requestConstBuffer(char* &a_pOutputBuff, unsigned int a_Size);//return buffer id
	virtual char* getConstBufferContainer(int a_ID);
	virtual void* getConstBufferResource(int a_ID);
	virtual void freeConstBuffer(int a_ID);

	// uav part
	virtual unsigned int requestUavBuffer(char* &a_pOutputBuff, unsigned int a_UnitSize, unsigned int a_ElementCount) ;
	virtual void resizeUavBuffer(int a_ID, char* &a_pOutputBuff, unsigned int a_ElementCount);
	virtual char* getUavBufferContainer(int a_ID);
	virtual void* getUavBufferResource(int a_ID);
	virtual void syncUavBuffer(bool a_bToGpu, std::vector<unsigned int> &a_BuffIDList);
	virtual void syncUavBuffer(bool a_bToGpu, std::vector< std::tuple<unsigned int, unsigned int, unsigned int> > &a_BuffIDList);
	virtual void freeUavBuffer(int a_ID);
	
	// 
	IDXGIFactory4* getDeviceFactory(){ return m_pGraphicInterface; }
	ID3D12Device* getDeviceInst(){ return m_pDevice; }
	D3D12_VERTEX_BUFFER_VIEW getVertexBufferView(int a_ID);
	D3D12_INDEX_BUFFER_VIEW getIndexBufferView(int a_ID);
	ID3D12DescriptorHeap* getShaderBindingHeap(){ return m_pShaderResourceHeap->getHeapInst(); }

	// thread part
	D3D12GpuThread requestThread();
	void recycleThread(D3D12GpuThread a_Thread);
	void waitForResourceUpdate();

private:
	void resourceThread();
	ID3D12Resource* initSizedResource(unsigned int a_Size, D3D12_HEAP_TYPE a_HeapType, D3D12_RESOURCE_STATES a_InitState = D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_FLAGS a_Flag = D3D12_RESOURCE_FLAG_NONE);
	void updateResourceData(ID3D12Resource *a_pRes, void *a_pSrcData, unsigned int a_SizeInByte);

	struct TextureBinder
	{
		TextureBinder()
			: m_pTexture(nullptr)
			, m_Size(8.0f, 8.0f, 1.0f)
			, m_Format(PixelFormat::rgba8_unorm)
			, m_HeapID(0)
			, m_Type(TEXTYPE_SIMPLE_2D)
			, m_MipmapLevels(1){}
		~TextureBinder(){ SAFE_RELEASE(m_pTexture) }

		ID3D12Resource *m_pTexture;
		glm::ivec3 m_Size;
		PixelFormat::Key m_Format;
		unsigned int m_HeapID;
		TextureType m_Type;
		unsigned int m_MipmapLevels;
	};
	struct RenderTargetBinder
	{
		RenderTargetBinder() : m_TextureID(-1), m_pRefBinder(nullptr), m_HeapID(0){}
		~RenderTargetBinder(){}

		int m_TextureID;
		TextureBinder *m_pRefBinder;
		unsigned int m_HeapID;
	};
	int allocateTexture(glm::ivec3 a_Size, PixelFormat::Key a_Format, D3D12_RESOURCE_DIMENSION a_Dim, unsigned int a_MipmapLevel, unsigned int a_Flag);
	void updateTexture(int a_ID, unsigned int a_MipmapLevel, glm::ivec3 a_Size, glm::ivec3 a_Offset, void *a_pSrcData, unsigned char a_FillColor);

	struct VertexBinder
	{
		VertexBinder() : m_pVtxRes(nullptr){}
		~VertexBinder(){ SAFE_RELEASE(m_pVtxRes) }

		ID3D12Resource *m_pVtxRes;
		D3D12_VERTEX_BUFFER_VIEW m_VtxView;
	};
	struct IndexBinder
	{
		IndexBinder() : m_pIndexRes(nullptr){}
		~IndexBinder(){ SAFE_RELEASE(m_pIndexRes) }

		ID3D12Resource *m_pIndexRes;
		D3D12_INDEX_BUFFER_VIEW m_IndexView;
	};
	struct ConstBufferBinder
	{
		ConstBufferBinder() : m_pResource(nullptr), m_HeapID(0), m_CurrSize(0), m_pCurrBuff(nullptr){}
		~ConstBufferBinder(){ SAFE_RELEASE(m_pResource) }

		ID3D12Resource *m_pResource;
		unsigned int m_HeapID;
		unsigned int m_CurrSize;
		char *m_pCurrBuff;
	};
	struct UnorderAccessBufferBinder : ConstBufferBinder
	{
		UnorderAccessBufferBinder() : ConstBufferBinder(), m_ElementSize(0), m_ElementCount(0){}

		unsigned int m_ElementSize;
		unsigned int m_ElementCount;
	};
	struct ReadBackBuffer
	{
		ReadBackBuffer() : m_pTempResource(nullptr), m_pTargetBuffer(nullptr), m_Size(0){}
		void readback()
		{
			D3D12_RANGE l_MapRange;
			l_MapRange.Begin = 0;
			l_MapRange.End = m_Size;

			unsigned char* l_pDataBegin = nullptr;
			m_pTempResource->Map(0, &l_MapRange, reinterpret_cast<void**>(&l_pDataBegin));
			memcpy(m_pTargetBuffer, l_pDataBegin, m_Size);
			m_pTempResource->Unmap(0, nullptr);
		}

		ID3D12Resource *m_pTempResource;
		unsigned char *m_pTargetBuffer;
		unsigned int m_Size;
	};

	D3D12GpuThread newThread(D3D12_COMMAND_LIST_TYPE a_Type);

	D3D12HeapManager *m_pShaderResourceHeap, *m_pSamplerHeap, *m_pRenderTargetHeap, *m_pDepthHeap;
	std::map<wxWindow *, GraphicCanvas *> m_CanvasContainer;
	
	SerializedObjectPool<TextureBinder> m_ManagedTexture;
	SerializedObjectPool<RenderTargetBinder> m_ManagedRenderTarget;
	SerializedObjectPool<VertexBinder> m_ManagedVertexBuffer;
	SerializedObjectPool<IndexBinder> m_ManagedIndexBuffer;
	SerializedObjectPool<ConstBufferBinder> m_ManagedConstBuffer;
	SerializedObjectPool<UnorderAccessBufferBinder> m_ManagedUavBuffer;

	IDXGIFactory4 *m_pGraphicInterface;
	ID3D12Device *m_pDevice;
	DXGI_SAMPLE_DESC m_MsaaSetting;
	ID3D12CommandQueue *m_pResCmdQueue, *m_pComputeQueue;
	D3D12GpuThread m_ResThread[D3D12_NUM_COPY_THREAD];
	unsigned int m_IdleResThread;
	HANDLE m_FenceEvent;
	uint64 m_SyncVal;
	ID3D12Fence *m_pSynchronizer;
	std::vector<ID3D12Resource *> m_TempResources[D3D12_NUM_COPY_THREAD];
	std::vector<ReadBackBuffer> m_ReadBackContainer[D3D12_NUM_COPY_THREAD];
	std::thread *m_pResourceLoop;
	bool m_bResLoop;

	std::deque<D3D12GpuThread> m_GraphicThread, m_ComputeThread;// idle thread
	std::mutex m_ThreadMutex, m_ResourceMutex;
};

}
#endif