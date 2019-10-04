// Scene.h
//
// 2015/02/10 Ian Wu/Real0000
//

#ifndef _SCENE_H_
#define _SCENE_H_

namespace R
{

struct InputData;
class CameraComponent;
class DirLight;
class EngineComponent;
class MeshBatcher;
class ModelCache;
class OmniLight;
class RenderPipeline;
class Scene;
class SceneNode;
class ScenePartition;
class SpotLight;

template<typename T>
class LightContainer;

struct SharedSceneMember
{
	enum
	{
		GRAPH_MESH = 0,
		GRAPH_LIGHT,
		GRAPH_CAMERA,

		NUM_GRAPH_TYPE
	};

	SharedSceneMember();
	~SharedSceneMember();

	SharedSceneMember& operator=(const SharedSceneMember &a_Src);
	
	ScenePartition *m_pGraphs[NUM_GRAPH_TYPE];
	RenderPipeline *m_pRenderer;
	LightContainer<DirLight> *m_pDirLights;
	LightContainer<OmniLight> *m_pOmniLights;
	LightContainer<SpotLight> *m_pSpotLights;
	MeshBatcher *m_pBatcher;
	ModelCache *m_pModelFactory;
	std::shared_ptr<Scene> m_pScene;
	std::shared_ptr<SceneNode> m_pSceneNode;
};

class SceneNode : public std::enable_shared_from_this<SceneNode>
{
	friend class EngineComponent;
	friend class SceneNode;
public:
	virtual ~SceneNode();// don't call this method directly
	static std::shared_ptr<SceneNode> create(SharedSceneMember *a_pSharedMember, std::shared_ptr<SceneNode> a_pOwner, wxString a_Name = wxT("NoName"));

	std::shared_ptr<SceneNode> addChild();
	template<typename T>
	std::shared_ptr<T> addComponent()
	{
		return EngineComponent::create<T>(m_pMembers, shared_from_this());
	}
	
	void destroy();
	void setParent(std::shared_ptr<SceneNode> a_pNewParent);
	std::shared_ptr<SceneNode> find(wxString a_Name);

	void setTransform(glm::mat4x4 a_Transform);
	void update(glm::mat4x4 a_ParentTranform);

	wxString getName(){ return m_Name; }
	void setName(wxString a_Name){ m_Name = a_Name; }
	const std::list< std::shared_ptr<SceneNode> >& getChildren(){ return m_Children; }
	const std::shared_ptr<SceneNode> getParent(){ return m_pMembers->m_pSceneNode; }
	glm::mat4x4& getTransform(){ return m_World; }

	std::shared_ptr<EngineComponent> getComponent(wxString a_Name);
	void getComponent(wxString a_Name, std::vector< std::shared_ptr<EngineComponent> > &a_Output);
	template<typename T>
	void getComponent(unsigned int a_TypeID, std::vector< std::shared_ptr<T> > &a_Output)
	{
		auto it = m_Components.find(a_TypeID);
		if( m_Components.end() == it || it->second.empty() ) return;
		a_Output.resize(it->second.size());
		std::transform(it->second.begin(), it->second.end(), a_Output.begin()
			, [=](std::shared_ptr<EngineComponent> a_pComponent){ return reinterpret_cast<std::shared_ptr<T>&>(a_pComponent); });
	}
	
private:
	SceneNode(SharedSceneMember *a_pSharedMember, std::shared_ptr<SceneNode> a_pOwner, wxString a_Name);

	void add(std::shared_ptr<EngineComponent> a_pComponent);
	void remove(std::shared_ptr<EngineComponent> a_pComponent);
	void addTranformListener(std::shared_ptr<EngineComponent> a_pComponent);
	void removeTranformListener(std::shared_ptr<EngineComponent> a_pComponent);

	wxString m_Name;
	glm::mat4x4 m_World;
	glm::mat4x4 m_LocalTransform;

	std::list< std::shared_ptr<SceneNode> > m_Children;
	SharedSceneMember *m_pMembers;// scene node -> parent

	std::list< std::shared_ptr<EngineComponent> > m_TransformListener;// use std::set instead ?
	std::map<unsigned int, std::set< std::shared_ptr<EngineComponent> > > m_Components;
};

class Scene : public std::enable_shared_from_this<Scene>
{
	friend class Scene;
	friend class SceneManager;
public:
	virtual ~Scene();

	void destroy();

	// file part
	void initEmpty();
	bool load(wxString a_Path);
	void loadAsync(wxString a_Path, std::function<void(bool)> a_Callback);
	bool save(wxString a_Path);
	float getLoadingProgress(){ return m_LoadingProgress; }

	// update part
	void preprocessInput();
	void processInput(InputData &a_Data);
	void update(float a_Delta);
	void render(GraphicCanvas *a_pCanvas);
	
	// listener
	void addUpdateListener(std::shared_ptr<EngineComponent> a_pListener);
	void removeUpdateListener(std::shared_ptr<EngineComponent> a_pListener);
	void addInputListener(std::shared_ptr<EngineComponent> a_pComponent);
	void removeInputListener(std::shared_ptr<EngineComponent> a_pComponent);
	void clearInputListener();

	// misc;
	std::shared_ptr<SceneNode> getRootNode();
	void pause(){ m_bActivate = false; }
	void resume(){ m_bActivate = true; }

private:
	Scene();

	void clear();
	void loadAsyncThread(wxString a_Path);
	void loadScenePartitionSetting(boost::property_tree::ptree &a_Node);
	void loadRenderPipelineSetting(boost::property_tree::ptree &a_Node);

	bool m_bLoading;
	float m_LoadingProgress;
	std::function<void(bool)> m_LoadingCompleteCallback;

	SharedSceneMember *m_pMembers;
	std::shared_ptr<CameraComponent> m_pCurrCamera;
	
	std::mutex m_InputLocker;
	std::list< std::shared_ptr<EngineComponent> > m_InputListener, m_ReadyInputListener;
	std::set< std::shared_ptr<EngineComponent> > m_DroppedInputListener;
	std::list< std::shared_ptr<EngineComponent> > m_UpdateCallback;
	bool m_bActivate;
};

class SceneManager
{
public:
	static SceneManager& singleton();
	std::shared_ptr<Scene> create(wxString a_Name);

	void setMainScene(GraphicCanvas *a_pCanvas, std::shared_ptr<Scene> a_pScene);
	void dropScene(wxString a_Name);
	void dropCanvas(GraphicCanvas *a_pWeak);
	std::shared_ptr<Scene> getScene(wxString a_Name);

	// focused main scene only
	void processInput(std::vector<InputData *> &a_Data);
	void update(float a_Delta);
	void render();

private:
	SceneManager();
	virtual ~SceneManager();

	std::map<std::shared_ptr<Scene>, GraphicCanvas*> m_CanvasMainScene;
	std::map<wxString, std::shared_ptr<Scene> > m_SceneMap;
	std::mutex m_CanvasDropLock;
};

}
#endif