// DeviceManager.h
//
// 2015/08/05 Ian Wu/Real0000
//
#ifndef _DEVICE_MANAGER_H_
#define _DEVICE_MANAGER_H_

namespace R
{

class GraphicThread
{
public:
	virtual void init() = 0;

};

class GraphicDevice
{
public:
	virtual void initDeviceMap() = 0;
	virtual void initDevice(unsigned int a_DeviceID) = 0;
	virtual void init() = 0;
	virtual void setupCanvas(wxWindow *a_pCanvas, glm::ivec2 a_Size, bool a_bFullScr, bool a_bStereo) = 0;
	virtual void resetCanvas(wxWindow *a_pCanvas, glm::ivec2 a_Size, bool a_bFullScr, bool a_bStereo) = 0;
	virtual void destroyCanvas(wxWindow *a_pCanvas) = 0;
	virtual void waitGpuSync(wxWindow *a_pCanvas) = 0;
	virtual void waitFrameSync(wxWindow *a_pCanvas) = 0;
	virtual std::pair<int, int> maxShaderModel() = 0;

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
	virtual unsigned int getPixelSize(PixelFormat::Key a_Key);
	virtual unsigned int getParamAlignment(ShaderParamType::Key a_Key) = 0;
	virtual unsigned int getParamAlignmentSize(ShaderParamType::Key a_Key);

	// texture part
	virtual unsigned int allocateTexture1D(unsigned int a_Size, PixelFormat::Key a_Format) = 0;
	virtual unsigned int allocateTexture2D(glm::ivec2 a_Size, PixelFormat::Key a_Format, unsigned int a_ArraySize = 1) = 0;
	virtual unsigned int allocateTexture3D(glm::ivec3 a_Size, PixelFormat::Key a_Format) = 0;
	virtual void updateTexture1D(unsigned int a_TextureID, unsigned int a_Size, unsigned int a_Offset, void *a_pSrcData) = 0;
	virtual void updateTexture2D(unsigned int a_TextureID, glm::ivec2 a_Size, glm::ivec2 a_Offset, unsigned int a_Idx, void *a_pSrcData) = 0;
	virtual void updateTexture3D(unsigned int a_TextureID, glm::ivec3 a_Size, glm::ivec3 a_Offset, void *a_pSrcData) = 0;
	virtual void freeTexture(unsigned int l_TextureID) = 0;

	virtual PixelFormat::Key getTexturePixelFormat(unsigned int a_TextureID) = 0;
	virtual unsigned int getTexturePixelSize(unsigned int a_TextureID) = 0; 
	virtual glm::ivec3 getTextureDimension(unsigned int a_TextureID) = 0;
	
	// render target part
	virtual unsigned int createRenderTarget(unsigned int a_TextureID) = 0;
	virtual unsigned int createDepthStencilTarget(unsigned int a_TextureID) = 0;

	// const buffers part

	// get resource pointer
	virtual void* getTextureResource() = 0;

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
#define GET_GDEVICE(type) GraphicDeviceManager::singleton().getTypedDevice<type>()
#define GDEVICE() GraphicDeviceManager::singleton().getDevice()

}

#endif