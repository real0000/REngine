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

class SceneNode : public std::enable_shared_from_this<SceneNode>
{
	friend class EngineComponent;
	friend class SceneNode;
public:
	SceneNode(wxString a_Name = wxT("NoName"));
	virtual ~SceneNode();

	std::shared_ptr<SceneNode> addChild();
	void destroy();
	void setParent(std::shared_ptr<SceneNode> a_pNewParent);
	std::shared_ptr<SceneNode> find(wxString a_Name);

	void setTransform(glm::mat4x4 a_Transform);
	void update(glm::mat4x4 a_ParentTranform);

	wxString getName(){ return m_Name; }
	void setName(wxString a_Name){ m_Name = a_Name; }
	const std::list< std::shared_ptr<SceneNode> >& getChildren(){ return m_Children; }
	const std::shared_ptr<SceneNode> getParent(){ return m_pParent; }

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
	void add(std::shared_ptr<EngineComponent> a_pComponent);
	void remove(std::shared_ptr<EngineComponent> a_pComponent);

	wxString m_Name;
	glm::mat4x4 m_World;
	glm::mat4x4 m_LocalTransform;

	std::list< std::shared_ptr<SceneNode> > m_Children;
	std::shared_ptr<SceneNode> m_pParent;
	std::map<unsigned int, std::set< std::shared_ptr<EngineComponent> > > m_Components;
};

class Scene
{
public:
	Scene();
	virtual ~Scene();
	static std::shared_ptr<Scene> create();

	// file part
	wxString getFilepath(){ return m_Filepath; }
	void setFilepath(wxString a_Path){ m_Filepath = a_Path; }
	bool load();
	bool loadAsync(std::function<void> a_Callback);
	bool save();

	// update part
	void update(float a_Delta);
	void render();

private:
	wxString m_Filepath;
	std::function<void()> m_LoadingCompleteCallback;

	std::shared_ptr<SceneNode> m_pRoot;
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