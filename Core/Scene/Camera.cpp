// Camera.h
//
// 2015/01/21 Ian Wu/Real0000
//

#include "CommonUtil.h"
#include "RGDeviceWrapper.h"
#include "Core.h"
#include "Camera.h"

#include "Asset/MaterialAsset.h"
#include "Scene/Scene.h"
#include "Scene/Graph/ScenePartition.h"
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
{
	std::shared_ptr<ShaderProgram> l_pProgram = ProgramManager::singleton().getData(DefaultPrograms::Standard);

	auto l_BlockDescVec = l_pProgram->getBlockDesc(ShaderRegType::ConstBuffer);
	auto l_BlockDescIt = std::find_if(l_BlockDescVec.begin(), l_BlockDescVec.end(), [=](ProgramBlockDesc *a_pBlock) -> bool { return a_pBlock->m_StructureName == "CameraBuffer"; });
	m_pCameraBlock = MaterialBlock::create(ShaderRegType::ConstBuffer, *l_BlockDescIt);
	
	m_Matrices[PROJECTION] = glm::tweakedInfinitePerspective(m_ViewParam.x, m_ViewParam.y, m_ViewParam.z);
	m_pCameraBlock->setParam("m_Projection", 0, m_Matrices[PROJECTION]);
	if( nullptr != a_pOwner ) calView(a_pOwner->getTransform());
}

Camera::~Camera()
{
	m_pCameraBlock = nullptr;
}

void Camera::postInit()
{
	RenderableComponent::postInit();
	addTransformListener();
}

void Camera::start()
{
	auto l_pThis = shared_from_base<Camera>();
	if( !isHidden() ) getScene()->getSceneGraph(GRAPH_CAMERA)->add(l_pThis);
}

void Camera::end()
{
	auto l_pThis = shared_from_base<Camera>();
	if( !isHidden() ) getScene()->getSceneGraph(GRAPH_CAMERA)->remove(l_pThis);
}

void Camera::hiddenFlagChanged()
{
	if( isHidden() )
	{
		getScene()->getSceneGraph(GRAPH_CAMERA)->remove(shared_from_base<Camera>());
		removeTransformListener();
	}
	else
	{
		getScene()->getSceneGraph(GRAPH_CAMERA)->add(shared_from_base<Camera>());
		addTransformListener();
	}
}

void Camera::loadComponent(boost::property_tree::ptree &a_Src)
{
}

void Camera::saveComponent(boost::property_tree::ptree &a_Dst)
{
}

void Camera::setOrthoView(float a_Width, float a_Height, float a_Near, float a_Far, glm::mat4x4 &a_Transform)
{
	m_ViewParam = glm::vec4(a_Width, a_Height, a_Near, a_Far);
	m_pCameraBlock->setParam("m_CameraParam", 0, m_ViewParam);

    m_Type = ORTHO;
	calProjection(a_Transform);
}
    
void Camera::setPerspectiveView(float a_Fovy, float a_Aspect, float a_Near, glm::mat4x4 &a_Transform)
{
	m_ViewParam = glm::vec4(a_Fovy, a_Aspect, a_Near, m_ViewParam.w);
	m_pCameraBlock->setParam("m_CameraParam", 0, m_ViewParam);
    m_Type = PERSPECTIVE;
	calProjection(a_Transform);
}

void Camera::setTetrahedonView(glm::mat4x4 &a_Transform)
{
	m_Type = TETRAHEDRON;
	calViewProjection(a_Transform);
}

void Camera::setCubeView(glm::mat4x4 &a_Transform)
{
	m_Type = CUBE;
	calViewProjection(a_Transform);
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
	
	auto l_pThis = shared_from_base<Camera>();
	if( !isHidden() )
	{
		boundingBox().m_Center = glm::vec3(a_NewTransform[0][3], a_NewTransform[1][3], a_NewTransform[2][3]);
		if( TETRAHEDRON == m_Type || CUBE == m_Type ) boundingBox().m_Size = glm::vec3(m_ViewParam.w, m_ViewParam.w, m_ViewParam.w);
		else boundingBox().m_Size = glm::one<glm::vec3>();
		getScene()->getSceneGraph(GRAPH_CAMERA)->update(l_pThis);
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

		//case TETRAHEDRON:
		//case CUBE:
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

		//case TETRAHEDRON:
		//case CUBE:
		default:break;
	}
	calViewProjection(a_NewTransform);
}

void Camera::calViewProjection(const glm::mat4x4 &a_NewTransform)
{
	switch( m_Type )
	{
		case ORTHO:
		case PERSPECTIVE:{
			m_Matrices[VIEWPROJECTION] = m_Matrices[PROJECTION] * m_Matrices[VIEW];
			m_Matrices[INVERTVIEWPROJECTION] = glm::inverse(m_Matrices[VIEWPROJECTION]);
			m_Frustum.fromViewProjection(m_Matrices[VIEWPROJECTION]);

			m_pCameraBlock->setParam("m_ViewProjection", 0, m_Matrices[VIEWPROJECTION]);
			m_pCameraBlock->setParam("m_InvViewProjection", 0, m_Matrices[INVERTVIEWPROJECTION]);
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
