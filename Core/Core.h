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

#define BATCHDRAW_UNIT 1024

STRING_ENUM_CLASS(GraphicApi,
	d3d11,
	d3d12,
	opengl,
	vulkan)

enum ComponentDefine
{
	// common component
	COMPONENT_Camera = 0,
	COMPONENT_RenderableMesh,
	COMPONENT_OmniLight,
	COMPONENT_SpotLight,
	COMPONENT_DirLight,
	//COMPONENT_VOXEL_TERRAIN,

	CUSTOM_COMPONENT,// all custom component type id must >= this value
};

enum SceneGraphType
{
	GRAPH_MESH = 0,
	GRAPH_LIGHT,
	GRAPH_STATIC_LIGHT,
	GRAPH_CAMERA,

	NUM_GRAPH_TYPE
};

enum MaterialSlot
{
	MATSLOT_PIPELINE_START = 0,
	MATSLOT_OPAQUE = MATSLOT_PIPELINE_START,
	MATSLOT_TRANSPARENT,
	MATSLOT_OMNI_SHADOWMAP,
	MATSLOT_SPOT_SHADOWMAP,
	MATSLOT_DIR_SHADOWMAP,
	MATSLOT_PIPELINE_END = MATSLOT_DIR_SHADOWMAP,

	MATSLOT_LIGHTMAP,
};

enum TriggerGroup
{
	TRIGGER_CAMERA = 0x01,
	TRIGGER_MESH = 0x02,
	TRIGGER_LIGHT = 0x04,
};

struct InputData;
class Asset;
class InputMediator;
class GraphicCanvas;
class Scene;
class SceneAsset;
class SceneNode;
class TextureAsset;
class VertexBuffer;

class EngineComponent : public std::enable_shared_from_this<EngineComponent>
{
	friend class EngineComponent;
public:
	template<typename T>
	static std::shared_ptr<T> create(std::shared_ptr<Scene> a_pRefScene, std::shared_ptr<SceneNode> a_pOwner)
	{
		std::shared_ptr<T> l_pNewComponent = std::shared_ptr<T>(new T(a_pRefScene, a_pOwner));
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
	virtual void preEnd(){}
	virtual void end(){}
	virtual void hiddenFlagChanged(){}

	virtual unsigned int typeID() = 0;
	virtual bool runtimeOnly(){ return false; }// true if save/load component not required
	virtual void loadComponent(boost::property_tree::ptree &a_Src){}
	virtual void saveComponent(boost::property_tree::ptree &a_Dst){}
	virtual bool inputListener(InputData &a_Input){ return false; }
	virtual void updateListener(float a_Delta){}
	virtual void transformListener(const glm::mat4x4 &a_NewTransform){}

	wxString getName(){ return m_Name; }
	void setName(wxString a_Name){ m_Name = a_Name; }
	bool isHidden(){ return m_bHidden; }
	void setHidden(bool a_bHidden);
	std::shared_ptr<SceneNode> getOwner(){ return m_pOwner; }
	void setOwner(std::shared_ptr<SceneNode> a_pOwner);
	void remove();

	// for engine only, user should not call this method
	void detach();

protected:
	EngineComponent(std::shared_ptr<Scene> a_pRefScene, std::shared_ptr<SceneNode> a_pOwner);
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
	std::shared_ptr<Scene> getScene(){ return m_pRefScene; }

private:
	struct
	{
		unsigned int m_bInputListener : 1;
		unsigned int m_bUpdateListener : 1;
		unsigned int m_bTransformListener : 1;
	} m_Flags;

	bool m_bHidden;
	wxString m_Name;
	std::shared_ptr<Scene> m_pRefScene;
	std::shared_ptr<SceneNode> m_pOwner;
};
#define COMPONENT_HEADER(className) \
	friend class EngineComponent; \
	public:\
		virtual unsigned int typeID(){ return COMPONENT_##className; }	\
		static unsigned int staticTypeID(){ return COMPONENT_##className; }\
		static std::string typeName(){ return #className; }

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
	unsigned int m_LightMapSample;
	unsigned int m_LightMapSampleDepth;
	unsigned int m_LightMapSize;
	float m_TileSize;
	unsigned int m_NumRenderCommandList;

	// Asset
	wxString m_CDN;

private:
	EngineSetting();
	virtual ~EngineSetting();
};

class EngineCore : public wxEvtHandler
{
	friend class EngineComponent;
public:
	static EngineCore& singleton();
	
	// for normal game : 1 canvas with close button
	GraphicCanvas* createCanvas();
	// for tool window
	GraphicCanvas* createCanvas(wxWindow *a_pParent);

	void run(wxApp *a_pMain);
	bool isShutdown();
	void shutDown();
	double getDelta(){ return m_Delta; }// for input event

	// utility
	template<typename T>
	void registComponentReflector(){ SceneNode::registComponentReflector<T>(); }
	template<typename T>
	void registRenderPipelineReflector(){ Scene::registRenderPipelineReflector<T>(); }
	void addJob(std::function<void()> a_Job);
	void join();

private:
	EngineCore();
	virtual ~EngineCore();

	bool init();
	void mainLoop(wxIdleEvent &a_Event);

	bool m_bValid;
	bool m_bShutdown;
	std::mutex m_LoopGuard;
	double m_Delta, m_FrameDelta;

	// prevent asset gc
	std::vector<std::shared_ptr<Asset>> m_RuntimeTexture, m_DefaultShadowmap;
	std::shared_ptr<Asset> m_pQuadMesh;

	InputMediator *m_pInput;
	wxApp *m_pRefMain;
	ThreadPool m_ThreadPool;
};

}

#endif