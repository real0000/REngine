// D3D12Device.h
//
// 2017/05/18 Ian Wu/Real0000
//

#ifndef _D3D12DEVICE_H_
#define _D3D12DEVICE_H_

#include <unordered_map>
#include "D3DDevice.h"

namespace R
{
	
class D3D12HeapManager;
class D3D12Commander;
class D3D12Device;
class HLSLProgram12;

#define D3D12_NUM_COPY_THREAD 2
typedef std::pair<ID3D12CommandAllocator *, ID3D12GraphicsCommandList *> D3D12GpuThread;

class D3D12HeapManager
{
public:
	D3D12HeapManager(ID3D12Device *a_pDevice, unsigned int a_ExtendSize, D3D12_DESCRIPTOR_HEAP_TYPE a_Type, D3D12_DESCRIPTOR_HEAP_FLAGS a_Flag = D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
	virtual ~D3D12HeapManager();

	unsigned int newHeap(ID3D12Resource *a_pResource, const D3D12_RENDER_TARGET_VIEW_DESC *a_pDesc);
	unsigned int newHeap(ID3D12Resource *a_pResource, const D3D12_SHADER_RESOURCE_VIEW_DESC *a_pDesc);
	unsigned int newHeap(ID3D12Resource *a_pResource, const D3D12_DEPTH_STENCIL_VIEW_DESC *a_pDesc);
	unsigned int newHeap(D3D12_SAMPLER_DESC *a_pDesc);
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

class D3D12Fence
{
public:
	D3D12Fence(ID3D12Device *a_pDevice, ID3D12CommandQueue *a_pQueue);
	virtual ~D3D12Fence();

	void signal();
	void wait();

private:
	ID3D12CommandQueue *m_pRefQueue;
	HANDLE m_FenceEvent;
	uint64 m_SyncVal;
	ID3D12Fence *m_pSynchronizer;
};

class D3D12Commander : public GraphicCommander
{
public:
	// for SetComputeRoot* and SetGraphicRoot*
	struct CmdComponent
	{
		virtual void setRoot32BitConstant(ID3D12GraphicsCommandList* a_pTarget, unsigned int a_RootParameterIndex, unsigned int a_SrcData, unsigned int a_DestOffsetIn32BitValues) = 0;
		virtual void setRoot32BitConstants(ID3D12GraphicsCommandList* a_pTarget, unsigned int a_RootParameterIndex, unsigned int a_Num32BitValuesToSet, const void *a_pSrcData, unsigned int a_DestOffsetIn32BitValues) = 0;
		virtual void setRootConstantBufferView(ID3D12GraphicsCommandList* a_pTarget, unsigned int a_RootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS a_BufferLocation) = 0;
		virtual void setRootDescriptorTable(ID3D12GraphicsCommandList* a_pTarget, unsigned int a_RootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE a_BaseDescriptor) = 0;
		virtual void setRootShaderResourceView(ID3D12GraphicsCommandList* a_pTarget, unsigned int a_RootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS a_BufferLocation) = 0;
		virtual void setRootSignature(ID3D12GraphicsCommandList* a_pTarget, ID3D12RootSignature *a_pSignature) = 0;
		virtual void setRootUnorderedAccessView(ID3D12GraphicsCommandList* a_pTarget, unsigned int a_RootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS a_BufferLocation) = 0;
	};
	struct ComputeCmdComponent : public CmdComponent
	{
		virtual void setRoot32BitConstant(ID3D12GraphicsCommandList* a_pTarget, unsigned int a_RootParameterIndex, unsigned int a_SrcData, unsigned int a_DestOffsetIn32BitValues)										{ a_pTarget->SetComputeRoot32BitConstant(a_RootParameterIndex, a_SrcData, a_DestOffsetIn32BitValues); }
		virtual void setRoot32BitConstants(ID3D12GraphicsCommandList* a_pTarget, unsigned int a_RootParameterIndex, unsigned int a_Num32BitValuesToSet, const void *a_pSrcData, unsigned int a_DestOffsetIn32BitValues)	{ a_pTarget->SetComputeRoot32BitConstants(a_RootParameterIndex, a_Num32BitValuesToSet, a_pSrcData, a_DestOffsetIn32BitValues); }
		virtual void setRootConstantBufferView(ID3D12GraphicsCommandList* a_pTarget, unsigned int a_RootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS a_BufferLocation)														{ a_pTarget->SetComputeRootConstantBufferView(a_RootParameterIndex, a_BufferLocation); }
		virtual void setRootDescriptorTable(ID3D12GraphicsCommandList* a_pTarget, unsigned int a_RootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE a_BaseDescriptor)														{ a_pTarget->SetComputeRootDescriptorTable(a_RootParameterIndex, a_BaseDescriptor); }
		virtual void setRootShaderResourceView(ID3D12GraphicsCommandList* a_pTarget, unsigned int a_RootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS a_BufferLocation)														{ a_pTarget->SetComputeRootShaderResourceView(a_RootParameterIndex, a_BufferLocation); }
		virtual void setRootSignature(ID3D12GraphicsCommandList* a_pTarget, ID3D12RootSignature *a_pSignature)																											{ a_pTarget->SetComputeRootSignature(a_pSignature); }
		virtual void setRootUnorderedAccessView(ID3D12GraphicsCommandList* a_pTarget, unsigned int a_RootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS a_BufferLocation)													{ a_pTarget->SetComputeRootUnorderedAccessView(a_RootParameterIndex, a_BufferLocation); }
	};
	struct GraphicCmdComponent : public CmdComponent
	{
		virtual void setRoot32BitConstant(ID3D12GraphicsCommandList* a_pTarget, unsigned int a_RootParameterIndex, unsigned int a_SrcData, unsigned int a_DestOffsetIn32BitValues)										{ a_pTarget->SetGraphicsRoot32BitConstant(a_RootParameterIndex, a_SrcData, a_DestOffsetIn32BitValues); }
		virtual void setRoot32BitConstants(ID3D12GraphicsCommandList* a_pTarget, unsigned int a_RootParameterIndex, unsigned int a_Num32BitValuesToSet, const void *a_pSrcData, unsigned int a_DestOffsetIn32BitValues)	{ a_pTarget->SetGraphicsRoot32BitConstants(a_RootParameterIndex, a_Num32BitValuesToSet, a_pSrcData, a_DestOffsetIn32BitValues); }
		virtual void setRootConstantBufferView(ID3D12GraphicsCommandList* a_pTarget, unsigned int a_RootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS a_BufferLocation)														{ a_pTarget->SetGraphicsRootConstantBufferView(a_RootParameterIndex, a_BufferLocation); }
		virtual void setRootDescriptorTable(ID3D12GraphicsCommandList* a_pTarget, unsigned int a_RootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE a_BaseDescriptor)														{ a_pTarget->SetGraphicsRootDescriptorTable(a_RootParameterIndex, a_BaseDescriptor); }
		virtual void setRootShaderResourceView(ID3D12GraphicsCommandList* a_pTarget, unsigned int a_RootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS a_BufferLocation)														{ a_pTarget->SetGraphicsRootShaderResourceView(a_RootParameterIndex, a_BufferLocation); }
		virtual void setRootSignature(ID3D12GraphicsCommandList* a_pTarget, ID3D12RootSignature *a_pSignature)																											{ a_pTarget->SetGraphicsRootSignature(a_pSignature); }
		virtual void setRootUnorderedAccessView(ID3D12GraphicsCommandList* a_pTarget, unsigned int a_RootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS a_BufferLocation)													{ a_pTarget->SetGraphicsRootUnorderedAccessView(a_RootParameterIndex, a_BufferLocation); }
	};

public:
	D3D12Commander(D3D12HeapManager *a_pHeapOwner);
	virtual ~D3D12Commander();

	// command init/sending
	virtual void begin(bool a_bConmpute);
	virtual void end();

	// draw command
	virtual void useProgram(std::shared_ptr<ShaderProgram> a_pProgram);
	virtual void bindVertex(VertexBuffer *a_pBuffer, int a_InstanceBuffer = -1);
	virtual void bindIndex(IndexBuffer *a_pBuffer);
	virtual void bindTexture(int a_ID, unsigned int a_Stage, bool a_bRenderTarget);
	virtual void bindSampler(int a_ID, unsigned int a_Stage);
	virtual void bindConstant(std::string a_Name, unsigned int a_SrcData);
	virtual void bindConstant(std::string a_Name, void* a_pSrcData, unsigned int a_SizeInUInt);
	virtual void bindConstBlock(int a_HeapID, int a_BlockStage);
	virtual void bindUavBlock(int a_HeapID, int a_BlockStage);
	virtual void clearRenderTarget(int a_ID, glm::vec4 a_Color);
	virtual void clearBackBuffer(GraphicCanvas *a_pCanvas, glm::vec4 a_Color);
	virtual void clearDepthTarget(int a_ID, bool a_bClearDepth, float a_Depth, bool a_bClearStencil, unsigned char a_Stencil);
	virtual void drawVertex(int a_NumVtx, int a_BaseVtx);
	virtual void drawElement(int a_BaseIdx, int a_NumIdx, int a_BaseVtx, unsigned int a_NumInstance, unsigned int a_BaseInstance);
	virtual void drawIndirect(unsigned int a_MaxCmd, void *a_pResPtr, void *a_pCounterPtr, unsigned int a_BufferOffset);
	virtual void drawIndirect(unsigned int a_MaxCmd, int a_BuffID);
	virtual void compute(unsigned int a_CountX, unsigned int a_CountY = 1, unsigned int a_CountZ = 1);
	
	virtual void setTopology(Topology::Key a_Key);
	virtual void setRenderTarget(int a_DepthID, std::vector<int> &a_RederTargetIDList);
	virtual void setRenderTargetWithBackBuffer(int a_DSVHandle, GraphicCanvas *a_pCanvas);
	virtual void setViewPort(int a_NumViewport, ...);// glm::Viewport
	virtual void setScissor(int a_NumScissor, ...);// glm::ivec4

private:
	D3D12Device *m_pRefDevice;
	D3D12HeapManager *m_pRefHeapOwner;
	
	// command state
	bool m_bCompute;
	CmdComponent *m_pComponent;
	D3D12GpuThread m_CurrThread;
	HLSLProgram12 *m_pCurrProgram;

	ID3D12Resource *m_pRefBackBuffer;
	bool m_bNeedResetBackBufferState;

	// resource state
	std::unordered_map<ID3D12Resource*, std::pair<int, int> > m_ResourceState;//resource, origin state, current state

	static ComputeCmdComponent m_ComputeComponent;
	static GraphicCmdComponent m_GraphicComponent;
};

class D3D12Canvas : public GraphicCanvas
{
public:
	D3D12Canvas(D3D12HeapManager *a_pHeapOwner, wxWindow *a_pParent, wxWindowID a_ID);
	virtual ~D3D12Canvas();

	virtual void init(bool a_bFullScr);
	virtual unsigned int getBackBuffer();
	virtual void* getBackBufferResource();

	virtual void present();
	virtual void resizeBackBuffer();

	void presentImp();

private:
	D3D12HeapManager *m_pRefHeapOwner;
	unsigned int m_BackBuffer[NUM_BACKBUFFER];
	ID3D12Resource *m_pBackbufferRes[NUM_BACKBUFFER];
	IDXGISwapChain3 *m_pSwapChain;
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
	virtual void shutdown();
	virtual GraphicCommander* commanderFactory();
	virtual GraphicCanvas* canvasFactory(wxWindow *a_pParent, wxWindowID a_ID);
	virtual std::pair<int, int> maxShaderModel();
	virtual void wait();
	
	// support
	virtual bool supportExtraIndirectCommand(){ return true; }

	// converter part( *::Key -> d3d, vulkan var )
	virtual unsigned int getBlendKey(BlendKey::Key a_Key);
	virtual unsigned int getBlendOP(BlendOP::Key a_Key);
	virtual unsigned int getBlendLogic(BlendLogic::Key a_Key);
	virtual unsigned int getCullMode(CullMode::Key a_Key);
	virtual unsigned int getComapreFunc(CompareFunc::Key a_Key);
	virtual unsigned int getStencilOP(StencilOP::Key a_Key);
	virtual unsigned int getTopologyType(TopologyType::Key a_Key);
	virtual unsigned int getTopology(Topology::Key a_Key);
	virtual unsigned int getFilter(Filter::Key a_Key);
	virtual unsigned int getAddressMode(AddressMode::Key a_Key);

	// texture part
	virtual int allocateTexture(glm::ivec2 a_Size, PixelFormat::Key a_Format, unsigned int a_ArraySize = 1, bool a_bCube = false);
	virtual int allocateTexture(glm::ivec3 a_Size, PixelFormat::Key a_Format);
	virtual void updateTexture(int a_ID, unsigned int a_MipmapLevel, glm::ivec2 a_Size, glm::ivec2 a_Offset, unsigned int a_Idx, void *a_pSrcData);
	virtual void updateTexture(int a_ID, unsigned int a_MipmapLevel, glm::ivec3 a_Size, glm::ivec3 a_Offset, void *a_pSrcData);
	virtual void generateMipmap(int a_ID, unsigned int a_Level, std::shared_ptr<ShaderProgram> a_pProgram);
	virtual void copyTexture(int a_Dst, int a_Src);
	virtual PixelFormat::Key getTextureFormat(int a_ID);
	virtual glm::ivec3 getTextureSize(int a_ID);
	virtual TextureType getTextureType(int a_ID);
	virtual void* getTextureResource(int a_ID);
	virtual void freeTexture(int a_ID);
	virtual int createSampler(Filter::Key a_Filter, AddressMode::Key a_UMode, AddressMode::Key a_VMode, AddressMode::Key a_WMode, float a_MipLodBias, unsigned int a_MaxAnisotropy, CompareFunc::Key a_Func, float a_MinLod = 0.0f, float a_MaxLod = std::numeric_limits<float>::max(), float a_Border0 = 0.0f, float a_Border1 = 0.0f, float a_Border2 = 0.0f, float a_Border3 = 0.0f);
	virtual void freeSampler(int a_ID);
	
	// render target part
	virtual int createRenderTarget(glm::ivec3 a_Size, PixelFormat::Key a_Format);
	virtual int createRenderTarget(glm::ivec2 a_Size, PixelFormat::Key a_Format, unsigned int a_ArraySize = 1, bool a_bCube = false);// texture 2d array
	virtual int getRenderTargetTexture(int a_ID);
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

	// misc
	virtual int requestIndrectCommandBuffer(char* &a_pOutputBuff, unsigned int a_Size);
	virtual void freeIndrectCommandBuffer(int a_BuffID);
	
	// 
	IDXGIFactory4* getDeviceFactory(){ return m_pGraphicInterface; }
	ID3D12Device* getDeviceInst(){ return m_pDevice; }
	D3D12_GPU_DESCRIPTOR_HANDLE getTextureGpuHandle(int a_ID, bool a_bRenderTarget);
	D3D12_GPU_DESCRIPTOR_HANDLE getSamplerGpuHandle(int a_ID);
	D3D12_GPU_VIRTUAL_ADDRESS getTextureGpuAddress(int a_ID, bool a_bRenderTarget);
	D3D12_CPU_DESCRIPTOR_HANDLE getRenderTargetCpuHandle(int a_ID, bool a_bDepth = false);
	ID3D12Resource* getRenderTargetResource(int a_ID);
	D3D12_GPU_DESCRIPTOR_HANDLE getConstBufferGpuHandle(int a_ID);
	D3D12_GPU_DESCRIPTOR_HANDLE getUnorderAccessBufferGpuHandle(int a_ID);
	D3D12_GPU_VIRTUAL_ADDRESS getConstBufferGpuAddress(int a_ID);
	D3D12_GPU_VIRTUAL_ADDRESS getUnorderAccessBufferGpuAddress(int a_ID);
	D3D12_VERTEX_BUFFER_VIEW& getVertexBufferView(int a_ID);
	D3D12_INDEX_BUFFER_VIEW& getIndexBufferView(int a_ID);
	ID3D12Resource* getIndirectCommandBuffer(int a_ID);
	ID3D12DescriptorHeap* getShaderBindingHeap(){ return m_pShaderResourceHeap->getHeapInst(); }
	ID3D12DescriptorHeap* getSamplerBindingHeap(){ return m_pSamplerHeap->getHeapInst(); }
	ID3D12CommandQueue* getDrawCommandQueue(){ return m_pDrawCmdQueue; }// for swap chain create

	// thread part
	D3D12GpuThread requestThread();
	void recycleThread(D3D12GpuThread a_Thread);
	void addPresentCanvas(D3D12Canvas *a_pCanvas);

private:
	void resourceThread();
	void graphicThread();
	ID3D12Resource* initSizedResource(unsigned int a_Size, D3D12_HEAP_TYPE a_HeapType, D3D12_RESOURCE_STATES a_InitState = D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_FLAGS a_Flag = D3D12_RESOURCE_FLAG_NONE);
	void updateResourceData(ID3D12Resource *a_pRes, void *a_pSrcData, unsigned int a_SizeInByte);

	struct TextureBinder
	{
		TextureBinder()
			: m_pTexture(nullptr)
			, m_Size(8.0f, 8.0f, 1.0f)
			, m_Format(PixelFormat::rgba8_unorm)
			, m_HeapID(0)
			, m_UavHeapID(0)
			, m_Type(TEXTYPE_SIMPLE_2D)
			, m_MipmapLevels(1)
			, m_bWriteable(false){}
		~TextureBinder(){ SAFE_RELEASE(m_pTexture) }

		ID3D12Resource *m_pTexture;
		glm::ivec3 m_Size;
		PixelFormat::Key m_Format;
		unsigned int m_HeapID;
		unsigned int m_UavHeapID;
		TextureType m_Type;
		unsigned int m_MipmapLevels;
		bool m_bWriteable;
	};
	struct SamplerBinder
	{
		SamplerBinder()
			: m_HeapID(0){}
		
		unsigned int m_HeapID;
	};
	struct RenderTargetBinder
	{
		RenderTargetBinder() : m_TextureID(-1), m_pRefBinder(nullptr), m_HeapID(0){}
		~RenderTargetBinder(){}

		int m_TextureID;
		std::shared_ptr<TextureBinder> m_pRefBinder;
		unsigned int m_HeapID;
	};
	int allocateTexture(glm::ivec3 a_Size, PixelFormat::Key a_Format, D3D12_RESOURCE_DIMENSION a_Dim, unsigned int a_MipmapLevel, unsigned int a_Flag, bool a_bCube = false);
	void updateTexture(int a_ID, unsigned int a_MipmapLevel, glm::ivec3 a_Size, glm::ivec3 a_Offset, void *a_pSrcData, unsigned char a_FillColor);

	struct VertexBinder
	{
		VertexBinder() : m_pVtxRes(nullptr){}
		virtual ~VertexBinder(){ SAFE_RELEASE(m_pVtxRes) }

		ID3D12Resource *m_pVtxRes;
		D3D12_VERTEX_BUFFER_VIEW m_VtxView;
	};
	struct IndexBinder
	{
		IndexBinder() : m_pIndexRes(nullptr){}
		virtual ~IndexBinder(){ SAFE_RELEASE(m_pIndexRes) }

		ID3D12Resource *m_pIndexRes;
		D3D12_INDEX_BUFFER_VIEW m_IndexView;
	};
	struct ConstBufferBinder
	{
		ConstBufferBinder() : m_pResource(nullptr), m_HeapID(0), m_CurrSize(0), m_pCurrBuff(nullptr){}
		virtual ~ConstBufferBinder(){ SAFE_RELEASE(m_pResource) }

		ID3D12Resource *m_pResource;
		unsigned int m_HeapID;
		unsigned int m_CurrSize;
		char *m_pCurrBuff;
	};
	struct UnorderAccessBufferBinder : ConstBufferBinder
	{
		UnorderAccessBufferBinder() : ConstBufferBinder(), m_ElementSize(0), m_ElementCount(0){}
		virtual ~UnorderAccessBufferBinder(){ SAFE_RELEASE(m_pCounterResource) }

		unsigned int m_ElementSize;
		unsigned int m_ElementCount;
		ID3D12Resource *m_pCounterResource;
	};
	struct ReadBackBuffer
	{
		ReadBackBuffer() : m_pTempResource(nullptr), m_pTargetBuffer(nullptr), m_Size(0){}
		virtual ~ReadBackBuffer(){ SAFE_RELEASE(m_pTempResource) }

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
	struct IndirectCommandBinder
	{
		IndirectCommandBinder() : m_pResource(nullptr), m_pTargetBuffer(nullptr){}
		virtual ~IndirectCommandBinder(){ SAFE_RELEASE(m_pResource) }

		ID3D12Resource *m_pResource;
		char *m_pTargetBuffer;
	};

	D3D12GpuThread newThread();

	D3D12HeapManager *m_pShaderResourceHeap, *m_pSamplerHeap, *m_pRenderTargetHeap, *m_pDepthHeap;
	
	SerializedObjectPool<TextureBinder> m_ManagedTexture;
	SerializedObjectPool<SamplerBinder> m_ManagedSampler;
	SerializedObjectPool<RenderTargetBinder> m_ManagedRenderTarget;
	SerializedObjectPool<VertexBinder> m_ManagedVertexBuffer;
	SerializedObjectPool<IndexBinder> m_ManagedIndexBuffer;
	SerializedObjectPool<ConstBufferBinder> m_ManagedConstBuffer;
	SerializedObjectPool<UnorderAccessBufferBinder> m_ManagedUavBuffer;
	SerializedObjectPool<IndirectCommandBinder> m_ManagedIndirectCommandBuffer;

	IDXGIFactory4 *m_pGraphicInterface;
	ID3D12Device *m_pDevice;
	DXGI_SAMPLE_DESC m_MsaaSetting;
	ID3D12CommandQueue *m_pResCmdQueue, *m_pComputeQueue, *m_pDrawCmdQueue;// to do : add bundle support;
	D3D12GpuThread m_ResThread[D3D12_NUM_COPY_THREAD];
	unsigned int m_IdleResThread;
	D3D12Fence *m_pResFence, *m_pComputeFence, *m_pGraphicFence;
	std::vector<ID3D12Resource *> m_TempResources[D3D12_NUM_COPY_THREAD];
	std::vector<ReadBackBuffer> m_ReadBackContainer[D3D12_NUM_COPY_THREAD];
	std::thread m_ResourceLoop, m_GraphicLoop;
	bool m_bLooping;

	std::vector<D3D12GpuThread> m_GraphicReadyThread, m_GraphicBusyThread;
	std::deque<D3D12GpuThread> m_GraphicThread;// idle thread
	std::vector<D3D12Canvas*> m_PresentCanvas;
	std::mutex m_ThreadMutex, m_ResourceMutex;
};

}
#endif