// Camera.h
//
// 2015/01/21 Ian Wu/Real0000
//

#include "CommonUtil.h"
#include "Core.h"
#include "Camera.h"

#include "Scene/Scene.h"
#include "Scene/Graph/ScenePartition.h"
#include "Scene/RenderPipline/RenderPipline.h"

namespace R
{

#pragma region CameraComponent
//
// Camera
//
CameraComponent::CameraComponent(SharedSceneMember *a_pSharedMember, std::shared_ptr<SceneNode> a_pOwner)
	: RenderableComponent(a_pSharedMember, a_pOwner)
	, m_ViewParam(45.0f, 1.0f, 0.1f, 4000.0f), m_Type(PERSPECTIVE)
{
	addTransformListener();
	calView();
}

CameraComponent::~CameraComponent()
{
}

void CameraComponent::start()
{
	auto l_pThis = shared_from_base<CameraComponent>();
	if( !isHidden() ) getSharedMember()->m_pGraphs[SharedSceneMember::GRAPH_CAMERA]->add(l_pThis);
}

void CameraComponent::end()
{
	auto l_pThis = shared_from_base<CameraComponent>();
	if( !isHidden() ) getSharedMember()->m_pGraphs[SharedSceneMember::GRAPH_CAMERA]->remove(l_pThis);
}

void CameraComponent::hiddenFlagChanged()
{
	ScenePartition *l_pGraphOwner = getSharedMember()->m_pGraphs[SharedSceneMember::GRAPH_CAMERA];
	if( isHidden() )
	{
		l_pGraphOwner->remove(shared_from_base<CameraComponent>());
		removeTransformListener();
	}
	else
	{
		l_pGraphOwner->add(shared_from_base<CameraComponent>());
		addTransformListener();
	}
}

void CameraComponent::setOrthoView(float a_Width, float a_Height, float a_Near, float a_Far)
{
	m_ViewParam = glm::vec4(a_Width, a_Height, a_Near, a_Far);
    m_Type = ORTHO;
	calProjection();
}
    
void CameraComponent::setPerspectiveView(float a_Fovy, float a_Aspect, float a_Near, float a_Far)
{
	m_ViewParam = glm::vec4(a_Fovy, a_Aspect, a_Near, a_Far);
    m_Type = PERSPECTIVE;
	calProjection();
}

void CameraComponent::setTetrahedonView(float a_Range)
{
	m_ViewParam = glm::vec4(120.0f, 1.0f, 0.01f, a_Range);
	m_Type = TETRAHEDRON;
	calViewProjection();
}

void CameraComponent::setCubeView(float a_Range)
{
	m_ViewParam = glm::vec4(90.0f, 1.0f, 0.01f, a_Range);
	m_Type = CUBE;
	calViewProjection();
}

void CameraComponent::transformListener(glm::mat4x4 &a_NewTransform)
{
	calView();
	
	SharedSceneMember *l_pMember = getSharedMember();
	auto l_pThis = shared_from_base<CameraComponent>();
	if( !isHidden() )
	{
		boundingBox().m_Center = glm::vec3(a_NewTransform[0][3], a_NewTransform[1][3], a_NewTransform[2][3]);
		if( TETRAHEDRON == m_Type || CUBE == m_Type ) boundingBox().m_Size = glm::vec3(m_ViewParam.w, m_ViewParam.w, m_ViewParam.w);
		else boundingBox().m_Size = glm::one<glm::vec3>();
		l_pMember->m_pGraphs[SharedSceneMember::GRAPH_CAMERA]->update(l_pThis);
	}
}

void CameraComponent::calView()
{
	switch( m_Type )
	{
		case ORTHO:
		case PERSPECTIVE:
			m_Matrices[INVERTVIEW] = getSharedMember()->m_pSceneNode->getTransform();
			m_Matrices[VIEW] = glm::inverse(m_Matrices[INVERTVIEW]);
			break;

		//case TETRAHEDRON:
		//case CUBE:
		default:break;
	}
	calViewProjection();
}

void CameraComponent::calProjection()
{
    switch( m_Type )
	{
		case ORTHO:
			m_Matrices[PROJECTION] = glm::ortho(m_ViewParam.x * -0.5f, m_ViewParam.x * 0.5f, m_ViewParam.y * -0.5f, m_ViewParam.y * 0.5f, m_ViewParam.z, m_ViewParam.w);
			break;

		case PERSPECTIVE:
			m_Matrices[PROJECTION] = glm::perspective(m_ViewParam.x, m_ViewParam.y, m_ViewParam.z, m_ViewParam.w);
			break;

		//case TETRAHEDRON:
		//case CUBE:
		default:break;
	}
	calViewProjection();
}

void CameraComponent::calViewProjection()
{
	switch( m_Type )
	{
		case ORTHO:
		case PERSPECTIVE:{
			m_Matrices[VIEWPROJECTION] = m_Matrices[PROJECTION] * m_Matrices[VIEW];
			m_Frustum.fromViewProjection(m_Matrices[VIEWPROJECTION]);
			}break;

		case TETRAHEDRON:{
			glm::mat4x4 l_World(getSharedMember()->m_pSceneNode->getTransform());
			glm::vec3 l_Eye(l_World[0][3], l_World[1][3], l_World[2][3]);
			glm::mat4x4 l_Projection(glm::perspective(m_ViewParam.x, m_ViewParam.y, m_ViewParam.z, m_ViewParam.w));

			//
			// to do : implement view projection calculate
			//
			
			glm::aabb l_Box(l_Eye, glm::vec3(m_ViewParam.w, m_ViewParam.w, m_ViewParam.w) * 2.0f);
			m_Frustum.fromAABB(l_Box);
			}break;

		case CUBE:{
			glm::mat4x4 l_World(getSharedMember()->m_pSceneNode->getTransform());
			glm::vec3 l_Eye(l_World[0][3], l_World[1][3], l_World[2][3]);
			glm::mat4x4 l_Projection(glm::perspective(m_ViewParam.x, m_ViewParam.y, m_ViewParam.z, m_ViewParam.w));
			m_Matrices[CUBEMAP_POSITIVE_X] = l_Projection * glm::lookAt(l_Eye, l_Eye + glm::vec3(m_ViewParam.w, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			m_Matrices[CUBEMAP_NEGATIVE_X] = l_Projection * glm::lookAt(l_Eye, l_Eye + glm::vec3(-m_ViewParam.w, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			m_Matrices[CUBEMAP_POSITIVE_Y] = l_Projection * glm::lookAt(l_Eye, l_Eye + glm::vec3(0.0f, m_ViewParam.w, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
			m_Matrices[CUBEMAP_NEGATIVE_Y] = l_Projection * glm::lookAt(l_Eye, l_Eye + glm::vec3(0.0f, -m_ViewParam.w, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
			m_Matrices[CUBEMAP_POSITIVE_Z] = l_Projection * glm::lookAt(l_Eye, l_Eye + glm::vec3(0.0f, 0.0f, m_ViewParam.w), glm::vec3(0.0f, 1.0f, 0.0f));
			m_Matrices[CUBEMAP_NEGATIVE_Z] = l_Projection * glm::lookAt(l_Eye, l_Eye + glm::vec3(0.0f, 0.0f, -m_ViewParam.w), glm::vec3(0.0f, 1.0f, 0.0f));
			
			glm::aabb l_Box(l_Eye, glm::vec3(m_ViewParam.w, m_ViewParam.w, m_ViewParam.w) * 2.0f);
			m_Frustum.fromAABB(l_Box);
			}break;

		default:break;
	}
}
#pragma endregion

}
