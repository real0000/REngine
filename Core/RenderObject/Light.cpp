// OmniLight.cpp
//
// 2017/09/15 Ian Wu/Real0000
//

#include "CommonUtil.h"
#include "Core.h"
#include "Scene/Scene.h"
#include "Scene/Graph/ScenePartition.h"

#include "Light.h"

namespace R
{

#pragma region Light
//
// Light
//
Light::Light(SharedSceneMember *a_pSharedMember, std::shared_ptr<SceneNode> a_pOwner)
	: RenderableComponent(a_pSharedMember, a_pOwner)
	, m_bHidden(false)
{
}

Light::~Light()
{
}

void Light::start()
{
	SharedSceneMember *l_pMembers = getSharedMember();
	ScenePartition *l_pGraphOwner = l_pMembers->m_pSceneNode->isStatic() ? l_pMembers->m_pGraphs[SharedSceneMember::GRAPH_STATIC_LIGHT] : l_pMembers->m_pGraphs[SharedSceneMember::GRAPH_DYNAMIC_LIGHT];
	l_pGraphOwner->add(shared_from_base<Light>());
}

void Light::end()
{
	if( m_bHidden ) return;

	SharedSceneMember *l_pMembers = getSharedMember();
	ScenePartition *l_pGraphOwner = l_pMembers->m_pSceneNode->isStatic() ? l_pMembers->m_pGraphs[SharedSceneMember::GRAPH_STATIC_LIGHT] : l_pMembers->m_pGraphs[SharedSceneMember::GRAPH_DYNAMIC_LIGHT];
	std::shared_ptr<Light> l_pThis = shared_from_base<Light>();

	l_pGraphOwner->remove(l_pThis);
}

void Light::staticFlagChanged()
{
	if( m_bHidden ) return;

	SharedSceneMember *l_pMembers = getSharedMember();
	std::pair<ScenePartition *, ScenePartition *> l_pOldNew = l_pMembers->m_pSceneNode->isStatic() ?
		std::make_pair(l_pMembers->m_pGraphs[SharedSceneMember::GRAPH_DYNAMIC_LIGHT], l_pMembers->m_pGraphs[SharedSceneMember::GRAPH_STATIC_LIGHT]) :
		std::make_pair(l_pMembers->m_pGraphs[SharedSceneMember::GRAPH_STATIC_LIGHT], l_pMembers->m_pGraphs[SharedSceneMember::GRAPH_DYNAMIC_LIGHT]);

	std::shared_ptr<Light> l_pThis = shared_from_base<Light>();
	l_pOldNew.first->remove(l_pThis);
	l_pOldNew.second->add(l_pThis);
}

void Light::transformListener(glm::mat4x4 &a_NewTransform)
{
	SharedSceneMember *l_pMembers = getSharedMember();
	ScenePartition *l_pGraphOwner = l_pMembers->m_pSceneNode->isStatic() ? l_pMembers->m_pGraphs[SharedSceneMember::GRAPH_STATIC_LIGHT] : l_pMembers->m_pGraphs[SharedSceneMember::GRAPH_DYNAMIC_LIGHT];

	l_pGraphOwner->update(shared_from_base<Light>());
}

void Light::setHidden(bool a_bHidden)
{
	if( m_bHidden == a_bHidden ) return;
	m_bHidden = a_bHidden;
	SharedSceneMember *l_pMembers = getSharedMember();
	ScenePartition *l_pGraphOwner = l_pMembers->m_pSceneNode->isStatic() ? l_pMembers->m_pGraphs[SharedSceneMember::GRAPH_STATIC_LIGHT] : l_pMembers->m_pGraphs[SharedSceneMember::GRAPH_DYNAMIC_LIGHT];
	if( m_bHidden )
	{
		l_pGraphOwner->remove(shared_from_base<Light>());
		removeTransformListener();
	}
	else
	{
		l_pGraphOwner->add(shared_from_base<Light>());
		addTransformListener();
	}
}
#pragma endregion

}