// Camera.h
//
// 2015/01/21 Ian Wu/Real0000
//

#include "CommonUtil.h"
#include "Camera.h"

namespace R
{

#pragma region Camera
//
// Camera
//
Camera::Camera()
	: SceneNode()
	, m_Eye(25.0f, 25.0f, 25.0f), m_LookAt(0.0f, 0.0f, 0.0f), m_Up(0.0f, 1.0f, 0.0f)
	, m_ViewSize(400.0f, 400.0f), m_PersParam(45.0f, 1.0f, 0.1f, 10000.0f), m_Zoom(1.0f), m_bOrtho(false)
{
}

Camera::~Camera()
{
}

void Camera::setEyeCenter(glm::vec3 l_Eye, glm::vec3 l_Center)
{
	m_Eye = l_Eye;
	m_LookAt = l_Center;
	calView();
}

void Camera::setEye(glm::vec3 _Eye)
{
	m_Eye = _Eye;
	calView();
}

void Camera::setLookAt(glm::vec3 l_LookAt)
{
	m_LookAt = l_LookAt;
	calView();
}

void Camera::setUp(glm::vec3 _Up)
{
	m_Up = _Up;
	calView();
}

void Camera::setZoom(float l_Zoom)
{
	m_Zoom = glm::clamp(l_Zoom, 0.1f, 10.0f);
	calProjection();
}

void Camera::setOrthoView(float _Width, float _Height)
{
	m_ViewSize.x = _Width;
	m_ViewSize.y = _Height;
    m_bOrtho = true;
	calProjection();
}
    
void Camera::setPerspectiveView(float _Fovy, float _Aspect, float _Near, float _Far)
{
    m_PersParam.x = _Fovy;
    m_PersParam.y = _Aspect;
    m_PersParam.z = _Near;
    m_PersParam.w = _Far;
    m_bOrtho = false;
	calProjection();
}

void Camera::calView()
{
	m_Matrices[VIEW] = glm::lookAt(m_Eye, m_LookAt, m_Up);
	m_Matrices[INVERTVIEW] = glm::inverse(m_Matrices[VIEW]);
	calViewProjection();
}

void Camera::calProjection()
{
    if( m_bOrtho )
        m_Matrices[PROJECTION] = glm::ortho(m_ViewSize.x / m_Zoom * -0.5f, m_ViewSize.x / m_Zoom * 0.5f,
                                                  m_ViewSize.y / m_Zoom * -0.5f, m_ViewSize.y / m_Zoom * 0.5f, m_PersParam.z, m_PersParam.w);
    else m_Matrices[PROJECTION] = glm::perspective(m_PersParam.x, m_PersParam.y, m_PersParam.z, m_PersParam.w);
	calViewProjection();
}

void Camera::calViewProjection()
{
	m_Matrices[PROJECTION] = m_Matrices[PROJECTION] * m_Matrices[VIEW];
}
#pragma endregion

}
