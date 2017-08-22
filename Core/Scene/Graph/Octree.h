// Octree.h
//
// 2017/08/18 Ian Wu/Real0000
//

#ifndef _OCTREE_H_
#define _OCTREE_H_

#include "ScenePartition.h"

namespace R
{

#define DEFAULT_OCTREE_EDGE 32.0
#define DEFAULT_OCTREE_ROOT_SIZE 16384.0

class OctreePartition : public ScenePartition
{
public:
	enum
	{
		NX_NY_NZ = 0,
		NX_NY_PZ,
		NX_PY_NZ,
		NX_PY_PZ,
		PX_NY_NZ,
		PX_NY_PZ,
		PX_PY_NZ,
		PX_PY_PZ,

		NUM_NODE
	};
	struct Node
	{
		Node();
		~Node();

		bool isLeaf();
		void clear();

		SerializedObjectPool<Node> *m_pRefOwner;

		unsigned int m_ID;
		unsigned int m_RelationID;// parent idx
		bool m_bValid;
		std::shared_ptr<Node> m_Children[NUM_NODE];
		std::shared_ptr<Node> m_pParent;
		glm::daabb m_Bounding;

		std::set< std::shared_ptr<RenderableComponent> > m_Models;
	};

public:
	OctreePartition(double a_RootEdge = DEFAULT_OCTREE_ROOT_SIZE, double a_MinEdge = DEFAULT_OCTREE_EDGE);
	virtual ~OctreePartition();

	virtual void add(std::shared_ptr<RenderableComponent> a_pComponent);
	virtual void update(std::shared_ptr<RenderableComponent> a_pComponent);
	virtual void remove(std::shared_ptr<RenderableComponent> a_pComponent);
	virtual void clear();

	virtual void getVisibleList(std::shared_ptr<CameraComponent> a_pTargetCamera, std::vector< std::shared_ptr<RenderableComponent> > &a_Output);

	void setEdge(float a_Edge){ m_Edge = a_Edge; }

private:
	void flush();
	void extend();
	bool checkNode(std::shared_ptr<Node> a_pNode, glm::daabb &a_Box);
	int insertNode(std::shared_ptr<Node> a_pNode, glm::daabb &a_Box);
	void removeNode(std::shared_ptr<Node> a_pNode);

	void assignBoxIntersect(glm::frustumface &a_Frustum, std::shared_ptr<Node> a_pNode, std::vector< std::shared_ptr<RenderableComponent> > &a_Output);
	void assignBoxInside(std::shared_ptr<Node> a_pNode, std::vector< std::shared_ptr<RenderableComponent> > &a_Output);

	double m_Edge;
	std::shared_ptr<Node> m_pRoot;
	SerializedObjectPool<Node> m_NodePool;
	std::map<std::shared_ptr<RenderableComponent>, int> m_ModelMap;
	std::set< std::shared_ptr<RenderableComponent> > m_ModelUpdated;

	std::mutex m_Locker;
};

}
#endif