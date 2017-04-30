// Scene.h
//
// 2015/02/10 Ian Wu/Real0000
//

#ifndef _SCENE_H_
#define _SCENE_H_

namespace R
{

class SceneNode;
class Camera;

class SceneBlock;
class Scene;
/*
class BaseSceneManager
{
public:
	virtual void addSceneNode(SceneNode *l_pNode) = 0;
	virtual void removedSceneNode(SceneNode *l_pNode) = 0;
	virtual void getVisibleList(Camera &l_Viewer, std::vector<SceneNode *> &l_OutputList) = 0;

};*/

class SceneBlock
{
	friend class Scene;
private:
	SceneBlock();
	virtual ~SceneBlock();

	void active();
	void deactive();

	SceneBlock *m_pNeighbors[26];
	std::vector<SceneNode *> m_RootNodes;
	physx::PxScene *m_pPhysicWorld;
};

class Scene
{
public:
	Scene();
	virtual ~Scene();

	void addSceneNode(SceneNode *l_pNode);
	
private:
	std::vector<SceneBlock *> m_SceneBlocks;
};

}
#endif