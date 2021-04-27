// OmniLight.cpp
//
// 2017/09/15 Ian Wu/Real0000
//

#include "CommonUtil.h"
#include "Core.h"

#include "Physical/IntersectHelper.h"
#include "Physical/PhysicalModule.h"
#include "RenderObject/Mesh.h"
#include "Scene/Camera.h"
#include "Scene/Scene.h"

#include "Light.h"

namespace R
{

#define MIN_PHYSIC_DIST 0.5f

#pragma region Light
//
// Light
//
Light::Light(std::shared_ptr<Scene> a_pRefScene, std::shared_ptr<SceneNode> a_pOwner)
	: RenderableComponent(a_pRefScene, a_pOwner)
	, m_bStatic(false)
	, m_pHelper(nullptr)
{
}

Light::~Light()
{
}

void Light::postInit()
{
	RenderableComponent::postInit();
	m_pHelper = new IntersectHelper(shared_from_base<EngineComponent>(), 
		[=](PhysicalListener *a_pListener) -> int
		{
			return createTrigger(a_pListener);
		},
		[=]() -> glm::mat4x4
		{
			return updateTrigger();
		}, TRIGGER_MESH);
	if( !isHidden() )
	{
		m_pHelper->setupTrigger(true);
		// addTransformListener(); add by LightContainer::create later
	}
}

void Light::preEnd()
{
	SAFE_DELETE(m_pHelper)
	if( !isHidden() ) removeTransformListener();
}

void Light::hiddenFlagChanged()
{
	if( isHidden() )
	{
		m_pHelper->removeTrigger();
		removeTransformListener();
	}
	else
	{
		m_pHelper->setupTrigger(true);
		addTransformListener();
	}
}

void Light::transformListener(const glm::mat4x4 &a_NewTransform)
{
	if( !isHidden() ) m_pHelper->setupTrigger(false);
}

void Light::setStatic(bool a_bStatic)
{
	if( m_bStatic == a_bStatic ) return;
	m_bStatic = a_bStatic;
}
#pragma endregion

#pragma region DirLight
//
// DirLight
//
DirLight::DirLight(std::shared_ptr<Scene> a_pRefScene, std::shared_ptr<SceneNode> a_pOwner)
	: Light(a_pRefScene, a_pOwner)
	, m_pRefParam(nullptr)
	, m_ID(0)
{
}

DirLight::~DirLight()
{
}
	
void DirLight::preEnd()
{
	Light::preEnd();
	getScene()->getDirLightContainer()->recycle(shared_from_base<DirLight>());
}

void DirLight::transformListener(const glm::mat4x4 &a_NewTransform)
{
	m_pRefParam->m_Direction = glm::normalize(glm::vec3(a_NewTransform[0][0], a_NewTransform[0][1], a_NewTransform[0][2]));
	getScene()->getDirLightContainer()->setDirty();

	Light::transformListener(a_NewTransform);
}

void DirLight::loadComponent(boost::property_tree::ptree &a_Src)
{
	assert(nullptr != m_pRefParam);

	boost::property_tree::ptree &l_Attr = a_Src.get_child("<xmlattr>");
	m_pRefParam->m_Color.x = l_Attr.get("r", 1.0f);
	m_pRefParam->m_Color.y = l_Attr.get("g", 1.0f);
	m_pRefParam->m_Color.z = l_Attr.get("b", 1.0f);
	m_pRefParam->m_Intensity = l_Attr.get("intensity", 1.0f);
	m_pRefParam->m_bCastShadow = l_Attr.get("castShadow", true) ? 1 : 0;
	setHidden(l_Attr.get("isHidden", false));

	getScene()->getDirLightContainer()->setDirty();
}

void DirLight::saveComponent(boost::property_tree::ptree &a_Dst)
{
	assert(nullptr != m_pRefParam);

	boost::property_tree::ptree l_Root;

	boost::property_tree::ptree l_Attr;
	
	l_Attr.add("r", m_pRefParam->m_Color.x);
	l_Attr.add("g", m_pRefParam->m_Color.y);
	l_Attr.add("b", m_pRefParam->m_Color.z);
	l_Attr.add("intensity", m_pRefParam->m_Intensity);
	l_Attr.add("castShadow", m_pRefParam->m_bCastShadow != 0);
	l_Attr.add("isHidden", isHidden());

	l_Root.add_child("<xmlattr>", l_Attr);
	
	a_Dst.add_child("DirLight", l_Root);
}

void DirLight::setShadowed(bool a_bShadow)
{
	assert(nullptr != m_pRefParam);
	m_pRefParam->m_bCastShadow = a_bShadow ? 1 : 0;
	getScene()->getDirLightContainer()->setDirty();
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
	getScene()->getDirLightContainer()->setDirty();
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
	getScene()->getDirLightContainer()->setDirty();
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

void DirLight::setShadowMapLayer(unsigned int a_Slot, glm::vec4 a_UV, int a_Layer)
{
	assert(nullptr != m_pRefParam);
	m_pRefParam->m_Layer[a_Slot] = a_Layer;
	bool l_bXY = 0 == (a_Slot%2);
	m_pRefParam->m_ShadowMapUV[a_Slot] = a_UV;
	getScene()->getDirLightContainer()->setDirty();
}

glm::vec4 DirLight::getShadowMapUV(unsigned int a_Slot)
{
	assert(nullptr != m_pRefParam);
	return m_pRefParam->m_ShadowMapUV[a_Slot];
}

int DirLight::getShadowMapLayer(unsigned int a_Slot)
{
	assert(nullptr != m_pRefParam);
	return m_pRefParam->m_Layer[a_Slot];
}

void DirLight::setShadowMapProjection(unsigned int a_Slot, glm::mat4x4 a_Matrix)
{
	assert(nullptr != m_pRefParam);
	m_pRefParam->m_ShadowMapProj[a_Slot] = a_Matrix;
	getScene()->getDirLightContainer()->setDirty();
}

glm::mat4x4 DirLight::getShadowMapProjection(unsigned int a_Slot)
{
	assert(nullptr != m_pRefParam);
	return m_pRefParam->m_ShadowMapProj[a_Slot];
}

unsigned int DirLight::getID()
{
	return m_ID;
}

int DirLight::createTrigger(PhysicalListener *a_pListener)
{
	glm::obb l_Box;
	l_Box.m_Size.x = l_Box.m_Size.y = l_Box.m_Size.z = std::numeric_limits<float>::max();
	l_Box.m_Transition = glm::identity<glm::mat4x4>();
	return getScene()->getPhysicalWorld()->createTrigger(a_pListener, l_Box, TRIGGER_LIGHT, TRIGGER_CAMERA | TRIGGER_MESH);
}

glm::mat4x4 DirLight::updateTrigger()
{
	return glm::identity<glm::mat4x4>();
}
#pragma endregion

#pragma region OmniLight
//
// OmniLight
//
OmniLight::OmniLight(std::shared_ptr<Scene> a_pRefScene, std::shared_ptr<SceneNode> a_pOwner)
	: Light(a_pRefScene, a_pOwner)
	, m_pRefParam(nullptr)
	, m_ID(0)
{
}

OmniLight::~OmniLight()
{
}

void OmniLight::preEnd()
{
	Light::preEnd();
	getScene()->getOmniLightContainer()->recycle(shared_from_base<OmniLight>());
}

void OmniLight::transformListener(const glm::mat4x4 &a_NewTransform)
{
	glm::vec3 l_Scale;
	glm::quat l_Rot;
	decomposeTRS(a_NewTransform, m_pRefParam->m_Position, l_Scale, l_Rot);

	m_pRefParam->m_Range = std::max(std::max(l_Scale.x, l_Scale.y), l_Scale.z);
	if( m_pRefParam->m_PhysicRange + MIN_PHYSIC_DIST >= m_pRefParam->m_Range )
	{
		m_pRefParam->m_PhysicRange = std::max(0.0f, m_pRefParam->m_Range - MIN_PHYSIC_DIST);
	}
	
	getScene()->getOmniLightContainer()->setDirty();

	Light::transformListener(a_NewTransform);
}

void OmniLight::loadComponent(boost::property_tree::ptree &a_Src)
{
	assert(nullptr != m_pRefParam);

	boost::property_tree::ptree &l_Attr = a_Src.get_child("<xmlattr>");
	m_pRefParam->m_Color.x = l_Attr.get("r", 1.0f);
	m_pRefParam->m_Color.y = l_Attr.get("g", 1.0f);
	m_pRefParam->m_Color.z = l_Attr.get("b", 1.0f);
	m_pRefParam->m_Intensity = l_Attr.get("intensity", 1.0f);
	m_pRefParam->m_bCastShadow = l_Attr.get("castShadow", true) ? 1 : 0;
	m_pRefParam->m_PhysicRange = l_Attr.get("physicRange", m_pRefParam->m_Range);
	setHidden(l_Attr.get("isHidden", false));

	getScene()->getOmniLightContainer()->setDirty();
}

void OmniLight::saveComponent(boost::property_tree::ptree &a_Dst)
{
	assert(nullptr != m_pRefParam);

	boost::property_tree::ptree l_Root;

	boost::property_tree::ptree l_Attr;
	
	l_Attr.add("r", m_pRefParam->m_Color.x);
	l_Attr.add("g", m_pRefParam->m_Color.y);
	l_Attr.add("b", m_pRefParam->m_Color.z);
	l_Attr.add("intensity", m_pRefParam->m_Intensity);
	l_Attr.add("castShadow", m_pRefParam->m_bCastShadow != 0);
	l_Attr.add("physicRange", m_pRefParam->m_PhysicRange);
	l_Attr.add("isHidden", isHidden());

	l_Root.add_child("<xmlattr>", l_Attr);
	
	a_Dst.add_child("OmniLight", l_Root);
}

void OmniLight::setShadowed(bool a_bShadow)
{
	assert(nullptr != m_pRefParam);
	m_pRefParam->m_bCastShadow = a_bShadow ? 1 : 0;
	getScene()->getOmniLightContainer()->setDirty();
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

void OmniLight::setPhysicRange(float a_Range)
{
	assert(nullptr != m_pRefParam);
	a_Range = std::abs(a_Range);
	if( a_Range + MIN_PHYSIC_DIST >= m_pRefParam->m_Range )
	{
		a_Range = std::max(m_pRefParam->m_Range - MIN_PHYSIC_DIST, 0.0f);
	}
	m_pRefParam->m_PhysicRange = a_Range;
	getScene()->getOmniLightContainer()->setDirty();
}

float OmniLight::getPhysicRange()
{
	assert(nullptr != m_pRefParam);
	return m_pRefParam->m_PhysicRange;
}

void OmniLight::setColor(glm::vec3 a_Color)
{
	assert(nullptr != m_pRefParam);
	m_pRefParam->m_Color = a_Color;
	getScene()->getOmniLightContainer()->setDirty();
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
	getScene()->getOmniLightContainer()->setDirty();
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
	getScene()->getOmniLightContainer()->setDirty();
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
	getScene()->getOmniLightContainer()->setDirty();
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

int OmniLight::createTrigger(PhysicalListener *a_pListener)
{
	glm::sphere l_Sphere;
	l_Sphere.m_Center = m_pRefParam->m_Position;
	l_Sphere.m_Range = m_pRefParam->m_Range;
	return getScene()->getPhysicalWorld()->createTrigger(a_pListener, l_Sphere, TRIGGER_LIGHT, TRIGGER_CAMERA | TRIGGER_MESH);
}

glm::mat4x4 OmniLight::updateTrigger()
{
	return getOwner()->getTransform();
}
#pragma endregion

#pragma region SpotLight
//
// SpotLight
//
SpotLight::SpotLight(std::shared_ptr<Scene> a_pRefScene, std::shared_ptr<SceneNode> a_pOwner)
	: Light(a_pRefScene, a_pOwner)
	, m_pRefParam(nullptr)
	, m_ID(0)
{
}

SpotLight::~SpotLight()
{
}

void SpotLight::preEnd()
{
	Light::preEnd();
	getScene()->getSpotLightContainer()->recycle(shared_from_base<SpotLight>());
}

void SpotLight::transformListener(const glm::mat4x4 &a_NewTransform)
{
	glm::vec3 l_Scale;
	glm::quat l_Rot;
	decomposeTRS(a_NewTransform, m_pRefParam->m_Position, l_Scale, l_Rot);

	m_pRefParam->m_Range = std::max(l_Scale.x, 0.01f);
	if( m_pRefParam->m_PhysicRange + MIN_PHYSIC_DIST >= m_pRefParam->m_Range )
	{
		m_pRefParam->m_PhysicRange = std::max(0.0f, m_pRefParam->m_Range - MIN_PHYSIC_DIST);
	}

	m_pRefParam->m_Direction = glm::normalize(glm::vec3(a_NewTransform[0][0], a_NewTransform[1][0], a_NewTransform[2][0]));
	float l_Size = std::max(l_Scale.y, l_Scale.z);
	m_pRefParam->m_Angle = std::atan2(l_Size, l_Scale.x);

	getScene()->getSpotLightContainer()->setDirty();
	
	Light::transformListener(a_NewTransform);
}

void SpotLight::loadComponent(boost::property_tree::ptree &a_Src)
{
	assert(nullptr != m_pRefParam);

	boost::property_tree::ptree &l_Attr = a_Src.get_child("<xmlattr>");
	m_pRefParam->m_Color.x = l_Attr.get("r", 1.0f);
	m_pRefParam->m_Color.y = l_Attr.get("g", 1.0f);
	m_pRefParam->m_Color.z = l_Attr.get("b", 1.0f);
	m_pRefParam->m_Intensity = l_Attr.get("intensity", 1.0f);
	m_pRefParam->m_bCastShadow = l_Attr.get("castShadow", true) ? 1 : 0;
	m_pRefParam->m_PhysicRange = l_Attr.get("physicRange", m_pRefParam->m_Range);
	setHidden(l_Attr.get("isHidden", false));

	getScene()->getSpotLightContainer()->setDirty();
}

void SpotLight::saveComponent(boost::property_tree::ptree &a_Dst)
{
	assert(nullptr != m_pRefParam);

	boost::property_tree::ptree l_Root;

	boost::property_tree::ptree l_Attr;
	
	l_Attr.add("r", m_pRefParam->m_Color.x);
	l_Attr.add("g", m_pRefParam->m_Color.y);
	l_Attr.add("b", m_pRefParam->m_Color.z);
	l_Attr.add("intensity", m_pRefParam->m_Intensity);
	l_Attr.add("castShadow", m_pRefParam->m_bCastShadow != 0);
	l_Attr.add("physicRange", m_pRefParam->m_PhysicRange);
	l_Attr.add("isHidden", isHidden());

	l_Root.add_child("<xmlattr>", l_Attr);
	
	a_Dst.add_child("SpotLight", l_Root);
}

void SpotLight::setShadowed(bool a_bShadow)
{
	assert(nullptr != m_pRefParam);
	m_pRefParam->m_bCastShadow = a_bShadow ? 1 : 0;
	getScene()->getSpotLightContainer()->setDirty();
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

void SpotLight::setPhysicRange(float a_Range)
{
	assert(nullptr != m_pRefParam);
	a_Range = std::abs(a_Range);
	if( a_Range + MIN_PHYSIC_DIST >= m_pRefParam->m_Range )
	{
		a_Range = std::max(m_pRefParam->m_Range - MIN_PHYSIC_DIST, 0.0f);
	}
	m_pRefParam->m_PhysicRange = a_Range;
	getScene()->getSpotLightContainer()->setDirty();
}

float SpotLight::getPhysicRange()
{
	assert(nullptr != m_pRefParam);
	return m_pRefParam->m_PhysicRange;
}

void SpotLight::setColor(glm::vec3 a_Color)
{
	assert(nullptr != m_pRefParam);
	m_pRefParam->m_Color = a_Color;
	getScene()->getSpotLightContainer()->setDirty();
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
	getScene()->getSpotLightContainer()->setDirty();
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
	getScene()->getSpotLightContainer()->setDirty();
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
	getScene()->getSpotLightContainer()->setDirty();
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

int SpotLight::createTrigger(PhysicalListener *a_pListener)
{
	glm::mat4x4 l_World(updateTrigger());
	return getScene()->getPhysicalWorld()->createTrigger(a_pListener, 0.5f * m_pRefParam->m_Angle, m_pRefParam->m_Range, l_World, TRIGGER_CAMERA, TRIGGER_LIGHT | TRIGGER_MESH);
}

glm::mat4x4 SpotLight::updateTrigger()
{
	glm::mat4x4 l_World(getOwner()->getTransform());
	return l_World * glm::rotate(glm::identity<glm::mat4x4>(), glm::pi<float>(), glm::vec3(l_World[2][0], l_World[2][1], l_World[2][2]));
}
#pragma endregion

}