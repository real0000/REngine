// Scene.h
//
// 2015/02/10 Ian Wu/Real0000
//

#ifndef _SCENE_H_
#define _SCENE_H_

namespace R
{

class CameraComponent;
class EngineComponent;
class Scene;
class SceneNode;
class ScenePartition;

struct SharedSceneMember
{
	enum
	{
		GRAPH_STATIC_MODEL = 0,
		GRAPH_STATIC_LIGHT,
		GRAPH_DYNAMIC_MODEL,
		GRAPH_DYNAMIC_LIGHT,

		NUM_GRAPH_TYPE
	};

	SharedSceneMember();
	~SharedSceneMember();

	SharedSceneMember& operator=(const SharedSceneMember &a_Src);

	ScenePartition *m_pGraphs[NUM_GRAPH_TYPE];
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
		if( m_Components.end() == it || it->second.emplace() ) return;
		a_Output.resize(it->second.size());
		std::copy(it->second.begin(), it->second.end(), a_Output.begin());
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
public:
	static std::shared_ptr<Scene> create();
	virtual ~Scene();

	void destroy();

	// file part
	void initEmpty();
	bool load(wxString a_Path);
	void loadAsync(wxString a_Path, std::function<void(bool)> a_Callback);
	bool save(wxString a_Path);
	float getLoadingProgress(){ return m_LoadingProgress; }

	// update part
	void update(float a_Delta);//for post processor
	void render();

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
};

class SceneManager
{
public:
	static SceneManager& singleton();

	void pop(unsigned int a_ID);
	void push(unsigned int a_ID, std::shared_ptr<Scene> a_pScene);
	void replace(unsigned int a_ID, std::shared_ptr<Scene> a_pScene);

	unsigned int newStack(std::shared_ptr<Scene> a_pStartScene);
	void removeStack(unsigned int a_ID);

	void update(float a_Delta);
	void render();

private:
	SceneManager();
	virtual ~SceneManager();

	std::map<unsigned int, std::deque< std::shared_ptr<Scene> > > m_ActiveScene;
};

}
#endif