// Mesh.cpp
//
// 2017/09/20 Ian Wu/Real0000
//

#include "CommonUtil.h"
#include "RImporters.h"
#include "RGDeviceWrapper.h"
#include "Core.h"

#include "Asset/AssetBase.h"
#include "Asset/MeshAsset.h"
#include "Asset/MaterialAsset.h"
#include "Scene/Scene.h"
#include "Scene/Graph/ScenePartition.h"

#include "Mesh.h"

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
{
}

RenderableMesh::~RenderableMesh()
{
}

void RenderableMesh::start()
{
	addTransformListener();
	getScene()->getSceneGraph(GRAPH_MESH)->add(shared_from_base<RenderableMesh>());
	m_WorldOffset = getScene()->getRenderBatcher()->requestWorldSlot(shared_from_base<RenderableMesh>());
}

void RenderableMesh::end()
{
	std::shared_ptr<RenderableMesh> l_pThis = shared_from_base<RenderableMesh>();
	
	if( !isHidden() ) getScene()->getSceneGraph(GRAPH_MESH)->remove(l_pThis);
	getScene()->getRenderBatcher()->recycleWorldSlot(l_pThis);
	if( -1 != m_SkinOffset ) getScene()->getRenderBatcher()->recycleSkinSlot(m_pMesh);

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
		getScene()->getSceneGraph(GRAPH_MESH)->remove(shared_from_base<RenderableMesh>());
		removeTransformListener();
	}
	else
	{
		getScene()->getSceneGraph(GRAPH_MESH)->add(shared_from_base<RenderableMesh>());
		addTransformListener();
	}
}

void RenderableMesh::updateListener(float a_Delta)
{
}

void RenderableMesh::transformListener(glm::mat4x4 &a_NewTransform)
{
	glm::vec3 l_Trans, l_Scale;
	glm::quat l_Rot;
	decomposeTRS(a_NewTransform, l_Trans, l_Scale, l_Rot);

	glm::aabb &l_Box = m_pMesh->getComponent<MeshAsset>()->getMeshes()[m_MeshIdx]->m_VisibleBoundingBox;
	boundingBox().m_Center = l_Trans + l_Box.m_Center;
	boundingBox().m_Size = l_Scale * l_Box.m_Size;

	getScene()->getSceneGraph(GRAPH_MESH)->update(shared_from_base<RenderableMesh>());
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
	getScene()->getRenderBatcher()->requestSkinSlot(m_pMesh);
	m_pMesh = a_pAsset;
	m_MeshIdx = a_MeshIdx;
	
	m_SkinOffset = getScene()->getRenderBatcher()->requestSkinSlot(m_pMesh);

	MeshAsset::Instance *l_pMeshInst = a_pAsset->getComponent<MeshAsset>()->getMeshes()[m_MeshIdx];
	m_Materials.clear();
	for( auto it=l_pMeshInst->m_Materials.begin() ; it!=l_pMeshInst->m_Materials.end() ; ++it ) setMaterial(it->first, it->second);
	syncKeyMap();

	glm::vec3 l_Trans, l_Scale;
	glm::quat l_Rot;
	glm::aabb &l_Box = l_pMeshInst->m_VisibleBoundingBox;
	decomposeTRS(getOwner()->getTransform(), l_Trans, l_Scale, l_Rot);
	boundingBox().m_Center = l_Trans + l_Box.m_Center;
	boundingBox().m_Size = l_Scale * l_Box.m_Size;
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
	
	MeshAsset *l_pMeshRootInst = a_pAsset->getComponent<MeshAsset>();
	MeshAsset::Instance *l_pMeshInst = l_pMeshRootInst->getMeshes()[m_MeshIdx];
	
	l_pMaterialInst->setParam("m_World", m_WorldOffset, getOwner()->getTransform());
	l_pMaterialInst->setParam("m_VtxFlag", m_WorldOffset, l_pMeshInst->m_VtxFlag);
	l_pMaterialInst->setParam("m_SkinMatBase", m_WorldOffset, m_SkinOffset);

	l_pMaterialInst->setBlock(STANDARD_TRANSFORM_SKIN, getScene()->getRenderBatcher()->getSkinMatrixBlock());
	l_pMaterialInst->setBlock(STANDARD_TRANSFORM_NORMAL, getScene()->getRenderBatcher()->getWorldMatrixBlock());
	syncKeyMap();
}

std::shared_ptr<Asset> RenderableMesh::getMaterial(unsigned int a_Slot)
{
	auto it = m_Materials.find(a_Slot);
	return m_Materials.end() == it ? nullptr : it->second.second;
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