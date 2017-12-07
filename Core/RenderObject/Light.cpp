// OmniLight.cpp
//
// 2017/09/15 Ian Wu/Real0000
//

#include "CommonUtil.h"
#include "Core.h"
#include "Scene/Camera.h"
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
	, m_pShadowCamera(EngineComponent::create<CameraComponent>(a_pSharedMember, nullptr))
{
}

Light::~Light()
{
}

void Light::start()
{
	getSharedMember()->m_pGraphs[SharedSceneMember::GRAPH_LIGHT]->add(shared_from_base<Light>());
}

void Light::end()
{
	m_pShadowCamera = nullptr;
	if( isHidden() ) return;

	getSharedMember()->m_pGraphs[SharedSceneMember::GRAPH_LIGHT]->remove(shared_from_base<Light>());
}

void Light::hiddenFlagChanged()
{
	if( isHidden() )
	{
		getSharedMember()->m_pGraphs[SharedSceneMember::GRAPH_LIGHT]->remove(shared_from_base<Light>());
		removeTransformListener();
	}
	else
	{
		getSharedMember()->m_pGraphs[SharedSceneMember::GRAPH_LIGHT]->add(shared_from_base<Light>());
		addTransformListener();
	}
}

void Light::transformListener(glm::mat4x4 &a_NewTransform)
{
	getSharedMember()->m_pGraphs[SharedSceneMember::GRAPH_LIGHT]->update(shared_from_base<Light>());
}
#pragma endregion

