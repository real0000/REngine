// Camera.h
//
// 2015/01/21 Ian Wu/Real0000
//

#include "CommonUtil.h"
#include "RGDeviceWrapper.h"
#include "Core.h"
#include "Camera.h"

#include "Asset/MaterialAsset.h"
#include "Physical/IntersectHelper.h"
#include "Physical/PhysicalModule.h"
#include "Scene/Scene.h"
#include "Scene/RenderPipline/RenderPipline.h"

namespace R
{

#pragma region Camera
//
// Camera
//
Camera::Camera(std::shared_ptr<Scene> a_pRefScene, std::shared_ptr<SceneNode> a_pOwner)
	: RenderableComponent(a_pRefScene, a_pOwner)
	, m_ViewParam(glm::pi<float>() / 3.0f, 16.0f / 9.0f, 0.1f, 10000.0f), m_Type(PERSPECTIVE)
	, m_pCameraBlock(nullptr)
	, m_pHelper(nullptr)
{
	std::shared_ptr<ShaderProgram> l_pProgram = ProgramManager::singleton().getData(DefaultPrograms::Standard);

	auto l_BlockDescVec = l_pProgram->getBlockDesc(ShaderRegType::ConstBuffer);
	auto l_BlockDescIt = std::find_if(l_BlockDescVec.begin(), l_BlockDescVec.end(), [=](ProgramBlockDesc *a_pBlock) -> bool { return a_pBlock->m_StructureName == "CameraBuffer"; });
	m_pCameraBlock = MaterialBlock::create(ShaderRegType::ConstBuffer, *l_BlockDescIt);
	
	m_Matrices[PROJECTION] = glm::tweakedInfinitePerspective(m_ViewParam.x, m_ViewParam.y, m_ViewParam.z);
	
	m_pCameraBlock->setParam("m_Projection", 0, m_Matrices[PROJECTION]);
	if( nullptr != a_pOwner )
	{
		calView(a_pOwner->getTransform());
		m_Matrices[WORLD] = a_pOwner->getTransform();
	}
	else m_Matrices[WORLD] = glm::identity<glm::mat4x4>();
	m_pCameraBlock->setParam("m_CamWorld", 0, m_Matrices[WORLD]);
}

Camera::~Camera()
{
	m_pCameraBlock = nullptr;
}

void Camera::postInit()
{
	RenderableComponent::postInit();
	m_pHelper = new IntersectHelper(shared_from_base<EngineComponent>(), 
		[=](PhysicalListener *a_pListener) -> int
		{
			glm::mat4x4 l_World(getOwner()->getTransform());
			l_World = l_World * glm::rotate(glm::identity<glm::mat4x4>(), glm::pi<float>(), glm::vec3(l_World[2][0], l_World[2][1], l_World[2][2]));
			switch( m_Type )
			{
				case PERSPECTIVE:{
					float l_Radius = m_ViewParam.x;
					if( m_ViewParam.y > 1.0f ) l_Radius *= m_ViewParam.y;
					l_Radius = tan(0.5f * l_Radius) * m_ViewParam.w * 2.0f;
					return getScene()->getPhysicalWorld()->createTrigger(a_pListener, l_Radius, m_ViewParam.w, l_World, TRIGGER_CAMERA, TRIGGER_LIGHT | TRIGGER_MESH);
					}

				case ORTHO:{
					glm::obb l_Box;
					l_Box.m_Size = glm::vec3(m_ViewParam.x, m_ViewParam.y, m_ViewParam.w);
					l_Box.m_Transition = l_World;
					return getScene()->getPhysicalWorld()->createTrigger(a_pListener, l_Box, TRIGGER_CAMERA, TRIGGER_LIGHT | TRIGGER_MESH);
					}

				default:
					assert(false && "invalid camera type");
					break;
			}
			return -1;
		},
		[=]() -> glm::mat4x4
		{
			glm::mat4x4 l_World(getOwner()->getTransform());
			l_World = l_World * glm::rotate(glm::identity<glm::mat4x4>(), glm::pi<float>(), glm::vec3(l_World[2][0], l_World[2][1], l_World[2][2]));
			return l_World;
		}, TRIGGER_LIGHT | TRIGGER_MESH);
	
	if( !isHidden() )
	{
		m_pHelper->setupTrigger(true);
		addTransformListener();
	}
}

void Camera::preEnd()
{
	SAFE_DELETE(m_pHelper)
}

void Camera::hiddenFlagChanged()
{
	if( isHidden() ) m_pHelper->removeTrigger();
	else m_pHelper->setupTrigger(true);
}

void Camera::loadComponent(boost::property_tree::ptree &a_Src)
{
}

void Camera::saveComponent(boost::property_tree::ptree &a_Dst)
{
}

void Camera::setOrthoView(float a_Width, float a_Height, float a_Near, float a_Far, glm::mat4x4 a_Transform)
{
	m_ViewParam = glm::vec4(a_Width, a_Height, a_Near, a_Far);
	m_pCameraBlock->setParam("m_CameraParam", 0, m_ViewParam);

    m_Type = ORTHO;
	calProjection(a_Transform);
	
	m_pHelper->setupTrigger(true);
}
    
void Camera::setPerspectiveView(float a_Fovy, float a_Aspect, float a_Near, glm::mat4x4 a_Transform)
{
	m_ViewParam = glm::vec4(a_Fovy, a_Aspect, a_Near, m_ViewParam.w);
	m_pCameraBlock->setParam("m_CameraParam", 0, m_ViewParam);
    m_Type = PERSPECTIVE;
	calProjection(a_Transform);
	
	m_pHelper->setupTrigger(true);
}

void Camera::getCameraParam(glm::vec3 &a_Eye, glm::vec3 &a_Dir, glm::vec3 &a_Up)
{
	assert(m_Type == ORTHO || m_Type == PERSPECTIVE);

	glm::mat4x4 l_World(getOwner()->getTransform());
	a_Eye = glm::vec3(l_World[3][0], l_World[3][1], l_World[3][2]);
	a_Dir = glm::normalize(glm::vec3(l_World[0][0], l_World[0][1], l_World[0][2]));
	a_Up  = glm::normalize(glm::vec3(l_World[1][0], l_World[1][1], l_World[1][2]));
}

void Camera::transformListener(const glm::mat4x4 &a_NewTransform)
{
	calView(a_NewTransform);
	if( !isHidden() )
	{
		m_Matrices[WORLD] = getOwner()->getTransform();
		m_pCameraBlock->setParam("m_CamWorld", 0, m_Matrices[WORLD]);
		
		m_pHelper->setupTrigger(false);
	}
}

void Camera::calView(const glm::mat4x4 &a_NewTransform)
{
	switch( m_Type )
	{
		case ORTHO:
		case PERSPECTIVE:{
			glm::vec3 l_Eye, l_Dir, l_Up;
			getCameraParam(l_Eye, l_Dir, l_Up);
			m_Matrices[VIEW] = glm::lookAt(l_Eye, l_Eye + l_Dir * 10.0f, l_Up);
			m_Matrices[INVERTVIEW] = glm::inverse(m_Matrices[VIEW]);

			m_pCameraBlock->setParam("m_View", 0, m_Matrices[VIEW]);
			m_pCameraBlock->setParam("m_InvView", 0, m_Matrices[INVERTVIEW]);
			}break;

		default:break;
	}
	calViewProjection(a_NewTransform);
}

void Camera::calProjection(const glm::mat4x4 &a_NewTransform)
{
    switch( m_Type )
	{
		case ORTHO:
			m_Matrices[PROJECTION] = glm::ortho(m_ViewParam.x * -0.5f, m_ViewParam.x * 0.5f, m_ViewParam.y * -0.5f, m_ViewParam.y * 0.5f, m_ViewParam.z, m_ViewParam.w);
			m_pCameraBlock->setParam("m_Projection", 0, m_Matrices[PROJECTION]);
			break;

		case PERSPECTIVE:
			m_Matrices[PROJECTION] = glm::tweakedInfinitePerspective(m_ViewParam.x, m_ViewParam.y, m_ViewParam.z);
			m_pCameraBlock->setParam("m_Projection", 0, m_Matrices[PROJECTION]);
			break;

		default:break;
	}
	calViewProjection(a_NewTransform);
}

void Camera::calViewProjection(const glm::mat4x4 &a_NewTransform)
{
	m_Matrices[VIEWPROJECTION] = m_Matrices[PROJECTION] * m_Matrices[VIEW];
	m_Matrices[INVERTVIEWPROJECTION] = glm::inverse(m_Matrices[VIEWPROJECTION]);
	m_Frustum.fromViewProjection(m_Matrices[VIEWPROJECTION]);

	m_pCameraBlock->setParam("m_ViewProjection", 0, m_Matrices[VIEWPROJECTION]);
	m_pCameraBlock->setParam("m_InvViewProjection", 0, m_Matrices[INVERTVIEWPROJECTION]);
}
#pragma endregion

}
