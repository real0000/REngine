// Core.h
//
// 2014/03/12 Ian Wu/Real0000
//

#ifndef _CORE_H_
#define _CORE_H_

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
	COMPONENT_MODEL_MESH,

	// scene manager
	COMPONENT_BVH_NODE,
	
	CUSTOM_COMPONENT,// all custom component type id must >= this value
};

class SceneNode;
class GraphicCanvas;

class EngineComponent
{
public:
	EngineComponent(SceneNode *a_pOwner);
	virtual ~EngineComponent();

	virtual unsigned int typeID() = 0;
	virtual bool isHidden() = 0;

	wxString getName(){ return m_Name; }
	void setName(wxString a_Name){ m_Name = a_Name; }
	SceneNode* getOwnerNode(){ return m_pRefOwner; }
	void setOwnerNode(SceneNode *a_pOwner);

private:
	wxString m_Name;
	SceneNode *m_pRefOwner;
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

private:
	EngineSetting();
	virtual ~EngineSetting();
};

class EngineCore
{
public:
	static EngineCore& singleton();
	
	// for normal game : 1 canvas with close button
	GraphicCanvas* createCanvas();
	// for tool window
	GraphicCanvas* createCanvas(wxWindow *a_pParent);
	void destroyCanvas(GraphicCanvas *a_pCanvas);

	bool isShutdown();
	void shutDown();

private:
	EngineCore();
	virtual ~EngineCore();

	bool init();
	void mainLoop();

	bool m_bValid;
	bool m_bShutdown;

	std::thread *m_pMainLoop;
	std::set<GraphicCanvas *> m_ManagedCanvas;
	std::mutex m_CanvasLock;
};

}

#endif