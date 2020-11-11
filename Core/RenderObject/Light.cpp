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

#define MIN_PHYSIC_DIST 0.5f

#pragma region Light
//
// Light
//
Light::Light(std::shared_ptr<Scene> a_pRefScene, std::shared_ptr<SceneNode> a_pOwner)
	: RenderableComponent(a_pRefScene, a_pOwner)
	, m_pShadowCamera(EngineComponent::create<Camera>(a_pRefScene, nullptr))
	, m_bStatic(false)
{
}

Light::~Light()
{
}

void Light::start()
{
	getScene()->getSceneGraph(m_bStatic ? GRAPH_STATIC_LIGHT : GRAPH_LIGHT)->add(shared_from_base<Light>());
}

void Light::end()
{
	m_pShadowCamera = nullptr;
	if( isHidden() ) return;

	getScene()->getSceneGraph(m_bStatic ? GRAPH_STATIC_LIGHT : GRAPH_LIGHT)->remove(shared_from_base<Light>());
}

void Light::hiddenFlagChanged()
{
	if( isHidden() )
	{
		getScene()->getSceneGraph(m_bStatic ? GRAPH_STATIC_LIGHT : GRAPH_LIGHT)->remove(shared_from_base<Light>());
		removeTransformListener();
	}
	else
	{
		getScene()->getSceneGraph(m_bStatic ? GRAPH_STATIC_LIGHT : GRAPH_LIGHT)->add(shared_from_base<Light>());
		addTransformListener();
	}
}

void Light::transformListener(glm::mat4x4 &a_NewTransform)
{
	getScene()->getSceneGraph(m_bStatic ? GRAPH_STATIC_LIGHT : GRAPH_LIGHT)->update(shared_from_base<Light>());
}

void Light::setStatic(bool a_bStatic)
{
	if( m_bStatic == a_bStatic ) return;
	if( m_bStatic )
	{
		getScene()->getSceneGraph(GRAPH_STATIC_LIGHT)->remove(shared_from_base<Light>());
		getScene()->getSceneGraph(GRAPH_LIGHT)->add(shared_from_base<Light>());
	}
	else
	{
		getScene()->getSceneGraph(GRAPH_LIGHT)->remove(shared_from_base<Light>());
		getScene()->getSceneGraph(GRAPH_STATIC_LIGHT)->add(shared_from_base<Light>());
	}
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
	boundingBox().m_Size = glm::vec3(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
	boundingBox().m_Center = glm::zero<glm::vec3>();
}

DirLight::~DirLight()
{
}
	
void DirLight::end()
{
	Light::end();
	getScene()->getDirLightContainer()->recycle(shared_from_base<DirLight>());
}

void DirLight::transformListener(glm::mat4x4 &a_NewTransform)
{
	m_pRefParam->m_Direction = glm::normalize(glm::vec3(a_NewTransform[0][0], a_NewTransform[1][0], a_NewTransform[2][0]));
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

void OmniLight::end()
{
	Light::end();
	getScene()->getOmniLightContainer()->recycle(shared_from_base<OmniLight>());
}

void OmniLight::transformListener(glm::mat4x4 &a_NewTransform)
{
	glm::vec3 l_Scale;
	glm::quat l_Rot;
	decomposeTRS(a_NewTransform, m_pRefParam->m_Position, l_Scale, l_Rot);

	m_pRefParam->m_Range = std::max(std::max(l_Scale.x, l_Scale.y), l_Scale.z);
	if( m_pRefParam->m_PhysicRange + MIN_PHYSIC_DIST >= m_pRefParam->m_Range )
	{
		m_pRefParam->m_PhysicRange = std::max(0.0f, m_pRefParam->m_Range - MIN_PHYSIC_DIST);
	}
	
	boundingBox().m_Center = m_pRefParam->m_Position;
	boundingBox().m_Size = glm::vec3(m_pRefParam->m_Range, m_pRefParam->m_Range, m_pRefParam->m_Range);
	getScene()->getOmniLightContainer()->setDirty();

	getShadowCamera()->setCubeView(a_NewTransform);

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

void SpotLight::end()
{
	Light::end();
	getScene()->getSpotLightContainer()->recycle(shared_from_base<SpotLight>());
}

void SpotLight::transformListener(glm::mat4x4 &a_NewTransform)
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

	boundingBox().m_Size = glm::vec3(l_Size, l_Size, l_Size);
	boundingBox().m_Center = m_pRefParam->m_Position + m_pRefParam->m_Direction * 0.5f * m_pRefParam->m_Range;
	getScene()->getSpotLightContainer()->setDirty();
	
	getShadowCamera()->setPerspectiveView(m_pRefParam->m_Angle, 1.0f, 0.01f, a_NewTransform);

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
#pragma endregion

}