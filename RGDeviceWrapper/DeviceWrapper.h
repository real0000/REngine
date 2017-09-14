// DeviceManager.h
//
// 2015/08/05 Ian Wu/Real0000
//
#ifndef _DEVICE_MANAGER_H_
#define _DEVICE_MANAGER_H_

namespace R
{

class IndexBuffer;
class ShaderProgram;
class VertexBuffer;
class GraphicCanvas;

// must call use program after thread started
class GraphicCommander
{
public:
	GraphicCommander();
	virtual ~GraphicCommander();

	// draw command
	virtual void useProgram(unsigned int a_Key);
	virtual void useProgram(std::shared_ptr<ShaderProgram> a_pProgram) = 0;
	virtual void bindVertex(VertexBuffer *a_pBuffer) = 0;
	virtual void bindIndex(IndexBuffer *a_pBuffer) = 0;
	virtual void bindTexture(int a_ID, unsigned int a_Stage, bool a_bRenderTarget) = 0;
	virtual void bindConstant(std::string a_Name, unsigned int a_SrcData) = 0;
	virtual void bindConstant(std::string a_Name, void* a_pSrcData, unsigned int a_SizeInUInt) = 0;
	virtual void bindConstBlock(int a_ID, int a_BlockStage) = 0;
	virtual void bindUavBlock(int a_ID, int a_BlockStage) = 0;
	virtual void clearRenderTarget(int a_RTVHandle, glm::vec4 a_Color) = 0;
	virtual void clearBackBuffer(GraphicCanvas *a_pCanvas, glm::vec4 a_Color) = 0;
	virtual void clearDepthTarget(int a_DSVHandle, bool a_bClearDepth, float a_Depth, bool a_bClearStencil, unsigned char a_Stencil) = 0;
	virtual void drawVertex(int a_NumVtx, int a_BaseVtx) = 0;
	virtual void drawElement(int a_BaseIdx, int a_NumIdx, int a_BaseVtx) = 0;
	virtual void drawIndirect(std::shared_ptr<ShaderProgram> a_pProgram, unsigned int a_MaxCmd, void *a_pResPtr, void *a_pCounterPtr, unsigned int a_BufferOffset) = 0;
	virtual void compute(unsigned int a_CountX, unsigned int a_CountY = 1, unsigned int a_CountZ = 1) = 0;

	virtual void setTopology(Topology::Key a_Key) = 0;
	virtual void setRenderTarget(int a_DSVHandle, unsigned int a_NumRT, ...);
	virtual void setRenderTarget(int a_DSVHandle, std::vector<int> &a_RTVHandle) = 0;
	virtual void setRenderTargetWithBackBuffer(int a_DSVHandle, GraphicCanvas *a_pCanvas) = 0;
	virtual void setViewPort(int a_NumViewport, ...) = 0;// glm::Viewport
	virtual void setScissor(int a_NumScissor, ...) = 0;// glm::ivec4
};

class GraphicCanvas : public wxWindow
{
public:
	GraphicCanvas(wxWindow *a_pParent, wxWindowID a_ID);
	virtual ~GraphicCanvas();

	void setFullScreen(bool a_bFullScreen);
	bool isFullScreen(){ return m_bFullScreen; }
	
	void onSize(wxSizeEvent &a_Event);

	virtual void init(bool a_bFullScr) = 0;
	virtual void present() = 0;
	virtual unsigned int getBackBuffer() = 0;

protected:
	virtual void resizeBackBuffer() = 0;

private:
	bool m_bFullScreen;

	DECLARE_EVENT_TABLE()
};

// member function with source data must call on main thread
class GraphicDevice
{
public:
	virtual void initDeviceMap() = 0;
	virtual void initDevice(unsigned int a_DeviceID) = 0;
	virtual void init() = 0;
	virtual GraphicCommander* commanderFactory() = 0;
	virtual GraphicCanvas* canvasFactory(wxWindow *a_pParent, wxWindowID a_ID) = 0;
	virtual std::pair<int, int> maxShaderModel() = 0;
	virtual void wait() = 0;

	// support flags
	virtual bool supportExtraIndirectCommand() = 0;

	// converter part( *::Key -> d3d, vulkan var )
	virtual unsigned int getBlendKey(BlendKey::Key a_Key) = 0;
	virtual unsigned int getBlendOP(BlendOP::Key a_Key) = 0;
	virtual unsigned int getBlendLogic(BlendLogic::Key a_Key) = 0;
	virtual unsigned int getCullMode(CullMode::Key a_Key) = 0;
	virtual unsigned int getComapreFunc(CompareFunc::Key a_Key) = 0;
	virtual unsigned int getStencilOP(StencilOP::Key a_Key) = 0;
	virtual unsigned int getTopologyType(TopologyType::Key a_Key) = 0;
	virtual unsigned int getTopology(Topology::Key a_Key) = 0;
	virtual unsigned int getPixelFormat(PixelFormat::Key a_Key) = 0;
	virtual unsigned int getVertexSlotStride(unsigned int a_Type);//VTXSLOT_*
	virtual unsigned int getParamAlignment(ShaderParamType::Key a_Key) = 0;
	virtual unsigned int getParamAlignmentSize(ShaderParamType::Key a_Key);

