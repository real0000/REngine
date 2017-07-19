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
public:
	SceneNode();
	virtual ~SceneNode();

	void setTransform(glm::mat4x4 a_Transform);
	void addChild(SceneNode *a_pNode);
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
	glm::mat4x4 m_World;
	glm::mat4x4 m_LocalTransform;

	std::vector<SceneNode *> m_Children;
	SceneNode *m_pParent;
	std::map<unsigned int, std::vector< std::shared_ptr<EngineComponent> > > m_Components;
};


class Scene
{
public:
	Scene();
	virtual ~Scene();


	
private:
	SceneNode *m_pRoot;
	CameraComponent *m_pCurrCamera;
};

}
#endif