// Light.h
//
// 2017/09/19 Ian Wu/Real0000
//

#ifndef _LIGHT_H_
#define _LIGHT_H_

#include "RenderObject.h"

namespace R
{
	
class Camera;
class DirLight;
class IntersectHelper;
class MaterialBlock;
class OmniLight;
class PhysicalListener;
class SceneNode;
class SpotLight;

class Light : public RenderableComponent
{
	friend class EngineComponent;
public:
	Light(std::shared_ptr<Scene> a_pRefScene, std::shared_ptr<SceneNode> a_pOwner);
	virtual ~Light();
	
	virtual void postInit();
	virtual void preEnd();
	virtual void hiddenFlagChanged();
	virtual void transformListener(const glm::mat4x4 &a_NewTransform) override;
	virtual void setStatic(bool a_bStatic);
	virtual bool isStatic(){ return m_bStatic; }
	IntersectHelper* getHelper(){ return m_pHelper; }

	virtual unsigned int getID() = 0;

protected:
	virtual int createTrigger(PhysicalListener *a_pListener) = 0;
	virtual glm::mat4x4 updateTrigger() = 0;

private:
	bool m_bStatic;
	IntersectHelper *m_pHelper;
};

template<typename T>
class LightContainer
{
public:
	LightContainer(std::string a_BlockName, unsigned int a_InitSize = 1024, unsigned int a_ExtendSize = 1024)
		: m_bDirty(false)
		, m_pLightData(nullptr)
		, m_FreeCount(a_InitSize), m_ExtendSize(a_ExtendSize)
		, m_MaxID(0)
		, m_Lights(true)
	{
		m_Lights.setNewFunc(std::bind(&LightContainer<T>::newLight, this));

		ProgramBlockDesc *l_pDesc = ProgramManager::singleton().createBlockFromDesc(a_BlockName);
		m_pLightData = MaterialBlock::create(ShaderRegType::UavBuffer, l_pDesc, m_FreeCount);
		SAFE_DELETE(l_pDesc)
	}
	virtual ~LightContainer()
	{
		clear();
		m_pLightData = nullptr;
	}

	void clear()
	{
		m_Lights.clear();
		m_FreeCount = m_Lights.count();
		m_bDirty = true;
	}

	void setExtendSize(unsigned int a_NewSize)
	{
		assert(0 != a_NewSize);
		m_ExtendSize = a_NewSize;
	}

	std::shared_ptr<T> create(std::shared_ptr<Scene> a_pRefScene, std::shared_ptr<SceneNode> a_pOwner)
	{
		std::lock_guard<std::mutex> l_Guard(m_Locker);

		// for retain use
		m_pRefScene = a_pRefScene;
		m_pOwner = a_pOwner;

		std::shared_ptr<T> l_pTarget = nullptr;
		unsigned int l_ID = m_Lights.retain(&l_pTarget);
		l_pTarget->m_ID = l_ID;
		l_pTarget->m_pRefParam = reinterpret_cast<T::Data *>(m_pLightData->getBlockPtr(l_ID));
		l_pTarget->m_pRefParam->m_bCastShadow = 1;
		l_pTarget->setHidden(false);
		l_pTarget->addTransformListener();
		m_MaxID = std::max(l_pTarget->m_ID, m_MaxID);
		return l_pTarget;
	}

	std::shared_ptr<MaterialBlock> getMaterialBlock(){ return m_pLightData; }
	
	void recycle(std::shared_ptr<T> a_pLight)
	{
		std::lock_guard<std::mutex> l_Guard(m_Locker);
		a_pLight->m_pRefParam->m_bCastShadow = 0;
		m_Lights.release(a_pLight->m_ID);
	}

	void setDirty()
	{
		m_bDirty = true;
	}

	void flush()
	{
		if( !m_bDirty ) return;
		m_pLightData->sync(true, false);
		m_bDirty = false;
	}

	std::shared_ptr<T> get(unsigned int a_ID){ return m_Lights[a_ID]; }
	unsigned int getMaxID(){ return m_MaxID; }

private:
	std::shared_ptr<T> newLight()
	{
		std::shared_ptr<T> l_pNewLight = EngineComponent::create<T>(m_pRefScene, m_pOwner);
		m_pOwner = nullptr;
		if( 0 == m_FreeCount )
		{
			m_pLightData->extend(m_ExtendSize);
			m_FreeCount = m_ExtendSize - 1;
		}
		else --m_FreeCount;

		return l_pNewLight;
	}

	// temp data
	std::shared_ptr<Scene> m_pRefScene;
	std::shared_ptr<SceneNode> m_pOwner;

	bool m_bDirty;
	std::shared_ptr<MaterialBlock> m_pLightData;
	unsigned int m_FreeCount;
	unsigned int m_ExtendSize;
	unsigned int m_MaxID;
	SerializedObjectPool<T> m_Lights;
	std::mutex m_Locker;
};

class DirLight : public Light
{
	friend class LightContainer<DirLight>;// for id set
	COMPONENT_HEADER(DirLight)
public:
	virtual ~DirLight();
	
	virtual void preEnd();
	virtual void transformListener(const glm::mat4x4 &a_NewTransform) override;
	virtual void loadComponent(boost::property_tree::ptree &a_Src);
	virtual void saveComponent(boost::property_tree::ptree &a_Dst);

