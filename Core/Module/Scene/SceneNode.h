// SceneNode.h
//
// 2015/01/24 Ian Wu/Real0000
//

#ifndef _SCENE_NODE_H_
#define _SCENE_NODE_H_

namespace R
{

class GLSLMaterial;

class SceneNode
{
public:
	SceneNode();
	virtual ~SceneNode();

	void setPosition(glm::vec3 l_Position);
	void setRotation(glm::quat l_Rotation);
	void setScale(glm::vec3 l_Scale);
	void setTransform(glm::mat4x4 l_Transform);
	void addChild(SceneNode *l_pNode);

	void update(glm::mat4x4 l_ParentTranform);

private:
	glm::mat4x4 m_World;
	glm::mat4x4 m_LocalTransform;

	std::vector<SceneNode *> m_Children;
	SceneNode *m_pParent;
	physx::PxGeometry *m_pBoundingGeometry;
};

}

#endif