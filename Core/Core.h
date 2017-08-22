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
	COMPONENT_MODEL_MESH,
	//COMPONENT_MODEL_VOXEL_TERRAIN,

	// scene manager
	COMPONENT_BVH_NODE,
	
	CUSTOM_COMPONENT,// all custom component type id must >= this value
};

struct InputData;
struct SharedSceneMember;
class SceneNode;
class EngineCanvas;
class InputMediator;

class EngineComponent : public std::enable_shared_from_this<EngineComponent>
{
public:
	template<typename T>
	static std::shared_ptr<T> create(SharedSceneMember *a_pSharedMember, std::shared_ptr<SceneNode> a_pOwner)
	{
		return std::shared_ptr<T>(new T(a_pSharedMember, a_pOwner));
	}

	virtual unsigned int typeID() = 0;
	virtual bool isHidden() = 0;
	virtual bool inputListener(InputData &a_Input){ return false; }
	virtual void updateListener(float a_Delta){}
	virtual void transformListener(glm::mat4x4 &a_NewTransform){}

	wxString getName(){ return m_Name; }
	void setName(wxString a_Name){ m_Name = a_Name; }
	std::shared_ptr<SceneNode> getOwnerNode();
	void setOwner(std::shared_ptr<SceneNode> a_pOwner);
	void remove();

	void detach();//for engine use, user should not call this method

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
	EngineCanvas* createCanvas();
	// for tool window
	EngineCanvas* createCanvas(wxWindow *a_pParent);
	void destroyCanvas(EngineCanvas *a_pCanvas);

	bool isShutdown();
	void shutDown();

private:
	EngineCore();
	virtual ~EngineCore();
	
	// listener
	void addInputListener(std::shared_ptr<EngineComponent> a_pListener);
	void removeInputListener(std::shared_ptr<EngineComponent> a_pListener);
	void addUpdateListener(std::shared_ptr<EngineComponent> a_pListener);
	void removeUpdateListener(std::shared_ptr<EngineComponent> a_pListener);

	bool init();
	void mainLoop();

	bool m_bValid;
	bool m_bShutdown;

	InputMediator *m_pInput;
	std::thread *m_pMainLoop;
	std::set<EngineCanvas *> m_ManagedCanvas;
	std::list< std::shared_ptr<EngineComponent> > m_UpdateCallback;
	std::mutex m_CanvasLock;
};

}

#endif