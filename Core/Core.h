// Core.h
//
// 2014/03/12 Ian Wu/Real0000
//

#ifndef _CORE_H_
#define _CORE_H_

#include "SDL.h"

namespace R
{

// enging config file
#define CONIFG_FILE "Config.ini"

STRING_ENUM_CLASS(GraphicApi,
	d3d11,
	d3d12,
	opengl,
	vulkan)

enum ComponentDefine
{
	// common component
	COMPONENT_CAMERA = 0,
	COMPONENT_MESH,
	COMPONENT_OMNI_LIGHT,
	COMPONENT_SPOT_LIGHT,
	COMPONENT_DIR_LIGHT,
	//COMPONENT_VOXEL_TERRAIN,

	CUSTOM_COMPONENT,// all custom component type id must >= this value
};

struct InputData;
struct SharedSceneMember;
class Asset;
class InputMediator;
class GraphicCanvas;
class Scene;
class SceneNode;
class TextureAsset;
class VertexBuffer;

class EngineComponent : public std::enable_shared_from_this<EngineComponent>
{
	friend class EngineComponent;
public:
	template<typename T>
	static std::shared_ptr<T> create(SharedSceneMember *a_pSharedMember, std::shared_ptr<SceneNode> a_pOwner)
	{
		std::shared_ptr<T> l_pNewComponent = std::shared_ptr<T>(new T(a_pSharedMember, a_pOwner));
		l_pNewComponent->postInit();
		l_pNewComponent->start();
		return l_pNewComponent;
	}

	template<typename T>
	std::shared_ptr<T> shared_from_base()
	{
		return std::static_pointer_cast<T>(shared_from_this());
	}
	
	virtual void postInit();
	virtual void start(){}
	virtual void end(){}
	virtual void hiddenFlagChanged(){}

	virtual unsigned int typeID() = 0;
	virtual bool inputListener(InputData &a_Input){ return false; }
	virtual void updateListener(float a_Delta){}
	virtual void transformListener(glm::mat4x4 &a_NewTransform){}

	wxString getName(){ return m_Name; }
	void setName(wxString a_Name){ m_Name = a_Name; }
	bool isHidden(){ return m_bHidden; }
	void setHidden(bool a_bHidden);
	std::shared_ptr<SceneNode> getOwnerNode();
	void setOwner(std::shared_ptr<SceneNode> a_pOwner);
	void remove();

	// for engine only, user should not call this method
	void detach();

protected:
	EngineComponent(SharedSceneMember *a_pSharedMember, std::shared_ptr<SceneNode> a_pOwner);
	virtual ~EngineComponent();

	// listener
	void addAllListener();
	void removeAllListener();
	void addInputListener();
	void removeInputListener();
	void addUpdateListener();
	void removeUpdateListener();
	void addTransformListener();
	void removeTransformListener();
	
	//
	SharedSceneMember* getSharedMember(){ return m_pMembers; }

private:
	struct
	{
		unsigned int m_bInputListener : 1;
		unsigned int m_bUpdateListener : 1;
		unsigned int m_bTransformListener : 1;
	} m_Flags;

	bool m_bHidden;
	wxString m_Name;
	SharedSceneMember *m_pMembers;
};

class EngineSetting
{
public:
	static EngineSetting& singleton();

	void save();

	// General
	wxString m_Title;

	// Graphic
	GraphicApi::Key m_Api;
	glm::ivec2 m_DefaultSize;
	bool m_bFullScreen;
	unsigned int m_FPS;
	unsigned int m_ShadowMapSize;
	float m_TileSize;
	unsigned int m_NumRenderCommandList;

	// Asset
	wxString m_CDN;

private:
	EngineSetting();
	virtual ~EngineSetting();
};

class EngineCore
{
	friend class EngineComponent;
public:
	static EngineCore& singleton();
	
	// for normal game : 1 canvas with close button
	GraphicCanvas* createCanvas();
	// for tool window
	GraphicCanvas* createCanvas(wxWindow *a_pParent);
	
	bool isShutdown();
	void shutDown();

	// utility
	std::shared_ptr<Asset> getWhiteTexture(){ return m_WhiteTexture.second; }
	std::shared_ptr<VertexBuffer> getQuadBuffer(){ return m_pQuad; }

private:
	EngineCore();
	virtual ~EngineCore();

	bool init();
	void mainLoop();

	bool m_bValid;
	bool m_bShutdown;

	std::pair<int, std::shared_ptr<Asset>> m_WhiteTexture;
	std::shared_ptr<VertexBuffer> m_pQuad;
	InputMediator *m_pInput;
	std::thread m_MainLoop;
};

}

#endif