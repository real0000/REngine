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

class SceneNode
{
	friend class EngineComponent;
public:
	SceneNode();
	virtual ~SceneNode();

	void setTransform(glm::mat4x4 a_Transform);
	std::shared_ptr<SceneNode> addChild();
	void update(glm::mat4x4 a_ParentTranform);

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

	glm::mat4x4 m_World;
	glm::mat4x4 m_LocalTransform;

	std::vector< std::shared_ptr<SceneNode> > m_Children;
	std::shared_ptr<SceneNode> m_pParent;
	std::map<unsigned int, std::set< std::shared_ptr<EngineComponent> > > m_Components;
};

class Scene
{
public:
	void remove(std::shared_ptr<Scene> a_pScene);
	//void setRegionManager();

	// file part
	wxString getFilepath(){ return m_Filepath; }
	void setFilepath(wxString a_Path){ m_Filepath = a_Path; }
	bool load();
	bool save();

	// update part
	void update(float a_Delta);
	void render();
	
private:
	Scene();
	virtual ~Scene();

	wxString m_Filepath;
	std::shared_ptr<SceneNode> m_pRoot;
	std::shared_ptr<CameraComponent> m_pCurrCamera;
	std::map< std::shared_ptr<EngineComponent>, std::vector< std::function<void(float)> > > m_UpdateCallback;
};

class SceneManager
{
public:
	static SceneManager& singleton();

	std::shared_ptr<Scene> create();

	void update(float a_Delta);
	void render();

private:
	SceneManager();
	virtual ~SceneManager();

	std::set< std::shared_ptr<Scene> > m_Scene;
};

}
#endif