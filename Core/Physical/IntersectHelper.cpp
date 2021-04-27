// IntersectHelper.cpp
//
// 2021/04/26 Ian Wu/Real0000
//

#include "CommonUtil.h"
#include "RGDeviceWrapper.h"
#include "Core.h"

#include "IntersectHelper.h"

#include "PhysicalModule.h"
#include "RenderObject/Mesh.h"
#include "RenderObject/Light.h"
#include "Scene/Camera.h"
#include "Scene/Scene.h"

namespace R
{

#pragma region IntersectHelper
//
// IntersectHelper
//
IntersectHelper::IntersectHelper(std::shared_ptr<EngineComponent> a_pOwner, std::function<int(PhysicalListener*)> a_pCreateFunc, std::function<glm::mat4x4()> a_pUpdateFunc, unsigned int a_RecordFlag)
	: m_TriggerID(-1)
	, m_pOwner(a_pOwner)
	, m_pRefWorld(nullptr)
	, m_pCreateFunc(a_pCreateFunc), m_pUpdateFunc(a_pUpdateFunc)
	, m_RecordFlag(a_RecordFlag)
{
	assert(nullptr != m_pOwner);
	assert(nullptr != m_pCreateFunc);
	assert(nullptr != m_pUpdateFunc);
	m_pRefWorld = m_pOwner->getOwner()->getScene()->getPhysicalWorld();
}

IntersectHelper::~IntersectHelper()
{
	removeTrigger();
	m_pOwner = nullptr;
	m_pRefWorld = nullptr;
}

void IntersectHelper::updateTrigger()
{
	m_pRefWorld->updateTrigger(m_TriggerID, m_pUpdateFunc());
}

void IntersectHelper::setupTrigger(bool a_bCreate)
{
	if( a_bCreate )
	{
		removeTrigger();

		PhysicalListener *l_pListener = m_pRefWorld->createListener(m_pOwner);
		auto l_pInterSectFunc = [=](std::shared_ptr<EngineComponent> a_pComponent, PhysicalListener::Type a_Type, bool a_bInsert) -> void
		{
			switch( a_pComponent->typeID() )
			{
				case COMPONENT_RenderableMesh:{
					if( 0 != (m_RecordFlag & TRIGGER_MESH) )
					{
						std::shared_ptr<RenderableMesh> l_pMesh = a_pComponent->shared_from_base<RenderableMesh>();
						if( a_bInsert ) m_Meshes.insert(l_pMesh);
						else m_Meshes.erase(l_pMesh);
					}
					}break;

				case COMPONENT_DirLight:
				case COMPONENT_SpotLight:
				case COMPONENT_OmniLight:{
					if( 0 != (m_RecordFlag & TRIGGER_LIGHT) )
					{
						std::shared_ptr<Light> l_pLight = a_pComponent->shared_from_base<Light>();
						if( a_bInsert ) m_Lights.insert(l_pLight);
						else m_Lights.erase(l_pLight);
					}
					}break;

				case COMPONENT_Camera:{
					if( 0 != (m_RecordFlag & TRIGGER_CAMERA) )
					{
						std::shared_ptr<Camera> l_pCamera = a_pComponent->shared_from_base<Camera>();
						if( a_bInsert ) m_Cameras.insert(l_pCamera);
						else m_Cameras.erase(l_pCamera);
					}
					}break;

				default:break;
			}
		};
		l_pListener->setEvent(PhysicalListener::ENTER, std::bind(l_pInterSectFunc, std::placeholders::_1, std::placeholders::_2, true));
		l_pListener->setEvent(PhysicalListener::LEAVE, std::bind(l_pInterSectFunc, std::placeholders::_1, std::placeholders::_2, false));

		m_TriggerID = m_pCreateFunc(l_pListener);
		
	}
	else
	{
		if( m_TriggerID >= 0 ) updateTrigger();
	}
}

void IntersectHelper::removeTrigger()
{
	m_pRefWorld->removeTrigger(m_TriggerID);
	m_Meshes.clear();
	m_Lights.clear();
	m_Cameras.clear();
}
#pragma endregion

}