	virtual void setShadowed(bool a_bShadow);
	virtual bool getShadowed() override;

	void setColor(glm::vec3 a_Color);
	glm::vec3 getColor();
	void setIntensity(float a_Intensity);
	float getIntensity();
	glm::vec3 getDirection();
	void setShadowMapLayer(unsigned int a_Slot, glm::vec4 a_UV, int a_Layer);
	glm::vec4 getShadowMapUV(unsigned int a_Slot);
	int getShadowMapLayer(unsigned int a_Slot);
	void setShadowMapProjection(unsigned int a_Slot, glm::mat4x4 a_Matrix);
	glm::mat4x4 getShadowMapProjection(unsigned int a_Slot);

	virtual unsigned int getID();
	
protected:
	virtual int createTrigger(PhysicalListener *a_pListener);
	virtual glm::mat4x4 updateTrigger();

private:
	struct Data
	{
		glm::vec3 m_Color;
		float m_Intensity;
		glm::vec3 m_Direction;
		int m_bCastShadow;
		glm::ivec4 m_Layer;
		glm::vec4 m_ShadowMapUV[4];
		glm::mat4x4 m_ShadowMapProj[4];
	};
	DirLight(std::shared_ptr<Scene> a_pRefScene, std::shared_ptr<SceneNode> a_pOwner);

	Data *m_pRefParam;
	unsigned int m_ID;
};

class OmniLight : public Light
{
	friend class LightContainer<OmniLight>;// for id set
	COMPONENT_HEADER(OmniLight)
public:
	virtual ~OmniLight();
	
	virtual void preEnd();
	virtual void transformListener(const glm::mat4x4 &a_NewTransform) override;
	virtual void loadComponent(boost::property_tree::ptree &a_Src);
	virtual void saveComponent(boost::property_tree::ptree &a_Dst);
	
	virtual void setShadowed(bool a_bShadow);
	virtual bool getShadowed();

	glm::vec3 getPosition();
	float getRange();
	void setPhysicRange(float a_Range);
	float getPhysicRange();
	void setColor(glm::vec3 a_Color);
	glm::vec3 getColor();
	void setIntensity(float a_Intensity);
	float getIntensity();
	void setShadowMapUV(glm::vec4 a_UV, int a_Layer);
	glm::vec4 getShadowMapUV();
	int getShadowMapLayer();
	void setShadowMapProjection(glm::mat4x4 a_Matrix, unsigned int a_Index);
	glm::mat4x4 getShadowMapProjection(unsigned int a_Index);

	virtual unsigned int getID();
	
protected:
	virtual int createTrigger(PhysicalListener *a_pListener);
	virtual glm::mat4x4 updateTrigger();

private:
	struct Data
	{
		glm::vec3 m_Position;
		float m_Range;
		glm::vec3 m_Color;
		float m_Intensity;
		glm::vec4 m_ShadowMapUV;
		int m_Layer;
		int m_bCastShadow;
		float m_PhysicRange;// for light map
		float m_Padding1;
		glm::mat4x4 m_ShadowMapProj[4];
	};
	OmniLight(std::shared_ptr<Scene> a_pRefScene, std::shared_ptr<SceneNode> a_pOwner);

	Data *m_pRefParam;
	unsigned int m_ID;
};

class SpotLight : public Light
{
	friend class LightContainer<SpotLight>;// for id set
	COMPONENT_HEADER(SpotLight)
public:
	virtual ~SpotLight();
	
	virtual void preEnd();
	virtual void transformListener(const glm::mat4x4 &a_NewTransform) override;
	virtual void loadComponent(boost::property_tree::ptree &a_Src);
	virtual void saveComponent(boost::property_tree::ptree &a_Dst);
	
	virtual void setShadowed(bool a_bShadow);
	virtual bool getShadowed();

	glm::vec3 getPosition();
	float getRange();
	void setPhysicRange(float a_Range);
	float getPhysicRange();
	void setColor(glm::vec3 a_Color);
	glm::vec3 getColor();
	void setIntensity(float a_Intensity);
	float getIntensity();
	glm::vec3 getDirection();
	float getAngle();
	void setShadowMapUV(glm::vec4 a_UV, int a_Layer);
	glm::vec4 getShadowMapUV();
	int getShadowMapLayer();
	void setShadowMapProjection(glm::mat4x4 a_Matrix);
	glm::mat4x4 getShadowMapProjection();

	virtual unsigned int getID();
	
protected:
	virtual int createTrigger(PhysicalListener *a_pListener);
	virtual glm::mat4x4 updateTrigger();

private:
	struct Data
	{
		glm::vec3 m_Position;
		float m_Range;
		glm::vec3 m_Color;
		float m_Intensity;
		glm::vec3 m_Direction;
		float m_Angle;
		glm::vec4 m_ShadowMapUV;
		int m_Layer;
		int m_bCastShadow;
		float m_PhysicRange;
		float m_Padding1;
		glm::mat4x4 m_ShadowMapProj;
	};
	SpotLight(std::shared_ptr<Scene> a_pRefScene, std::shared_ptr<SceneNode> a_pOwner);

	Data *m_pRefParam;
	unsigned int m_ID;
};

}

#endif