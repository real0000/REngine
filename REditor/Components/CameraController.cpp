// CameraController.cpp
//
// 2020/12/21 Ian Wu/Real0000
//

#include "REngine.h"
#include "REditor.h"

namespace R
{

#pragma region CameraController
//
// CameraController
//
CameraController::CameraController(std::shared_ptr<Scene> a_pRefScene, std::shared_ptr<SceneNode> a_pNode)
	: EngineComponent(a_pRefScene, a_pNode)
	, m_bValid(false)
	, m_MousePt(0.0f, 0.0f)
	, m_MaxSpeed(10.0f)
	, m_MovingSpeed(0.0f)
	, m_MovingFlag(0, 0, 0)
{
}

CameraController::~CameraController()
{
}

void CameraController::start()
{
	addInputListener();
	addUpdateListener();
}

void CameraController::loadComponent(boost::property_tree::ptree &a_Src)
{
}

void CameraController::saveComponent(boost::property_tree::ptree &a_Dst)
{
}

bool CameraController::inputListener(InputData &a_Input)
{
	if( KEYBOARD_DEVICE == a_Input.m_DeviceName )
	{
		if( !m_bValid ) return false;

		switch( a_Input.m_Key )
		{
			case SDLK_w: m_MovingFlag.z = a_Input.m_Data.m_bDown ? -1 : 0; break;
			case SDLK_s: m_MovingFlag.z = a_Input.m_Data.m_bDown ? 1 : 0;break;
			case SDLK_a: m_MovingFlag.x = a_Input.m_Data.m_bDown ? -1 : 0;break;
			case SDLK_d: m_MovingFlag.x = a_Input.m_Data.m_bDown ? 1 : 0; break;
			case SDLK_q: m_MovingFlag.y = a_Input.m_Data.m_bDown ? 1 : 0; break;
			case SDLK_e: m_MovingFlag.y = a_Input.m_Data.m_bDown ? -1 : 0;break;
			default: return false;
		}
		if( m_MovingFlag == glm::zero<glm::ivec3>() ) m_MovingSpeed = 0.0f;
	}
	else if( MOUSE_DEVICE == a_Input.m_DeviceName )
	{
		switch( a_Input.m_Key )
		{
			case InputMediator::MOUSE_RIGHT_BUTTON:{
				a_Input.m_Data.m_bDown ? mouseDown() : mouseUp();
				}return true;

			case InputMediator::MOUSE_MOVE:{
				m_MousePt.x = a_Input.m_Data.m_Val[0];
				m_MousePt.y = a_Input.m_Data.m_Val[1];
				if( !m_bValid ) return false;
				mouseMove();
				}return true;

			default: return false;
		}
	}

	return false;
}

void CameraController::updateListener(float a_Delta)
{
	if( m_MovingFlag == glm::zero<glm::ivec3>() ) return;
	
	std::vector<std::shared_ptr<Camera>> l_Cameras;
	getOwner()->getComponents<Camera>(l_Cameras);
	if( l_Cameras.empty() ) return;

	glm::vec3 l_Eye, l_Dir, l_Up;
	l_Cameras[0]->getCameraParam(l_Eye, l_Dir, l_Up);
	glm::vec3 l_Left(glm::normalize(glm::cross(l_Up, l_Dir)));

	m_MovingSpeed += (m_MaxSpeed / CAMERA_SPEEDUP_DURATION) * a_Delta;
	if( m_MovingSpeed > m_MaxSpeed ) m_MovingSpeed = m_MaxSpeed;

	l_Eye += float(m_MovingFlag.x) * l_Left * m_MovingSpeed * a_Delta;
	l_Eye += float(m_MovingFlag.y) * l_Up * m_MovingSpeed * a_Delta;
	l_Eye += float(m_MovingFlag.z) * l_Dir * m_MovingSpeed * a_Delta;

	glm::vec4 l_LocalCast(l_Eye.x, l_Eye.y, l_Eye.z, 1.0f);
	l_LocalCast = glm::inverse(getOwner()->getParent()->getTransform()) * l_LocalCast;
	l_Eye = glm::vec3(l_LocalCast.x, l_LocalCast.y, l_LocalCast.z);
	
	getOwner()->setPosition(l_Eye);
}

void CameraController::mouseDown()
{
	m_bValid = true;
	m_PrevPt = m_MousePt;
}

void CameraController::mouseMove()
{
	std::vector<std::shared_ptr<Camera>> l_Cameras;
	getOwner()->getComponents<Camera>(l_Cameras);
	if( l_Cameras.empty() ) return;

	glm::vec2 l_Sub(m_PrevPt - m_MousePt);
	m_PrevPt = m_MousePt;
	
	glm::vec3 l_Eye, l_Dir, l_Up;
	l_Cameras[0]->getCameraParam(l_Eye, l_Dir, l_Up);

	l_Dir = -l_Dir;
	float l_PI = glm::pi<float>();
	float l_Alpha = std::atan2(l_Dir.z, l_Dir.x) + l_PI - l_Sub.x / 100.0f;
	l_Alpha = glm::fract(l_Alpha / (2.0f * l_PI)) * l_PI * 2.0f;
	float l_Beta = std::asin(l_Dir.y) + l_Sub.y / 100.0f;
	l_Beta = glm::clamp(l_Beta, -l_PI * 0.5f + 0.01f, l_PI * 0.5f - 0.01f);

	glm::mat4x4 l_Rotate(glm::eulerAngleYZ(-l_Alpha, -l_Beta));
	l_Rotate = glm::inverse(getOwner()->getParent()->getTransform()) * l_Rotate;
	getOwner()->setRotate(l_Rotate);
}

void CameraController::mouseUp()
{
	m_bValid = false;
	m_MovingFlag = glm::ivec3(0, 0, 0);
}
#pragma endregion	

}