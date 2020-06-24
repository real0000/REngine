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
	, m_pMaterial(nullptr)
	, m_InstanceID(-1)
	, m_bStatic(false)
	, m_bShadowed(true)
{
}

RenderableMesh::~RenderableMesh()
{
}

void RenderableMesh::start()
{
	addTransformListener();
	getSharedMember()->m_pGraphs[m_bStatic ? SharedSceneMember::GRAPH_STATIC_MESH : SharedSceneMember::GRAPH_MESH]->add(shared_from_base<RenderableMesh>());
}

void RenderableMesh::end()
{
	if( isHidden() ) return;
	
	SharedSceneMember *l_pMembers = getSharedMember();
	std::shared_ptr<RenderableMesh> l_pThis = shared_from_base<RenderableMesh>();

	l_pMembers->m_pGraphs[SharedSceneMember::GRAPH_MESH]->remove(l_pThis);
	//l_pMembers->m_pBatcher->remove(l_pThis);
}

void RenderableMesh::hiddenFlagChanged()
{
	if( isHidden() )
	{
		getSharedMember()->m_pGraphs[SharedSceneMember::GRAPH_MESH]->remove(shared_from_base<RenderableMesh>());
		removeTransformListener();
	}
	else
	{
		getSharedMember()->m_pGraphs[SharedSceneMember::GRAPH_MESH]->add(shared_from_base<RenderableMesh>());
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
	m_pMesh = a_pAsset;
	m_MeshIdx = a_MeshIdx;

	MeshAsset::Instance *l_pMeshInst = a_pAsset->getComponent<MeshAsset>()->getMeshes()[m_MeshIdx];
	setMaterial(l_pMeshInst->m_pMaterial);

	glm::vec3 l_Trans, l_Scale;
	glm::quat l_Rot;
	glm::aabb &l_Box = l_pMeshInst->m_VisibleBoundingBox;
	decomposeTRS(getSharedMember()->m_pSceneNode->getTransform(), l_Trans, l_Scale, l_Rot);
	boundingBox().m_Center = l_Trans + l_Box.m_Center;
	boundingBox().m_Size = l_Scale * l_Box.m_Size;
}

void RenderableMesh::setMaterial(std::shared_ptr<Asset> a_pAsset)
{
	if( nullptr == m_pMesh ) return;

	if( nullptr != m_pMaterial )
	{
		m_pMaterial->getComponent<MaterialAsset>()->freeInstanceSlot(m_InstanceID);
		m_pMaterial = a_pAsset;
		getSharedMember()->m_pScene->recycleSkinSlot(m_pMesh);
	}
	
	MeshAsset *l_pMeshRootInst = a_pAsset->getComponent<MeshAsset>();
	MeshAsset::Instance *l_pMeshInst = l_pMeshRootInst->getMeshes()[m_MeshIdx];
	MaterialAsset* l_pMaterialInst = m_pMaterial->getComponent<MaterialAsset>();
	m_InstanceID = l_pMaterialInst->requestInstanceSlot();
	if( -1 != m_InstanceID )
	{
		l_pMaterialInst->setParam("m_World", m_InstanceID, getSharedMember()->m_pSceneNode->getTransform());
		l_pMaterialInst->setParam("m_Local", m_InstanceID, glm::mat4x4(1.0f));
		l_pMaterialInst->setParam("m_VtxFlag", m_InstanceID, l_pMeshInst->m_VtxFlag);
		if( !l_pMeshRootInst->getBones().empty() )
		{
			int l_Slot = getSharedMember()->m_pScene->requestSkinSlot(a_pAsset);
			l_pMaterialInst->setParam("m_SkinMatBase", m_InstanceID, l_Slot);
			l_pMaterialInst->setBlock("m_SkinTransition", getSharedMember()->m_pScene->getSkinMatrixBlock());
		}
		else l_pMaterialInst->setParam("m_SkinMatBase", m_InstanceID, 0);
	}
}
#pragma endregion

}