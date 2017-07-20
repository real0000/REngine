// Camera.h
//
// 2015/01/21 Ian Wu/Real0000
//

#include "CommonUtil.h"
#include "Core.h"
#include "Camera.h"

namespace R
{

#pragma region CameraComponent
//
// Camera
//
CameraComponent::CameraComponent(std::shared_ptr<SceneNode> a_pOwner)
	: EngineComponent(a_pOwner)
	, m_Eye(25.0f, 25.0f, 25.0f), m_LookAt(0.0f, 0.0f, 0.0f), m_Up(0.0f, 1.0f, 0.0f)
	, m_ViewParam(45.0f, 1.0f, 0.1f, 4000.0f), m_bOrtho(false)
{
}

CameraComponent::~CameraComponent()
{
}

void CameraComponent::setEyeCenter(glm::vec3 a_Eye, glm::vec3 a_Center)
{
	m_Eye = a_Eye;
	m_LookAt = a_Center;
	calView();
}

void CameraComponent::setEye(glm::vec3 a_Eye)
{
	m_Eye = a_Eye;
	calView();
}

void CameraComponent::setLookAt(glm::vec3 a_LookAt)
{
	m_LookAt = a_LookAt;
	calView();
}

void CameraComponent::setUp(glm::vec3 a_Up)
{
	m_Up = a_Up;
	calView();
}

void CameraComponent::setOrthoView(float a_Width, float a_Height, float a_Near, float a_Far)
{
	m_ViewParam = glm::vec4(a_Width, a_Height, a_Near, a_Far);
    m_bOrtho = true;
	calProjection();
}
    
void CameraComponent::setPerspectiveView(float a_Fovy, float a_Aspect, float a_Near, float a_Far)
{
	m_ViewParam = glm::vec4(a_Fovy, a_Aspect, a_Near, a_Far);
    m_bOrtho = false;
	calProjection();
}

void CameraComponent::calView()
{
	m_Matrices[VIEW] = glm::lookAt(m_Eye, m_LookAt, m_Up);
	m_Matrices[INVERTVIEW] = glm::inverse(m_Matrices[VIEW]);
	calViewProjection();
}

void CameraComponent::calProjection()
{
    if( m_bOrtho )
        m_Matrices[PROJECTION] = glm::ortho(m_ViewParam.x * -0.5f, m_ViewParam.x * 0.5f,
											m_ViewParam.y * -0.5f, m_ViewParam.y * 0.5f, m_ViewParam.z, m_ViewParam.w);
    else m_Matrices[PROJECTION] = glm::perspective(m_ViewParam.x, m_ViewParam.y, m_ViewParam.z, m_ViewParam.w);
	calViewProjection();
}

void CameraComponent::calViewProjection()
{
	m_Matrices[PROJECTION] = m_Matrices[PROJECTION] * m_Matrices[VIEW];
}
#pragma endregion

}
