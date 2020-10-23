// Octree.cpp
//
// 2017/08/18 Ian Wu/Real0000
//

#include "CommonUtil.h"
#include "Core.h"
#include "RenderObject/RenderObject.h"

#include "Octree.h"
#include "Scene/Camera.h"
#include "Scene/Scene.h"

namespace R
{

#pragma region OctreePartition_Node
//
// OctreePartition::Node
//
OctreePartition::Node::Node()
	: m_ID(0)
	, m_RelationID(0)
	, m_bValid(false)
	, m_pParent(nullptr)
{
	for( unsigned int i=0 ; i<NUM_OCT_POS ; ++i ) m_Children[i] = nullptr;
}

OctreePartition::Node::~Node()
{
}

bool OctreePartition::Node::isLeaf()
{
	for( unsigned int i=0 ; i<NUM_OCT_POS ; ++i )
	{
		if( nullptr != m_Children[0] ) return false;
	}
	return true;
}

void OctreePartition::Node::clear()
{
	m_Models.clear();
	for( unsigned int i=0 ; i<NUM_OCT_POS ; ++i )
	{
		if( nullptr != m_Children[i] ) m_Children[i]->clear();
		m_Children[i] = nullptr;
	}
	//m_pRefOwner->release(m_ID);
	m_pParent = nullptr;
}
#pragma endregion

#pragma region OctreePartition
//
// OctreePartition
//
OctreePartition::OctreePartition(double a_RootEdge, double a_MinEdge)
	: m_Edge(a_MinEdge)
	, m_NodePool(true)
	, m_pRoot(nullptr)
{
	BIND_DEFAULT_ALLOCATOR(OctreePartition::Node, m_NodePool)

	unsigned int l_ID = m_NodePool.retain(&m_pRoot);
	m_pRoot->m_Bounding.m_Size = glm::vec3(a_RootEdge);
	m_pRoot->m_Bounding.m_Center = glm::zero<glm::vec3>();
	m_pRoot->m_bValid = true;
}

OctreePartition::~OctreePartition()
{
	clear();
	m_pRoot = nullptr;
	m_NodePool.clear();
}

void OctreePartition::add(std::shared_ptr<RenderableComponent> a_pComponent)
{
	m_ModelUpdated.insert(a_pComponent);
}

void OctreePartition::update(std::shared_ptr<RenderableComponent> a_pComponent)
{
	if( m_ModelUpdated.find(a_pComponent) != m_ModelUpdated.end() ) return;

	auto it = m_ModelMap.find(a_pComponent);
	if( m_ModelMap.end() == it ) return;
	
	glm::daabb &l_Box = a_pComponent->boundingBox();
	std::shared_ptr<Node> l_pTargetNode = m_NodePool[it->second];
	if( !l_pTargetNode->m_Bounding.inRange(l_Box) )
	{
		m_ModelUpdated.insert(a_pComponent);
		l_pTargetNode->m_Models.erase(a_pComponent);
		return;
	}

	int l_NewNodeID = insertNode(l_pTargetNode, l_Box);
	if( l_NewNodeID == it->second ) return;

	l_pTargetNode->m_Models.erase(a_pComponent);
	l_pTargetNode = m_NodePool[l_NewNodeID];
	l_pTargetNode->m_Models.insert(a_pComponent);
	m_ModelMap[a_pComponent] = l_NewNodeID;
}

void OctreePartition::remove(std::shared_ptr<RenderableComponent> a_pComponent)
{
	auto it = m_ModelMap.find(a_pComponent);
	if( m_ModelMap.end() != it )
	{
		auto l_pNode = m_NodePool[it->second];
		l_pNode->m_Models.erase(a_pComponent);
		if( l_pNode->m_Models.empty() && l_pNode->isLeaf() ) removeNode(l_pNode);
	}

	m_ModelMap.erase(a_pComponent);
	m_ModelUpdated.erase(a_pComponent);
}

void OctreePartition::clear()
{
	m_ModelMap.clear();
	m_ModelUpdated.clear();
	m_pRoot->clear();
}

void OctreePartition::getVisibleList(std::shared_ptr<CameraComponent> a_pTargetCamera, std::vector< std::shared_ptr<RenderableComponent> > &a_Output)
{
	flush();
	assignBoxIntersect(a_pTargetCamera->getFrustum(), m_pRoot, a_Output);
}

void OctreePartition::getAllComponent(std::vector<std::shared_ptr<RenderableComponent>> &a_Output)
{
	a_Output.clear();
	for( auto it=m_ModelMap.begin() ; it!=m_ModelMap.end() ; ++it ) a_Output.push_back(it->first);
}

void OctreePartition::flush()
{
	std::lock_guard<std::mutex> l_Guard(m_Locker);
	if( m_ModelUpdated.empty() ) return;

	for( auto it = m_ModelUpdated.begin() ; it != m_ModelUpdated.end() ; ++it )
	{
		glm::daabb &l_Box = (*it)->boundingBox();
		if( !m_pRoot->m_Bounding.inRange(l_Box) )
		{
			do extend();
			while(m_pRoot->m_Bounding.inRange(l_Box));
		}

		int l_ID = insertNode(m_pRoot, l_Box);
		auto l_pNode = m_NodePool[l_ID];
		l_pNode->m_Models.insert(*it);
		m_ModelMap[*it] = l_ID;
	}
	m_ModelUpdated.clear();
}

void OctreePartition::extend()
{
	std::shared_ptr<Node> l_pNewRoot = nullptr;
	unsigned int l_ID = m_NodePool.retain(&l_pNewRoot);
	l_pNewRoot->m_ID = l_ID;
	l_pNewRoot->m_bValid = true;
	l_pNewRoot->m_pParent = nullptr;
	l_pNewRoot->m_Bounding.m_Size = m_pRoot->m_Bounding.m_Size * 2.0;
	l_pNewRoot->m_Bounding.m_Center = glm::zero<glm::vec3>();
	for( unsigned int i=0 ; i<NUM_OCT_POS ; ++i )
	{
		unsigned int l_ChildID = m_NodePool.retain(&(l_pNewRoot->m_Children[i]));
		l_pNewRoot->m_Children[i]->m_ID = l_ChildID;
		l_pNewRoot->m_Children[i]->m_RelationID = i;
		l_pNewRoot->m_Children[i]->m_bValid = true;
		l_pNewRoot->m_Children[i]->m_pParent = l_pNewRoot;
		l_pNewRoot->m_Children[i]->m_Bounding.m_Size = m_pRoot->m_Bounding.m_Size;
	}

	glm::vec3 l_Min(m_pRoot->m_Bounding.getMin()), l_Max(m_pRoot->m_Bounding.getMax());

	l_pNewRoot->m_Children[OCT_NX_NY_NZ]->m_Bounding.m_Center = l_Min;
	l_pNewRoot->m_Children[OCT_NX_NY_NZ]->m_Children[OCT_PX_PY_PZ] = m_pRoot->m_Children[OCT_NX_NY_NZ];
	m_pRoot->m_Children[OCT_NX_NY_NZ]->m_RelationID = OCT_PX_PY_PZ;
	m_pRoot->m_Children[OCT_NX_NY_NZ]->m_pParent = l_pNewRoot->m_Children[OCT_NX_NY_NZ];
	m_pRoot->m_Children[OCT_NX_NY_NZ] = nullptr;

	l_pNewRoot->m_Children[OCT_NX_NY_PZ]->m_Bounding.m_Center = glm::vec3(l_Min.x, l_Min.y, l_Max.z);
	l_pNewRoot->m_Children[OCT_NX_NY_PZ]->m_Children[OCT_PX_PY_NZ] = m_pRoot->m_Children[OCT_NX_NY_PZ];
	m_pRoot->m_Children[OCT_NX_NY_PZ]->m_RelationID = OCT_PX_PY_NZ;
	m_pRoot->m_Children[OCT_NX_NY_PZ]->m_pParent = l_pNewRoot->m_Children[OCT_NX_NY_PZ];
	m_pRoot->m_Children[OCT_NX_NY_PZ] = nullptr;
	
	l_pNewRoot->m_Children[OCT_NX_PY_NZ]->m_Bounding.m_Center = glm::vec3(l_Min.x, l_Max.y, l_Min.z);
	l_pNewRoot->m_Children[OCT_NX_PY_NZ]->m_Children[OCT_PX_NY_PZ] = m_pRoot->m_Children[OCT_NX_PY_NZ];
	m_pRoot->m_Children[OCT_NX_PY_NZ]->m_RelationID = OCT_PX_NY_PZ;
	m_pRoot->m_Children[OCT_NX_PY_NZ]->m_pParent = l_pNewRoot->m_Children[OCT_NX_PY_NZ];
	m_pRoot->m_Children[OCT_NX_PY_NZ] = nullptr;

	l_pNewRoot->m_Children[OCT_NX_PY_PZ]->m_Bounding.m_Center = glm::vec3(l_Min.x, l_Max.y, l_Max.z);
	l_pNewRoot->m_Children[OCT_NX_PY_PZ]->m_Children[OCT_PX_NY_NZ] = m_pRoot->m_Children[OCT_NX_PY_PZ];
	m_pRoot->m_Children[OCT_NX_PY_PZ]->m_RelationID = OCT_PX_NY_NZ;
	m_pRoot->m_Children[OCT_NX_PY_PZ]->m_pParent = l_pNewRoot->m_Children[OCT_NX_PY_PZ];
	m_pRoot->m_Children[OCT_NX_PY_PZ] = nullptr;
	
	l_pNewRoot->m_Children[OCT_PX_NY_NZ]->m_Bounding.m_Center = glm::vec3(l_Max.x, l_Min.y, l_Min.z);
	l_pNewRoot->m_Children[OCT_PX_NY_NZ]->m_Children[OCT_NX_PY_PZ] = m_pRoot->m_Children[OCT_PX_NY_NZ];
	m_pRoot->m_Children[OCT_PX_NY_NZ]->m_RelationID = OCT_NX_PY_PZ;
	m_pRoot->m_Children[OCT_PX_NY_NZ]->m_pParent = l_pNewRoot->m_Children[OCT_PX_NY_NZ];
	m_pRoot->m_Children[OCT_PX_NY_NZ] = nullptr;

	l_pNewRoot->m_Children[OCT_PX_NY_PZ]->m_Bounding.m_Center = glm::vec3(l_Max.x, l_Min.y, l_Max.z);
	l_pNewRoot->m_Children[OCT_PX_NY_PZ]->m_Children[OCT_NX_PY_NZ] = m_pRoot->m_Children[OCT_PX_NY_PZ];
	m_pRoot->m_Children[OCT_PX_NY_PZ]->m_RelationID = OCT_NX_PY_NZ;
	m_pRoot->m_Children[OCT_PX_NY_PZ]->m_pParent = l_pNewRoot->m_Children[OCT_PX_NY_PZ];
	m_pRoot->m_Children[OCT_PX_NY_PZ] = nullptr;

	l_pNewRoot->m_Children[OCT_PX_PY_NZ]->m_Bounding.m_Center = glm::vec3(l_Max.x, l_Max.y, l_Min.z);
	l_pNewRoot->m_Children[OCT_PX_PY_NZ]->m_Children[OCT_NX_NY_PZ] = m_pRoot->m_Children[OCT_PX_PY_NZ];
	m_pRoot->m_Children[OCT_PX_PY_NZ]->m_RelationID = OCT_NX_NY_PZ;
	m_pRoot->m_Children[OCT_PX_PY_NZ]->m_pParent = l_pNewRoot->m_Children[OCT_PX_PY_NZ];
	m_pRoot->m_Children[OCT_PX_PY_NZ] = nullptr;
	
	l_pNewRoot->m_Children[OCT_PX_PY_PZ]->m_Bounding.m_Center = l_Max;
	l_pNewRoot->m_Children[OCT_PX_PY_PZ]->m_Children[OCT_NX_NY_NZ] = m_pRoot->m_Children[OCT_PX_PY_PZ];
	m_pRoot->m_Children[OCT_PX_PY_PZ]->m_RelationID = OCT_NX_NY_NZ;
	m_pRoot->m_Children[OCT_PX_PY_PZ]->m_pParent = l_pNewRoot->m_Children[OCT_PX_PY_PZ];
	m_pRoot->m_Children[OCT_PX_PY_PZ] = nullptr;

	for( auto it = m_pRoot->m_Models.begin() ; it != m_pRoot->m_Models.end() ; ++it )
	{
		l_pNewRoot->m_Models.insert(*it);
		m_ModelMap[*it] = l_pNewRoot->m_ID;
	}

	m_pRoot->m_Models.clear();
	m_pRoot->m_bValid = false;
	m_NodePool.release(m_pRoot->m_ID);
	m_pRoot = l_pNewRoot;
}

bool OctreePartition::checkNode(std::shared_ptr<Node> a_pNode, glm::daabb &a_Box)
{
	glm::dvec3 l_BoxMin(a_Box.getMin()), l_BoxMax(a_Box.getMax());
	return (l_BoxMin.x <= a_pNode->m_Bounding.m_Center.x && l_BoxMax.x >= a_pNode->m_Bounding.m_Center.x) ||
			(l_BoxMin.y <= a_pNode->m_Bounding.m_Center.y && l_BoxMax.y >= a_pNode->m_Bounding.m_Center.y) ||
			(l_BoxMin.z <= a_pNode->m_Bounding.m_Center.z && l_BoxMax.z >= a_pNode->m_Bounding.m_Center.z) ||
			std::abs(a_pNode->m_Bounding.m_Size.x - m_Edge) < std::numeric_limits<double>::epsilon();
}

int OctreePartition::insertNode(std::shared_ptr<Node> a_pNode, glm::daabb &a_Box)
{
	if( checkNode(a_pNode, a_Box) ) return a_pNode->m_ID;

	glm::dvec3 l_BoxCenter(a_Box.m_Center);	 
	unsigned int l_RelationID = 0;
	if( l_BoxCenter.x <= a_pNode->m_Bounding.m_Center.x )
	{
		if( l_BoxCenter.y <= a_pNode->m_Bounding.m_Center.y )
		{
			l_RelationID = l_BoxCenter.z <= a_pNode->m_Bounding.m_Center.z ? OCT_NX_NY_NZ : OCT_NX_NY_PZ;
		}
		else
		{
			l_RelationID = l_BoxCenter.z <= a_pNode->m_Bounding.m_Center.z ? OCT_NX_PY_NZ : OCT_NX_PY_PZ;
		}
	}
	else
	{
		if( l_BoxCenter.y <= a_pNode->m_Bounding.m_Center.y )
		{
			l_RelationID = l_BoxCenter.z <= a_pNode->m_Bounding.m_Center.z ? OCT_PX_NY_NZ : OCT_PX_NY_PZ;
		}
		else
		{
			l_RelationID = l_BoxCenter.z <= a_pNode->m_Bounding.m_Center.z ? OCT_PX_PY_NZ : OCT_PX_PY_PZ;
		}
	}

	std::shared_ptr<Node> l_pTargetNode = nullptr;
	if( nullptr == a_pNode->m_Children[l_RelationID] )
	{
		const glm::dvec3 c_Multiply[NUM_OCT_POS] = {
			glm::dvec3(-1.0, -1.0, -1.0),
			glm::dvec3(-1.0, -1.0, 1.0),
			glm::dvec3(-1.0, 1.0, -1.0),
			glm::dvec3(-1.0, 1.0, 1.0),
			glm::dvec3(1.0, -1.0, -1.0),
			glm::dvec3(1.0, -1.0, 1.0),
			glm::dvec3(1.0, 1.0, -1.0),
			glm::dvec3(1.0, 1.0, 1.0)};
	
		unsigned int l_NewChildID = m_NodePool.retain(&l_pTargetNode);
		a_pNode->m_Children[l_RelationID] = l_pTargetNode;

		l_pTargetNode->m_ID = l_NewChildID;
		l_pTargetNode->m_RelationID = l_RelationID;
		l_pTargetNode->m_bValid = true;
		l_pTargetNode->m_pParent = a_pNode;
		l_pTargetNode->m_Bounding.m_Size = a_pNode->m_Bounding.m_Size * 0.5;
		l_pTargetNode->m_Bounding.m_Center = a_pNode->m_Bounding.m_Center + l_pTargetNode->m_Bounding.m_Size * c_Multiply[l_RelationID];
	}

	return insertNode(l_pTargetNode, a_Box);
}

void OctreePartition::removeNode(std::shared_ptr<Node> a_pNode)
{
	if( nullptr == a_pNode->m_pParent ) return;// root node

	auto l_pParent = a_pNode->m_pParent;
	
	a_pNode->m_bValid = false;
	a_pNode->m_pParent = nullptr;
	m_NodePool.release(a_pNode->m_ID);

	l_pParent->m_Children[a_pNode->m_RelationID] = nullptr;
	if( l_pParent->isLeaf() && l_pParent->m_Models.empty() ) removeNode(l_pParent);
}

void OctreePartition::assignBoxIntersect(glm::frustumface &a_Frustum, std::shared_ptr<Node> a_pNode, std::vector< std::shared_ptr<RenderableComponent> > &a_Output)
{
	bool l_bInside = false;
	for( auto it = a_pNode->m_Models.begin() ; it != a_pNode->m_Models.end() ; ++it )
	{
		if( a_Frustum.intersect((*it)->boundingBox(), l_bInside) ) a_Output.push_back(*it);
	}

	for( unsigned int i=0 ; i<NUM_OCT_POS ; ++i )
	{
		if( nullptr == a_pNode->m_Children[i] ) continue;
		if( a_Frustum.intersect(a_pNode->m_Children[i]->m_Bounding, l_bInside) )
		{
			if( l_bInside ) assignBoxInside(a_pNode->m_Children[i], a_Output);
			else assignBoxIntersect(a_Frustum, a_pNode->m_Children[i], a_Output);
		}
	}
}

void OctreePartition::assignBoxInside(std::shared_ptr<Node> a_pNode, std::vector< std::shared_ptr<RenderableComponent> > &a_Output)
{
	std::list< std::shared_ptr<Node> > l_NodeList;
	l_NodeList.push_back(a_pNode);
	for( auto it = l_NodeList.begin() ; it != l_NodeList.end() ; ++it )
	{
		for( auto l_ModelIt = (*it)->m_Models.begin() ; l_ModelIt != (*it)->m_Models.end() ; ++l_ModelIt ) a_Output.push_back(*l_ModelIt);
		for( unsigned int i=0 ; i<NUM_OCT_POS ; ++i )
		{
			if( nullptr == (*it)->m_Children[i] ) continue;
			l_NodeList.push_back((*it));
		}
	}
}
#pragma endregion

}