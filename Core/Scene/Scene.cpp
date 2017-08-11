// Scene.cpp
//
// 2015/02/10 Ian Wu/Real0000
//

#include "CommonUtil.h"
#include "Core.h"
#include "Scene.h"

namespace R
{

SceneNode::SceneNode(wxString a_Name)
	: m_Name(a_Name)
	, m_pParent(nullptr)
{

}

SceneNode::~SceneNode()
{
}

void SceneNode::setTransform(glm::mat4x4 a_Transform)
{
	m_LocalTransform = a_Transform;
}

std::shared_ptr<SceneNode> SceneNode::addChild()
{
	std::shared_ptr<SceneNode> l_pNewNode = std::shared_ptr<SceneNode>(new SceneNode());
	m_Children.push_back(l_pNewNode);
	l_pNewNode->m_pParent = shared_from_this();
	return l_pNewNode;
}

void SceneNode::destroy()
{
	m_pParent = nullptr;
	for( auto it = m_Components.begin() ; it != m_Components.end() ; ++it )
	{
		for( auto setIt = it->second.begin() ; setIt != it->second.end() ; ++setIt )
		{
			(*setIt)->detach();
		}
		it->second.clear();
	}
	m_Components.clear();
	for( auto it=m_Children.begin() ; it!=m_Children.end() ; ++it ) (*it)->destroy();
	m_Children.clear();
}

void SceneNode::setParent(std::shared_ptr<SceneNode> a_pNewParent)
{
	if( a_pNewParent == m_pParent ) return;
	assert( nullptr != a_pNewParent );
	assert( nullptr != m_pParent );// don't do this to root node
	
	auto it = m_pParent->m_Children.begin();
	for( ; it != m_pParent->m_Children.end() ; ++it )
	{
		if( (*it).get() == this )
		{
			m_pParent->m_Children.erase(it);
			break;
		}
	}

	m_pParent = a_pNewParent;
	m_pParent->m_Children.push_back(shared_from_this());
	update(m_pParent->m_World);
}

std::shared_ptr<SceneNode> SceneNode::find(wxString a_Name)
{
	if( a_Name == m_Name ) return shared_from_this();

	std::shared_ptr<SceneNode> l_Res = nullptr;
	for( auto it = m_Children.begin() ; it != m_Children.end() ; ++it )
	{
		l_Res = (*it)->find(a_Name);
		if( nullptr != l_Res ) break;
	}

	return l_Res;
}

void SceneNode::update(glm::mat4x4 a_ParentTranform)
{
	m_World = a_ParentTranform * m_LocalTransform;
	for( auto it=m_Children.begin() ; it!=m_Children.end() ; ++it ) (*it)->update(m_World);
}

std::shared_ptr<EngineComponent> SceneNode::getComponent(wxString a_Name)
{
	if( a_Name.empty() ) return nullptr;
	for( auto it = m_Components.begin() ; it != m_Components.end() ; ++it )
	{
		for( auto setIt = it->second.begin() ; setIt != it->second.end() ; ++setIt )
		{
			if( (*setIt)->getName() == a_Name ) return *setIt;
		}
	}
	return nullptr;
}

void SceneNode::getComponent(wxString a_Name, std::vector< std::shared_ptr<EngineComponent> > &a_Output)
{
	if( a_Name.empty() ) return;
	for( auto it = m_Components.begin() ; it != m_Components.end() ; ++it )
	{
		for( auto setIt = it->second.begin() ; setIt != it->second.end() ; ++setIt )
		{
			if( (*setIt)->getName() == a_Name ) a_Output.push_back(*setIt);
		}
	}
}

void SceneNode::add(std::shared_ptr<EngineComponent> a_pComponent)
{
	auto l_TypeIt = m_Components.find(a_pComponent->typeID());
	std::set< std::shared_ptr<EngineComponent> > &l_TargetSet = m_Components.end() == l_TypeIt ? 
		(m_Components[a_pComponent->typeID()] = std::set< std::shared_ptr<EngineComponent> >()) :
		l_TypeIt->second;
	l_TargetSet.insert(a_pComponent);
}

void SceneNode::remove(std::shared_ptr<EngineComponent> a_pComponent)
{
	auto l_TypeIt = m_Components.find(a_pComponent->typeID());
	assert(m_Components.end() != l_TypeIt);
	auto l_ComponentIt = std::find(l_TypeIt->second.begin(), l_TypeIt->second.end(), a_pComponent);
	assert(l_TypeIt->second.end() != l_ComponentIt);
	l_TypeIt->second.erase(l_ComponentIt);
	if( l_TypeIt->second.empty() ) m_Components.erase(l_TypeIt);
}

}