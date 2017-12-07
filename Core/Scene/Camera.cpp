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
	if( nullptr != a_pOwner ) calView(a_pOwner->getTransform());
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
	if( isHidden() )
	{
		getSharedMember()->m_pGraphs[SharedSceneMember::GRAPH_CAMERA]->remove(shared_from_base<CameraComponent>());
		removeTransformListener();
	}
	else
	{
		getSharedMember()->m_pGraphs[SharedSceneMember::GRAPH_CAMERA]->add(shared_from_base<CameraComponent>());
		addTransformListener();
	}
}

void CameraComponent::setOrthoView(float a_Width, float a_Height, float a_Near, float a_Far, glm::mat4x4 &a_Transform)
{
	m_ViewParam = glm::vec4(a_Width, a_Height, a_Near, a_Far);
    m_Type = ORTHO;
	calProjection(a_Transform);
}
    
void CameraComponent::setPerspectiveView(float a_Fovy, float a_Aspect, float a_Near, float a_Far, glm::mat4x4 &a_Transform)
{
	m_ViewParam = glm::vec4(a_Fovy, a_Aspect, a_Near, a_Far);
    m_Type = PERSPECTIVE;
	calProjection(a_Transform);
}

void CameraComponent::setTetrahedonView(glm::mat4x4 &a_Transform)
{
	m_Type = TETRAHEDRON;
	calViewProjection(a_Transform);
}

void CameraComponent::setCubeView(glm::mat4x4 &a_Transform)
{
	m_Type = CUBE;
	calViewProjection(a_Transform);
}

void CameraComponent::getCameraParam(glm::vec3 &a_Eye, glm::vec3 &a_Dir, glm::vec3 &a_Up)
{
	assert(m_Type == ORTHO || m_Type == PERSPECTIVE);
	a_Eye = glm::vec3(m_Matrices[VIEW][0][3], m_Matrices[VIEW][1][3], m_Matrices[VIEW][2][3]);
	a_Dir = glm::normalize(glm::vec3(m_Matrices[VIEW][0][2], m_Matrices[VIEW][1][2], m_Matrices[VIEW][2][2]));
	a_Up  = glm::normalize(glm::vec3(m_Matrices[VIEW][0][1], m_Matrices[VIEW][1][1], m_Matrices[VIEW][2][1]));
}

void CameraComponent::transformListener(glm::mat4x4 &a_NewTransform)
{
	calView(a_NewTransform);
	
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

void CameraComponent::calView(glm::mat4x4 &a_NewTransform)
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
	calViewProjection(a_NewTransform);
}

void CameraComponent::calProjection(glm::mat4x4 &a_NewTransform)
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
	calViewProjection(a_NewTransform);
}

void CameraComponent::calViewProjection(glm::mat4x4 &a_NewTransform)
{
	switch( m_Type )
	{
		case ORTHO:
		case PERSPECTIVE:{
			m_Matrices[VIEWPROJECTION] = m_Matrices[PROJECTION] * m_Matrices[VIEW];
			m_Frustum.fromViewProjection(m_Matrices[VIEWPROJECTION]);
			}break;

		case TETRAHEDRON:{
			glm::vec3 l_Eye, l_Scale;
			glm::quat l_Rot;
			decomposeTRS(a_NewTransform, l_Eye, l_Scale, l_Rot);

			m_ViewParam = glm::vec4(120.0f, 1.0f, 0.01f, std::max(std::max(l_Scale.x, l_Scale.y), l_Scale.z));
			(a_NewTransform[0][3], a_NewTransform[1][3], a_NewTransform[2][3]);
			glm::mat4x4 l_Projection(glm::perspective(m_ViewParam.x, m_ViewParam.y, m_ViewParam.z, m_ViewParam.w));

			//
			// to do : implement view projection calculate
			//
			
			glm::aabb l_Box(l_Eye, glm::vec3(m_ViewParam.w, m_ViewParam.w, m_ViewParam.w) * 2.0f);
			m_Frustum.fromAABB(l_Box);
			}break;

		case CUBE:{
			glm::vec3 l_Eye, l_Scale;
			glm::quat l_Rot;
			decomposeTRS(a_NewTransform, l_Eye, l_Scale, l_Rot);
			
			m_ViewParam = glm::vec4(90.0f, 1.0f, 0.01f, std::max(std::max(l_Scale.x, l_Scale.y), l_Scale.z));
			glm::mat4x4 l_Projection(glm::perspective(m_ViewParam.x, m_ViewParam.y, m_ViewParam.z, m_ViewParam.w));
			
			m_Matrices[CUBEMAP_POSITIVE_X] = l_Projection * glm::lookAt(l_Eye, glm::vec3(l_Eye.x + m_ViewParam.w, l_Eye.y, l_Eye.z), glm::vec3(0.0f, 1.0f, 0.0f));
			m_Matrices[CUBEMAP_NEGATIVE_X] = l_Projection * glm::lookAt(l_Eye, glm::vec3(l_Eye.x - m_ViewParam.w, l_Eye.y, l_Eye.z), glm::vec3(0.0f, 1.0f, 0.0f));
			m_Matrices[CUBEMAP_POSITIVE_Y] = l_Projection * glm::lookAt(l_Eye, glm::vec3(l_Eye.x, l_Eye.y + m_ViewParam.w, l_Eye.z), glm::vec3(0.0f, 0.0f, -1.0f));
			m_Matrices[CUBEMAP_NEGATIVE_Y] = l_Projection * glm::lookAt(l_Eye, glm::vec3(l_Eye.x, l_Eye.y - m_ViewParam.w, l_Eye.z), glm::vec3(0.0f, 0.0f, 1.0f));
			m_Matrices[CUBEMAP_POSITIVE_Z] = l_Projection * glm::lookAt(l_Eye, glm::vec3(l_Eye.x, l_Eye.y, l_Eye.z + m_ViewParam.w), glm::vec3(0.0f, 1.0f, 0.0f));
			m_Matrices[CUBEMAP_NEGATIVE_Z] = l_Projection * glm::lookAt(l_Eye, glm::vec3(l_Eye.x, l_Eye.y, l_Eye.z - m_ViewParam.w), glm::vec3(0.0f, 1.0f, 0.0f));
			
			glm::aabb l_Box(l_Eye, glm::vec3(m_ViewParam.w, m_ViewParam.w, m_ViewParam.w) * 2.0f);
			m_Frustum.fromAABB(l_Box);
			}break;

		default:break;
	}
}
#pragma endregion

}
