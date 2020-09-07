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
RenderableMesh::RenderableMesh(SharedSceneMember *a_pMember, std::shared_ptr<SceneNode> a_pNode)
	: RenderableComponent(a_pMember, a_pNode)
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
	getSharedMember()->m_pGraphs[m_bStatic ? SharedSceneMember::GRAPH_STATIC_MESH : SharedSceneMember::GRAPH_MESH]->add(shared_from_base<RenderableMesh>());
	m_WorldOffset = getSharedMember()->m_pBatcher->requestWorldSlot(shared_from_base<RenderableMesh>());
}

void RenderableMesh::end()
{
	std::shared_ptr<RenderableMesh> l_pThis = shared_from_base<RenderableMesh>();
	SharedSceneMember *l_pMembers = getSharedMember();
	
	if( !isHidden() ) l_pMembers->m_pGraphs[m_bStatic ? SharedSceneMember::GRAPH_STATIC_MESH : SharedSceneMember::GRAPH_MESH]->remove(l_pThis);
	l_pMembers->m_pBatcher->recycleWorldSlot(l_pThis);
	if( -1 != m_SkinOffset ) l_pMembers->m_pBatcher->recycleSkinSlot(m_pMesh);

	for( auto it=m_Materials.begin() ; it!=m_Materials.end() ; ++it )
	{
		it->second.second->getComponent<MaterialAsset>()->freeInstanceSlot(it->second.first);
	}
	m_Materials.clear();
}

void RenderableMesh::hiddenFlagChanged()
{
	if( isHidden() )
	{
		getSharedMember()->m_pGraphs[m_bStatic ? SharedSceneMember::GRAPH_STATIC_MESH : SharedSceneMember::GRAPH_MESH]->remove(shared_from_base<RenderableMesh>());
		removeTransformListener();
	}
	else
	{
		getSharedMember()->m_pGraphs[m_bStatic ? SharedSceneMember::GRAPH_STATIC_MESH : SharedSceneMember::GRAPH_MESH]->add(shared_from_base<RenderableMesh>());
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

	getSharedMember()->m_pGraphs[m_bStatic ? SharedSceneMember::GRAPH_STATIC_MESH : SharedSceneMember::GRAPH_MESH]->update(shared_from_base<RenderableMesh>());
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
	if( m_bStatic )
	{
		getSharedMember()->m_pGraphs[SharedSceneMember::GRAPH_STATIC_MESH]->remove(shared_from_base<RenderableMesh>());
		getSharedMember()->m_pGraphs[SharedSceneMember::GRAPH_MESH]->add(shared_from_base<RenderableMesh>());
	}
	else
	{
		getSharedMember()->m_pGraphs[SharedSceneMember::GRAPH_MESH]->remove(shared_from_base<RenderableMesh>());
		getSharedMember()->m_pGraphs[SharedSceneMember::GRAPH_STATIC_MESH]->add(shared_from_base<RenderableMesh>());
	}
	m_bStatic = a_bStatic;
}

void RenderableMesh::setMesh(std::shared_ptr<Asset> a_pAsset, unsigned int a_MeshIdx)
{
	getSharedMember()->m_pBatcher->requestSkinSlot(m_pMesh);
	m_pMesh = a_pAsset;
	m_MeshIdx = a_MeshIdx;
	
	m_SkinOffset = getSharedMember()->m_pBatcher->requestSkinSlot(m_pMesh);

	MeshAsset::Instance *l_pMeshInst = a_pAsset->getComponent<MeshAsset>()->getMeshes()[m_MeshIdx];
	setMaterial(0, l_pMeshInst->m_pMaterial);

	for( auto it=m_Materials.begin() ; it!=m_Materials.end() ; ++it )
	{
		if( 0 == it->first ) continue;
		it->second.second->getComponent<MaterialAsset>()->setParam("m_SkinMatBase", m_WorldOffset, m_SkinOffset);
	}

	glm::vec3 l_Trans, l_Scale;
	glm::quat l_Rot;
	glm::aabb &l_Box = l_pMeshInst->m_VisibleBoundingBox;
	decomposeTRS(getSharedMember()->m_pSceneNode->getTransform(), l_Trans, l_Scale, l_Rot);
	boundingBox().m_Center = l_Trans + l_Box.m_Center;
	boundingBox().m_Size = l_Scale * l_Box.m_Size;
}

void RenderableMesh::removeMaterial(unsigned int a_Slot)
{
	auto it = m_Materials.find(a_Slot);
	if( m_Materials.end() == it ) return;
	
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
	
	l_pMaterialInst->setParam("m_World", m_WorldOffset, getSharedMember()->m_pSceneNode->getTransform());
	l_pMaterialInst->setParam("m_VtxFlag", m_WorldOffset, l_pMeshInst->m_VtxFlag);
	l_pMaterialInst->setParam("m_SkinMatBase", m_WorldOffset, m_SkinOffset);

	l_pMaterialInst->setBlock("m_SkinTransition", getSharedMember()->m_pBatcher->getSkinMatrixBlock());
	l_pMaterialInst->setBlock("m_NormalTransition", getSharedMember()->m_pBatcher->getWorldMatrixBlock());
}

std::shared_ptr<Asset> RenderableMesh::getMaterial(unsigned int a_Slot)
{
	auto it = m_Materials.find(a_Slot);
	return m_Materials.end() == it ? nullptr : it->second.second;
}
#pragma endregion

}