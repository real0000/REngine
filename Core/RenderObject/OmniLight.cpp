// OmniLight.cpp
//
// 2017/09/15 Ian Wu/Real0000
//

#include "CommonUtil.h"
#include "Core.h"
#include "RGDeviceWrapper.h"
#include "RenderObject/Material.h"
#include "Scene/Scene.h"
#include "Scene/Graph/ScenePartition.h"

#include "OmniLight.h"

namespace R
{

#pragma region OmniLight
//
// OmniLight
//
OmniLight::OmniLight(SharedSceneMember *a_pSharedMember, std::shared_ptr<SceneNode> a_pOwner)
	: Light(a_pSharedMember, a_pOwner)
	, m_pRefParam(nullptr)
	, m_ID(0)
	, m_bHidden(false)
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
	Light::transformListener(a_NewTransform);
	m_pRefParam->m_Position = glm::vec3(a_NewTransform[0][3], a_NewTransform[1][3], a_NewTransform[2][3]);
	getSharedMember()->m_pOmniLights->setDirty();
}

glm::vec3 OmniLight::getPosition()
{
	assert(nullptr != m_pRefParam);
	return m_pRefParam->m_Position;
}

void OmniLight::setRange(float a_Range)
{
	assert(nullptr != m_pRefParam);
	m_pRefParam->m_Range = a_Range;
	getSharedMember()->m_pOmniLights->setDirty();
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

void OmniLight::setShadowMapUV(glm::vec2 a_UV)
{
	assert(nullptr != m_pRefParam);
	m_pRefParam->m_ShadowMapUV = a_UV;
	getSharedMember()->m_pOmniLights->setDirty();
}

glm::vec2 OmniLight::getShadowMapUV()
{
	assert(nullptr != m_pRefParam);
	return m_pRefParam->m_ShadowMapUV;
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

#pragma region OmniLights
//
// OmniLights
//
struct CreateInfo
{
	CreateInfo() : m_pSharedMember(nullptr), m_pOwner(nullptr){}

	SharedSceneMember *m_pSharedMember;
	std::shared_ptr<SceneNode> m_pOwner;
};
thread_local CreateInfo g_CreateInfo;

OmniLights::OmniLights(unsigned int a_InitSize, unsigned int a_ExtendSize)
	: m_bDirty(false)
	, m_pLightData(nullptr)
	, m_FreeCount(a_InitSize), m_ExtendSize(a_ExtendSize)
	, m_Lights(true)
{
	m_Lights.setNewFunc(std::bind(&OmniLights::newOmniLight, this));

	ProgramBlockDesc *l_pDesc = ProgramManager::singleton().createBlockFromDesc("OmniLight");
	m_pLightData = MaterialBlock::create(ShaderRegType::UavBuffer, l_pDesc, m_FreeCount);
	SAFE_DELETE(l_pDesc)
}

OmniLights::~OmniLights()
{
	m_pLightData = nullptr;
}

void OmniLights::setExtendSize(unsigned int a_NewSize)
{
	assert(0 != a_NewSize);
	m_ExtendSize = a_NewSize;
}

std::shared_ptr<OmniLight> OmniLights::create(SharedSceneMember *a_pSharedMember, std::shared_ptr<SceneNode> a_pOwner)
{
	g_CreateInfo.m_pSharedMember = a_pSharedMember;
	g_CreateInfo.m_pOwner = a_pOwner;

	std::shared_ptr<OmniLight> l_pTarget = nullptr;
	unsigned int l_ID = m_Lights.retain(&l_pTarget);
	l_pTarget->m_ID = l_ID;
	l_pTarget->m_pRefParam = (OmniLight::Data *)m_pLightData->getBlockPtr(l_ID);
	l_pTarget->m_bHidden = false;
	l_pTarget->addTransformListener();
	return l_pTarget;
}

void OmniLights::recycle(std::shared_ptr<OmniLight> a_pLight)
{
	m_Lights.release(a_pLight->m_ID);
}

void OmniLights::flush()
{
	if( !m_bDirty ) return;
	m_pLightData->sync(true);
	m_bDirty = false;
}

std::shared_ptr<OmniLight> OmniLights::newOmniLight()
{
	std::lock_guard<std::mutex> l_Guard(m_Locker);

	std::shared_ptr<OmniLight> l_pNewLight = EngineComponent::create<OmniLight>(g_CreateInfo.m_pSharedMember, g_CreateInfo.m_pOwner);
	g_CreateInfo.m_pSharedMember = nullptr;
	g_CreateInfo.m_pOwner = nullptr;
	if( 0 == m_FreeCount )
	{
		m_pLightData->extend(m_ExtendSize);
		m_FreeCount = m_ExtendSize - 1;
	}
	else --m_FreeCount;

	return l_pNewLight;
}
#pragma endregion

}