#pragma region DirLight
//
// DirLight
//
DirLight::DirLight(SharedSceneMember *a_pSharedMember, std::shared_ptr<SceneNode> a_pOwner)
	: Light(a_pSharedMember, a_pOwner)
	, m_pRefParam(nullptr)
	, m_ID(0)
{
	boundingBox().m_Size = glm::vec3(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
	boundingBox().m_Center = glm::zero<glm::vec3>();
}

DirLight::~DirLight()
{
}
	
void DirLight::end()
{
	Light::end();
	getSharedMember()->m_pDirLights->recycle(shared_from_base<DirLight>());
}

void DirLight::transformListener(glm::mat4x4 &a_NewTransform)
{
	m_pRefParam->m_Direction = glm::normalize(glm::vec3(a_NewTransform[0][0], a_NewTransform[1][0], a_NewTransform[2][0]));
	getSharedMember()->m_pDirLights->setDirty();

	Light::transformListener(a_NewTransform);
}

void DirLight::setShadowed(bool a_bShadow)
{
	assert(nullptr != m_pRefParam);
	m_pRefParam->m_bCastShadow = a_bShadow ? 1 : 0;
	getSharedMember()->m_pDirLights->setDirty();
}

bool DirLight::getShadowed()
{
	assert(nullptr != m_pRefParam);
	return 0 != m_pRefParam->m_bCastShadow;
}

void DirLight::setColor(glm::vec3 a_Color)
{
	assert(nullptr != m_pRefParam);
	m_pRefParam->m_Color = a_Color;
	getSharedMember()->m_pDirLights->setDirty();
}

glm::vec3 DirLight::getColor()
{
	assert(nullptr != m_pRefParam);
	return m_pRefParam->m_Color;
}

void DirLight::setIntensity(float a_Intensity)
{
	assert(nullptr != m_pRefParam);
	m_pRefParam->m_Intensity = a_Intensity;
	getSharedMember()->m_pDirLights->setDirty();
}

float DirLight::getIntensity()
{
	assert(nullptr != m_pRefParam);
	return m_pRefParam->m_Intensity;
}

glm::vec3 DirLight::getDirection()
{
	assert(nullptr != m_pRefParam);
	return m_pRefParam->m_Direction;
}

void DirLight::setShadowMapLayer(int a_Layer)
{
	assert(nullptr != m_pRefParam);
	m_pRefParam->m_Layer = a_Layer;
	getSharedMember()->m_pDirLights->setDirty();
}

int DirLight::getShadowMapLayer()
{
	assert(nullptr != m_pRefParam);
	return m_pRefParam->m_Layer;
}

void DirLight::setShadowMapProjection(glm::mat4x4 a_Matrix)
{
	assert(nullptr != m_pRefParam);
	m_pRefParam->m_ShadowMapProj = a_Matrix;
	getSharedMember()->m_pDirLights->setDirty();
}

glm::mat4x4 DirLight::getShadowMapProjection()
{
	assert(nullptr != m_pRefParam);
	return m_pRefParam->m_ShadowMapProj;
}

unsigned int DirLight::getID()
{
	return m_ID;
}
#pragma endregion

#pragma region OmniLight
//
// OmniLight
//
OmniLight::OmniLight(SharedSceneMember *a_pSharedMember, std::shared_ptr<SceneNode> a_pOwner)
	: Light(a_pSharedMember, a_pOwner)
	, m_pRefParam(nullptr)
	, m_ID(0)
{
}

OmniLight::~OmniLight()
{
}

void OmniLight::end()
{
	Light::end();
	getSharedMember()->m_pOmniLights->recycle(shared_from_base<OmniLight>());
}

void OmniLight::transformListener(glm::mat4x4 &a_NewTransform)
{
	glm::vec3 l_Scale;
	glm::quat l_Rot;
	decomposeTRS(a_NewTransform, m_pRefParam->m_Position, l_Scale, l_Rot);

	m_pRefParam->m_Range = std::max(std::max(l_Scale.x, l_Scale.y), l_Scale.z);
	
	boundingBox().m_Center = m_pRefParam->m_Position;
	boundingBox().m_Size = glm::vec3(m_pRefParam->m_Range, m_pRefParam->m_Range, m_pRefParam->m_Range);
	getSharedMember()->m_pOmniLights->setDirty();

	getShadowCamera()->setCubeView(a_NewTransform);

	Light::transformListener(a_NewTransform);
}

void OmniLight::setShadowed(bool a_bShadow)
{
	assert(nullptr != m_pRefParam);
	m_pRefParam->m_bCastShadow = a_bShadow ? 1 : 0;
	getSharedMember()->m_pOmniLights->setDirty();
}

bool OmniLight::getShadowed()
{
	assert(nullptr != m_pRefParam);
	return 0 != m_pRefParam->m_bCastShadow;
}

glm::vec3 OmniLight::getPosition()
{
	assert(nullptr != m_pRefParam);
	return m_pRefParam->m_Position;
}

float OmniLight::getRange()
{
	assert(nullptr != m_pRefParam);
	return m_pRefParam->m_Range;
}

void OmniLight::setColor(glm::vec3 a_Color)
{
	assert(nullptr != m_pRefParam);
	m_pRefParam->m_Color = a_Color;
	getSharedMember()->m_pOmniLights->setDirty();
}

glm::vec3 OmniLight::getColor()
{
	assert(nullptr != m_pRefParam);
	return m_pRefParam->m_Color;
}

void OmniLight::setIntensity(float a_Intensity)
{
	assert(nullptr != m_pRefParam);
	m_pRefParam->m_Intensity = a_Intensity;
	getSharedMember()->m_pOmniLights->setDirty();
}

float OmniLight::getIntensity()
{
	assert(nullptr != m_pRefParam);
	return m_pRefParam->m_Intensity;
}

void OmniLight::setShadowMapUV(glm::vec4 a_UV, int a_Layer)
{
	assert(nullptr != m_pRefParam);
	m_pRefParam->m_ShadowMapUV = a_UV;
	m_pRefParam->m_Layer = a_Layer;
	getSharedMember()->m_pOmniLights->setDirty();
}

glm::vec4 OmniLight::getShadowMapUV()
{
	assert(nullptr != m_pRefParam);
	return m_pRefParam->m_ShadowMapUV;
}

int OmniLight::getShadowMapLayer()
{
	assert(nullptr != m_pRefParam);
	return m_pRefParam->m_Layer;
}

void OmniLight::setShadowMapProjection(glm::mat4x4 a_Matrix, unsigned int a_Index)
{
	assert(nullptr != m_pRefParam);
	assert(a_Index < 4);
	m_pRefParam->m_ShadowMapProj[a_Index] = a_Matrix;
	getSharedMember()->m_pOmniLights->setDirty();
}

glm::mat4x4 OmniLight::getShadowMapProjection(unsigned int a_Index)
{
	assert(nullptr != m_pRefParam);
	assert(a_Index < 4);
	return m_pRefParam->m_ShadowMapProj[a_Index];
}

unsigned int OmniLight::getID()
{
	return m_ID;
}
#pragma endregion

#pragma region SpotLight
//
// SpotLight
//
SpotLight::SpotLight(SharedSceneMember *a_pSharedMember, std::shared_ptr<SceneNode> a_pOwner)
	: Light(a_pSharedMember, a_pOwner)
	, m_pRefParam(nullptr)
	, m_ID(0)
{
}

SpotLight::~SpotLight()
{
}

void SpotLight::end()
{
	Light::end();
	getSharedMember()->m_pSpotLights->recycle(shared_from_base<SpotLight>());
}

void SpotLight::transformListener(glm::mat4x4 &a_NewTransform)
{
	glm::vec3 l_Scale;
	glm::quat l_Rot;
	decomposeTRS(a_NewTransform, m_pRefParam->m_Position, l_Scale, l_Rot);

	m_pRefParam->m_Range = std::max(l_Scale.x, 0.01f);
	m_pRefParam->m_Direction = glm::normalize(glm::vec3(a_NewTransform[0][0], a_NewTransform[1][0], a_NewTransform[2][0]));
	float l_Size = std::max(l_Scale.y, l_Scale.z);
	m_pRefParam->m_Angle = std::atan2(l_Size, l_Scale.x);

	boundingBox().m_Size = glm::vec3(l_Size, l_Size, l_Size);
	boundingBox().m_Center = m_pRefParam->m_Position + m_pRefParam->m_Direction * 0.5f * m_pRefParam->m_Range;
	getSharedMember()->m_pSpotLights->setDirty();
	
	getShadowCamera()->setPerspectiveView(m_pRefParam->m_Angle, 1.0f, 0.01f, m_pRefParam->m_Range, a_NewTransform);

	Light::transformListener(a_NewTransform);
}


void SpotLight::setShadowed(bool a_bShadow)
{
	assert(nullptr != m_pRefParam);
	m_pRefParam->m_bCastShadow = a_bShadow ? 1 : 0;
	getSharedMember()->m_pSpotLights->setDirty();
}

bool SpotLight::getShadowed()
{
	assert(nullptr != m_pRefParam);
	return 0 != m_pRefParam->m_bCastShadow;
}

glm::vec3 SpotLight::getPosition()
{
	assert(nullptr != m_pRefParam);
	return m_pRefParam->m_Position;
}

float SpotLight::getRange()
{
	assert(nullptr != m_pRefParam);
	return m_pRefParam->m_Range;
}

void SpotLight::setColor(glm::vec3 a_Color)
{
	assert(nullptr != m_pRefParam);
	m_pRefParam->m_Color = a_Color;
	getSharedMember()->m_pSpotLights->setDirty();
}

glm::vec3 SpotLight::getColor()
{
	assert(nullptr != m_pRefParam);
	return m_pRefParam->m_Color;
}

void SpotLight::setIntensity(float a_Intensity)
{
	assert(nullptr != m_pRefParam);
	m_pRefParam->m_Intensity = a_Intensity;
	getSharedMember()->m_pSpotLights->setDirty();
}

float SpotLight::getIntensity()
{
	assert(nullptr != m_pRefParam);
	return m_pRefParam->m_Intensity;
}

glm::vec3 SpotLight::getDirection()
{
	assert(nullptr != m_pRefParam);
	return m_pRefParam->m_Direction;
}

float SpotLight::getAngle()
{
	assert(nullptr != m_pRefParam);
	return m_pRefParam->m_Angle;
}

void SpotLight::setShadowMapUV(glm::vec4 a_UV, int a_Layer)
{
	assert(nullptr != m_pRefParam);
	m_pRefParam->m_ShadowMapUV = a_UV;
	m_pRefParam->m_Layer = a_Layer;
	getSharedMember()->m_pSpotLights->setDirty();
}

glm::vec4 SpotLight::getShadowMapUV()
{
	assert(nullptr != m_pRefParam);
	return m_pRefParam->m_ShadowMapUV;
}

int SpotLight::getShadowMapLayer()
{
	assert(nullptr != m_pRefParam);
	return m_pRefParam->m_Layer;
}

void SpotLight::setShadowMapProjection(glm::mat4x4 a_Matrix)
{
	assert(nullptr != m_pRefParam);
	m_pRefParam->m_ShadowMapProj = a_Matrix;
	getSharedMember()->m_pSpotLights->setDirty();
}

glm::mat4x4 SpotLight::getShadowMapProjection()
{
	assert(nullptr != m_pRefParam);
	return m_pRefParam->m_ShadowMapProj;
}

unsigned int SpotLight::getID()
{
	return m_ID;
}
#pragma endregion

}