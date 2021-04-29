// Mesh.cpp
//
// 2017/09/20 Ian Wu/Real0000
//

#include "CommonUtil.h"
#include "RImporters.h"
#include "RGDeviceWrapper.h"
#include "Core.h"

#include "Mesh.h"

#include "Asset/AssetBase.h"
#include "Asset/MeshAsset.h"
#include "Asset/MaterialAsset.h"
#include "Physical/IntersectHelper.h"
#include "Physical/PhysicalModule.h"
#include "Scene/Scene.h"

namespace R
{

#pragma region RenderableMesh
//
// RenderableMesh
//
RenderableMesh::RenderableMesh(std::shared_ptr<Scene> a_pRefScene, std::shared_ptr<SceneNode> a_pNode)
	: RenderableComponent(a_pRefScene, a_pNode)
	, m_pMesh(nullptr)
	, m_MeshIdx(0)
	, m_bStatic(false)
	, m_bShadowed(true)
	, m_SkinOffset(-1)
	, m_WorldOffset(-1)
	, m_BoundingBox()
	, m_pHelper(nullptr)
{
}

RenderableMesh::~RenderableMesh()
{
}

void RenderableMesh::postInit()
{
	RenderableComponent::postInit();
	m_pHelper = new IntersectHelper(shared_from_base<EngineComponent>(), 
		[=](PhysicalListener *a_pListener) -> int
		{
			glm::obb l_Box;
			l_Box.m_Size = m_BoundingBox.m_Size;
			l_Box.m_Transition = getOwner()->getTransform();
			return getScene()->getPhysicalWorld()->createTrigger(a_pListener, l_Box, TRIGGER_MESH, TRIGGER_LIGHT | TRIGGER_CAMERA);
		},
		[=]() -> glm::mat4x4
		{
			return getOwner()->getTransform();
		}, 0);
		
	if( !isHidden() )
	{
		addTransformListener();
		m_pHelper->setupTrigger(true);
	}
	m_WorldOffset = getScene()->getRenderBatcher()->requestWorldSlot(shared_from_base<RenderableMesh>());
}

void RenderableMesh::preEnd()
{
	SAFE_DELETE(m_pHelper)
	std::shared_ptr<RenderableMesh> l_pThis = shared_from_base<RenderableMesh>();
	
	std::shared_ptr<Scene> l_pScene = getScene();
	if( nullptr != l_pScene )
	{
		l_pScene->getRenderBatcher()->recycleWorldSlot(l_pThis);
		if( -1 != m_SkinOffset ) l_pScene->getRenderBatcher()->recycleSkinSlot(m_pMesh);
	}

	for( auto it=m_Materials.begin() ; it!=m_Materials.end() ; ++it )
	{
		it->second.second->getComponent<MaterialAsset>()->freeInstanceSlot(it->second.first);
	}
	m_pMesh = nullptr;
	m_Materials.clear();
}

void RenderableMesh::hiddenFlagChanged()
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

void RenderableMesh::transformListener(const glm::mat4x4 &a_NewTransform)
{
	glm::vec3 l_Trans, l_Scale;
	glm::quat l_Rot;
	decomposeTRS(a_NewTransform, l_Trans, l_Scale, l_Rot);

	if( nullptr != m_pMesh )
	{
		MeshAsset::Instance *l_pMeshInst = m_pMesh->getComponent<MeshAsset>()->getMeshes()[m_MeshIdx];
		getScene()->getRenderBatcher()->updateWorldSlot(m_WorldOffset, getOwner()->getTransform(), l_pMeshInst->m_VtxFlag, m_SkinOffset);
	}
	m_pHelper->setupTrigger(false);
}

void RenderableMesh::loadComponent(boost::property_tree::ptree &a_Src)
{
	
}

void RenderableMesh::saveComponent(boost::property_tree::ptree &a_Dst)
{
	
}

void RenderableMesh::setShadowed(bool a_bShadow)
{
	if( m_bShadowed == a_bShadow ) return;
	m_bShadowed = a_bShadow;
}

void RenderableMesh::setStatic(bool a_bStatic)
{
	if( m_bStatic == a_bStatic ) return;
	m_bStatic = a_bStatic;
}

void RenderableMesh::setMesh(std::shared_ptr<Asset> a_pAsset, unsigned int a_MeshIdx)
{
	getScene()->getRenderBatcher()->recycleSkinSlot(m_pMesh);
	m_pMesh = a_pAsset;
	m_MeshIdx = a_MeshIdx;
	
	m_SkinOffset = getScene()->getRenderBatcher()->requestSkinSlot(m_pMesh);

	MeshAsset::Instance *l_pMeshInst = a_pAsset->getComponent<MeshAsset>()->getMeshes()[m_MeshIdx];
	m_Materials.clear();
	for( auto it=l_pMeshInst->m_Materials.begin() ; it!=l_pMeshInst->m_Materials.end() ; ++it ) setMaterial(it->first, it->second);
	getScene()->getRenderBatcher()->updateWorldSlot(m_WorldOffset, getOwner()->getTransform(), l_pMeshInst->m_VtxFlag, m_SkinOffset);
	syncKeyMap();
	m_BoundingBox = l_pMeshInst->m_VisibleBoundingBox;

	m_pHelper->setupTrigger(true);
}

void RenderableMesh::removeMaterial(unsigned int a_Slot)
{
	auto it = m_Materials.find(a_Slot);
	if( m_Materials.end() == it ) return;

	m_KeyMap[a_Slot].m_Members.m_bValid = 0;
	it->second.second->getComponent<MaterialAsset>()->freeInstanceSlot(it->second.first);
	m_Materials.erase(it);
}

void RenderableMesh::setMaterial(unsigned int a_Slot, std::shared_ptr<Asset> a_pAsset)
{
	if( nullptr == m_pMesh ) return;
	removeMaterial(a_Slot);

	if( nullptr == a_pAsset ) return;
	MaterialAsset *l_pMaterialInst = a_pAsset->getComponent<MaterialAsset>();

	int l_InstanceSlot = l_pMaterialInst->requestInstanceSlot();
	m_Materials.insert(std::make_pair(a_Slot, std::make_pair(l_InstanceSlot, a_pAsset)));
	
	syncKeyMap();
}

RenderableMesh::MaterialData RenderableMesh::getMaterial(unsigned int a_Slot)
{
	auto it = m_Materials.find(a_Slot);
	return m_Materials.end() == it ? std::make_pair(-1, nullptr) : it->second;
}

RenderableMesh::SortKey RenderableMesh::getSortKey(unsigned int a_Slot)
{
	return m_KeyMap[a_Slot];
}

void RenderableMesh::syncKeyMap()
{
	m_KeyMap.clear();
	for( auto it=m_Materials.begin() ; it!=m_Materials.end() ; ++it )
	{
		SortKey &l_Targert = m_KeyMap[it->first];
		l_Targert.m_Key = 0;
		l_Targert.m_Members.m_MaterialID = it->second.second->getSerialKey();
		if( nullptr != m_pMesh )
		{
			l_Targert.m_Members.m_MeshID = m_pMesh->getSerialKey();
			l_Targert.m_Members.m_SubMeshIdx = m_MeshIdx;
			l_Targert.m_Members.m_bValid = 1;
		}
	}
}
#pragma endregion

}