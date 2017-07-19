// Scene.cpp
//
// 2015/02/10 Ian Wu/Real0000
//

#include "CommonUtil.h"
#include "Core.h"
#include "Scene.h"

namespace R
{

SceneNode::SceneNode()
{

}

SceneNode::~SceneNode()
{
}

void SceneNode::setTransform(glm::mat4x4 a_Transform)
{
	m_LocalTransform = a_Transform;
}

void SceneNode::addChild(SceneNode *a_pNode)
{
	m_Children.push_back(a_pNode);
}

void SceneNode::update(glm::mat4x4 a_ParentTranform)
{
	m_World = a_ParentTranform * m_LocalTransform;
	for( unsigned int i=0 ; i<m_Children.size() ; ++i ) m_Children[i]->update(m_World);
}

std::shared_ptr<EngineComponent> SceneNode::getComponent(wxString a_Name)
{
	if( a_Name.empty() ) return nullptr;
	for( auto it = m_Components.begin() ; it != m_Components.end() ; ++it )
	{
		for( unsigned int i=0 ; i<it->second.size() ; ++i )
		{
			return it->second[i];
		}
	}
	return nullptr;
}

void SceneNode::getComponent(wxString a_Name, std::vector< std::shared_ptr<EngineComponent> > &a_Output)
{
	if( a_Name.empty() ) return;
	for( auto it = m_Components.begin() ; it != m_Components.end() ; ++it )
	{
		for( unsigned int i=0 ; i<it->second.size() ; ++i )
		{
			if( it->second[i]->getName() == a_Name ) a_Output.push_back(it->second[i]);
		}
	}
}

}