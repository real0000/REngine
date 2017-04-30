// D3DDevice.h
//
// 2015/11/12 Ian Wu/Real0000
//

#ifndef _D3DDEVICE_H_
#define _D3DDEVICE_H_

#include "DeviceWrapper.h"

namespace R
{

class Dircet3DDevice : public GraphicDevice
{
public:
	Dircet3DDevice();
	virtual ~Dircet3DDevice();
	
	// converter part( *::Key -> d3d, vulkan var )
	virtual unsigned int getPixelFormat(PixelFormat::Key l_Key);
	virtual unsigned int getParamAlignment(ShaderParamType::Key l_Key);
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
	virtual void setupCanvas(wxWindow *a_pCanvas, glm::ivec2 a_Size, bool a_bFullScr, bool a_bStereo = false);
	virtual void resetCanvas(wxWindow *a_pCanvas, glm::ivec2 a_Size, bool a_bFullScr, bool a_bStereo = false);
	virtual void destroyCanvas(wxWindow *a_pCanvas);
	virtual void waitGpuSync(wxWindow *a_pCanvas);
	virtual void waitFrameSync(wxWindow *a_pCanvas);
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
	ID3D12Device* getDeviceInst(){ return m_pDevice; }

private:
	class HeapManager
	{
	public:
		HeapManager(ID3D12Device *a_pDevice, unsigned int a_ExtendSize, D3D12_DESCRIPTOR_HEAP_TYPE a_Type, D3D12_DESCRIPTOR_HEAP_FLAGS a_Flag = D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
		~HeapManager();

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
	class CanvasItem
	{
	public:
		CanvasItem(HeapManager *l_pHeapOwner);
		virtual ~CanvasItem();

		void init(HWND a_Wnd, glm::ivec2 a_Size, bool a_bFullScr, IDXGIFactory4 *a_pFactory, ID3D12Device *a_pDevice, ID3D12CommandQueue *a_pCmdQueue);
		void resize(HWND a_Wnd, glm::ivec2 a_Size, bool a_bFullScr, IDXGIFactory4 *a_pFactory, ID3D12Device *a_pDevice, ID3D12CommandQueue *a_pCmdQueue);

		void waitGpuSync(ID3D12CommandQueue *a_pQueue);
		void waitFrameSync(ID3D12CommandQueue *a_pQueue);
		bool isFullScreen(){ return m_bFullScreen; }
		bool isStereo(){ return m_bStereo; }

		void setStereo(bool a_bStereo){ m_bStereo = a_bStereo; }

	private:
		HANDLE m_FenceEvent;
		uint64 m_SyncVal;
		ID3D12Fence *m_pSynchronizer;

		bool m_bFullScreen;
		bool m_bStereo;
		unsigned int m_BackBuffer[NUM_BACKBUFFER];
		ID3D12Resource *m_pBackbufferRes[NUM_BACKBUFFER];
		IDXGISwapChain *m_pSwapChain;

		HeapManager *m_pRefHeapOwner;
	};
	
	HeapManager *m_pShaderResourceHeap, *m_pSamplerHeap, *m_pRenderTargetHeap, *m_pDepthHeap;
	std::map<wxWindow *, CanvasItem *> m_CanvasContainer;
	
	IDXGIFactory4 *m_pGraphicInterface;
	ID3D12Device *m_pDevice;
	ID3D12CommandQueue *m_pCmdQueue;
	ID3D12CommandAllocator *m_pResCmdAllocator;
	ID3D12GraphicsCommandList *m_pResCmdList;
};

}

#endif