	// texture part
	virtual int allocateTexture(unsigned int a_Size, PixelFormat::Key a_Format) = 0;
	virtual int allocateTexture(glm::ivec2 a_Size, PixelFormat::Key a_Format, unsigned int a_ArraySize = 1, bool a_bCube = false) = 0;
	virtual int allocateTexture(glm::ivec3 a_Size, PixelFormat::Key a_Format) = 0;
	virtual void updateTexture(int a_ID, unsigned int a_MipmapLevel, unsigned int a_Size, unsigned int a_Offset, void *a_pSrcData) = 0;
	virtual void updateTexture(int a_ID, unsigned int a_MipmapLevel, glm::ivec2 a_Size, glm::ivec2 a_Offset, unsigned int a_Idx, void *a_pSrcData) = 0;
	virtual void updateTexture(int a_ID, unsigned int a_MipmapLevel, glm::ivec3 a_Size, glm::ivec3 a_Offset, void *a_pSrcData) = 0;
	virtual void generateMipmap(int a_ID) = 0;
	virtual PixelFormat::Key getTextureFormat(int a_ID) = 0;
	virtual glm::ivec3 getTextureSize(int a_ID) = 0;
	virtual TextureType getTextureType(int a_ID) = 0;
	virtual void* getTextureResource(int a_ID) = 0;
	virtual void freeTexture(int a_ID) = 0;

	// render target part
	virtual int createRenderTarget(glm::ivec3 a_Size, PixelFormat::Key a_Format) = 0;// 3d render target, use uav
	virtual int createRenderTarget(glm::ivec2 a_Size, PixelFormat::Key a_Format, unsigned int a_ArraySize = 1, bool a_bCube = false) = 0;// texture 2d array
	virtual void freeRenderTarget(int a_ID) = 0;

	// vertex, index buffer
	virtual int requestVertexBuffer(void *a_pInitData, unsigned int a_Slot, unsigned int a_Count, wxString a_Name = wxT("")) = 0;//a_Slot < VTXSLOT_COUNT
	virtual void updateVertexBuffer(int a_ID, void *a_pData, unsigned int a_SizeInByte) = 0;
	virtual void* getVertexResource(int a_ID) = 0;
	virtual void freeVertexBuffer(int a_ID) = 0;
	virtual int requestIndexBuffer(void *a_pInitData, PixelFormat::Key a_Fmt, unsigned int a_Count, wxString a_Name = wxT("")) = 0;
	virtual void updateIndexBuffer(int a_ID, void *a_pData, unsigned int a_SizeInByte) = 0;
	virtual void* getIndexResource(int a_ID) = 0;
	virtual void freeIndexBuffer(int a_ID) = 0;

	// cbv part
	virtual int requestConstBuffer(char* &a_pOutputBuff, unsigned int a_Size) = 0;//return buffer id
	virtual char* getConstBufferContainer(int a_ID) = 0;
	virtual void* getConstBufferResource(int a_ID) = 0;
	virtual void freeConstBuffer(int a_ID) = 0;

	// uav part
	virtual unsigned int requestUavBuffer(char* &a_pOutputBuff, unsigned int a_UnitSize, unsigned int a_ElementCount) = 0;
	virtual void resizeUavBuffer(int a_ID, char* &a_pOutputBuff, unsigned int a_ElementCount) = 0;
	virtual char* getUavBufferContainer(int a_BuffID) = 0;
	virtual void* getUavBufferResource(int a_BuffID) = 0;
	virtual void syncUavBuffer(bool a_bToGpu, unsigned int a_NumBuff, ...);
	virtual void syncUavBuffer(bool a_bToGpu, std::vector<unsigned int> &a_BuffIDList) = 0;
	virtual void syncUavBuffer(bool a_bToGpu, std::vector< std::tuple<unsigned int, unsigned int, unsigned int> > &a_BuffIDList) = 0;// uav id, start, end
	virtual void freeUavBuffer(int a_BuffID) = 0;
	
protected:
	std::map<unsigned int, wxString> m_DeviceMap;// id : device name

};

class GraphicDeviceManager
{
public:
	static GraphicDeviceManager& singleton();

	template<typename T>
	void init()
	{
		assert(nullptr == m_pDeviceInst);
		m_pDeviceInst = new T();
		m_pDeviceInst->init();
		assert(nullptr != dynamic_cast<GraphicDevice *>(m_pDeviceInst));
	}
	template<typename T>
	T* getTypedDevice(){ return dynamic_cast<T *>(m_pDeviceInst); }
	GraphicDevice* getDevice(){ return m_pDeviceInst; }
	
private:
	GraphicDeviceManager();
	virtual ~GraphicDeviceManager();

	GraphicDevice *m_pDeviceInst;
};
#define TYPED_GDEVICE(type) GraphicDeviceManager::singleton().getTypedDevice<type>()
#define GDEVICE() GraphicDeviceManager::singleton().getDevice()

}

#endif