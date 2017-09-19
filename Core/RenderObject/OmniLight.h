// OmniLight.h
//
// 2017/09/15 Ian Wu/Real0000
//

#ifndef _OMNI_LIGHT_H_
#define _OMNI_LIGHT_H_

#include "Light.h"

namespace R
{

class MaterialBlock;
class OmniLights;
class SceneNode;

// use OmniLights::create to create this class instance
class OmniLight : public Light
{
	friend class EngineComponent;
	friend class OmniLights;// for id set
public:
	virtual ~OmniLight();
	
	virtual void end();
	virtual void transformListener(glm::mat4x4 &a_NewTransform);
	virtual unsigned int typeID(){ return COMPONENT_OMNI_LIGHT; }

	glm::vec3 getPosition();
	void setRange(float a_Range);
	float getRange();
	void setColor(glm::vec3 a_Color);
	glm::vec3 getColor();
	void setIntensity(float a_Intensity);
	float getIntensity();
	void setShadowMapUV(glm::vec2 a_UV);
	glm::vec2 getShadowMapUV();
	void setShadowMapProjection(glm::mat4x4 a_Matrix, unsigned int a_Index);
	glm::mat4x4 getShadowMapProjection(unsigned int a_Index);

	unsigned int getID();

private:
	struct Data
	{
		glm::vec3 m_Position;
		float m_Range;
		glm::vec3 m_Color;
		float m_Intensity;
		glm::vec2 m_ShadowMapUV;
		glm::vec2 m_Padding1;
		glm::mat4x4 m_ShadowMapProj[4];
	};
	OmniLight(SharedSceneMember *a_pSharedMember, std::shared_ptr<SceneNode> a_pOwner);

	Data *m_pRefParam;
	unsigned int m_ID;
	bool m_bHidden;
};

class OmniLights
{
public:
	OmniLights(unsigned int a_InitSize = 1024, unsigned int a_ExtendSize = 1024);
	virtual ~OmniLights();

	void setExtendSize(unsigned int a_NewSize);

	std::shared_ptr<OmniLight> create(SharedSceneMember *a_pSharedMember, std::shared_ptr<SceneNode> a_pOwner);
	void recycle(std::shared_ptr<OmniLight> a_pLight);

	void setDirty(){ m_bDirty = true; }
	void flush();

private:
	std::shared_ptr<OmniLight> newOmniLight();

	bool m_bDirty;
	std::shared_ptr<MaterialBlock> m_pLightData;
	unsigned int m_FreeCount;
	unsigned int m_ExtendSize;
	SerializedObjectPool<OmniLight> m_Lights;
	std::mutex m_Locker;
};

}

#endif