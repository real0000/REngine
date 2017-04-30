// SceneNode.cpp
//
// 2015/01/24 Ian Wu/Real0000
//

#include "CommonUtil.h"
#include "SceneNode.h"

namespace R
{

SceneNode::SceneNode()
	: m_pBoundingGeometry(nullptr)
{

}

SceneNode::~SceneNode()
{
	if( nullptr != m_pBoundingGeometry ) delete m_pBoundingGeometry;
}

void SceneNode::setPosition(glm::vec3 l_Position)
{
	m_LocalTransform[3] = glm::vec4(l_Position.x, l_Position.y, l_Position.z, 1.0f);
}

void SceneNode::setRotation(glm::quat l_Rotation)
{
	glm::vec3 l_OriginScale(glm::length(m_LocalTransform[0]), glm::length(m_LocalTransform[1]), glm::length(m_LocalTransform[2]));
	glm::mat3x3 l_RotMatrix(glm::toMat3(l_Rotation));
	m_LocalTransform[0] = glm::vec4(l_OriginScale.x * l_RotMatrix[0].x, l_OriginScale.x * l_RotMatrix[0].y, l_OriginScale.x * l_RotMatrix[0].z, 0.0f);
	m_LocalTransform[0] = glm::vec4(l_OriginScale.y * l_RotMatrix[1].x, l_OriginScale.y * l_RotMatrix[1].y, l_OriginScale.y * l_RotMatrix[1].z, 0.0f);
	m_LocalTransform[0] = glm::vec4(l_OriginScale.z * l_RotMatrix[2].x, l_OriginScale.z * l_RotMatrix[2].y, l_OriginScale.z * l_RotMatrix[2].z, 0.0f);
}

void SceneNode::setScale(glm::vec3 l_Scale)
{
	for( unsigned int i=0 ; i<3 ; ++i )
	{
		glm::vec3 l_Axis(m_LocalTransform[i].x, m_LocalTransform[i].y, m_LocalTransform[i].z);
		l_Axis = glm::normalize(l_Axis);
		m_LocalTransform[i].x = l_Axis.x * l_Scale[i];
		m_LocalTransform[i].y = l_Axis.y * l_Scale[i];
		m_LocalTransform[i].z = l_Axis.z * l_Scale[i];
	}
}

void SceneNode::setTransform(glm::mat4x4 l_Transform)
{
	m_LocalTransform = l_Transform;
}

void SceneNode::addChild(SceneNode *l_pNode)
{
	m_Children.push_back(l_pNode);
}

void SceneNode::update(glm::mat4x4 l_ParentTranform)
{
	m_World = l_ParentTranform * m_LocalTransform;
}

}