// RenderObject.cpp
//
// 2017/07/19 Ian Wu/Real0000
//
#include "CommonUtil.h"
#include "RGDeviceWrapper.h"
#include "RImporters.h"

#include "Core.h"
#include "RenderObject.h"
#include "Material.h"
#include "Mesh.h"

#include "Scene/Scene.h"
#include "Scene/Graph/ScenePartition.h"

namespace R
{

#pragma region RenderableComponent
//
// RenderableComponent
//
RenderableComponent::RenderableComponent(SharedSceneMember *a_pSharedMember, std::shared_ptr<SceneNode> a_pOwner)
	: EngineComponent(a_pSharedMember, a_pOwner)
{
}

RenderableComponent::~RenderableComponent()
{
}
#pragma endregion

#pragma region
//
// ModelComponentFactory
//
ModelComponentFactory::ModelComponentFactory(SharedSceneMember *a_pSharedMember)
	: m_pSharedMember(new SharedSceneMember)
{
	*m_pSharedMember = *a_pSharedMember;
}

ModelComponentFactory::~ModelComponentFactory()
{
	delete m_pSharedMember;
	assert(m_FileCache.empty());
}

void ModelComponentFactory::createMesh(std::shared_ptr<SceneNode> a_pOwner, wxString a_Filename, std::shared_ptr<Material> a_pMaterial, std::function<void()> a_pCallback)
{
	if( nullptr != a_pCallback ) std::thread l_Thread(&ModelComponentFactory::loadMesh, this, a_pOwner, a_Filename, a_pMaterial, a_pCallback);
	else loadMesh(a_pOwner, a_Filename, a_pMaterial, nullptr);
}

void ModelComponentFactory::createMesh(std::shared_ptr<SceneNode> a_pOwner, wxString a_Filename, std::function<void()> a_pCallback)
{
	if( nullptr != a_pCallback ) std::thread l_Thread(&ModelComponentFactory::loadSetting, this, a_pOwner, a_Filename, a_pCallback);
	else loadSetting(a_pOwner, a_Filename, nullptr);
}

std::shared_ptr<RenderableMesh> ModelComponentFactory::createSphere(std::shared_ptr<SceneNode> a_pOwner, std::shared_ptr<Material> a_pMaterial)
{
	return nullptr;
}

std::shared_ptr<RenderableMesh> ModelComponentFactory::createBox(std::shared_ptr<SceneNode> a_pOwner, std::shared_ptr<Material> a_pMaterial)
{
	return nullptr;
}

void ModelComponentFactory::clearCache()
{
	m_FileCache.clear();
	ModelManager::singleton().clearCache();
}

void ModelComponentFactory::loadMesh(std::shared_ptr<SceneNode> a_pOwner, wxString a_Filename, std::shared_ptr<Material> a_pMaterial, std::function<void()> a_pCallback)
{
	bool l_bNeedInitVertex = false;
	std::shared_ptr<ModelData> l_pModel = ModelManager::singleton().getData(a_Filename).second;
	std::shared_ptr<Instance> l_pRes = getInstance(a_Filename, l_pModel, l_bNeedInitVertex);

	if( l_bNeedInitVertex ) initMeshes(l_pRes, l_pModel);

	std::list<std::shared_ptr<RenderableMesh> > l_MeshComponents;
	initNodes(a_pOwner, l_pRes, l_pModel, l_MeshComponents);
	for( auto it = l_MeshComponents.begin() ; it != l_MeshComponents.end() ; ++it ) (*it)->setMaterial(a_pMaterial);

	if( nullptr != a_pCallback ) a_pCallback();
}

void ModelComponentFactory::loadSetting(std::shared_ptr<SceneNode> a_pOwner, wxString a_Filename, std::function<void()> a_pCallback)
{
}

std::shared_ptr<ModelComponentFactory::Instance> ModelComponentFactory::getInstance(wxString a_Filename, std::shared_ptr<ModelData> a_pSrc, bool &a_bNeedInitInstance)
{
	std::lock_guard<std::mutex> l_Guard(m_CacheLock);
	
	std::shared_ptr<Instance> l_pRes = nullptr;

	auto it = m_FileCache.find(a_Filename);
	if( m_FileCache.end() == it )
	{
		a_bNeedInitInstance = true;

		std::shared_ptr<ModelData> l_pModel = ModelManager::singleton().getData(a_Filename).second;
		l_pRes = std::shared_ptr<Instance>(new Instance);
		l_pRes->m_pVtxBuffer = std::shared_ptr<VertexBuffer>(new VertexBuffer());
		l_pRes->m_pVtxBuffer->setName(a_Filename);
		l_pRes->m_pIdxBuffer = std::shared_ptr<IndexBuffer>(new IndexBuffer());
		l_pRes->m_pIdxBuffer->setName(a_Filename);
		auto &l_Meshes = l_pModel->getMeshes();
			
		unsigned int l_VertexCount = 0;
		l_pRes->m_Meshes.resize(l_Meshes.size());
		for( unsigned int i=0 ; i<l_Meshes.size() ; ++i )
		{
			l_pRes->m_Meshes[i].first = l_Meshes[i]->m_Indicies.size();
			l_pRes->m_Meshes[i].second = l_VertexCount;
			l_VertexCount += l_Meshes[i]->m_Vertex.size();
		}
		m_FileCache.insert(std::make_pair(a_Filename, l_pRes));
	}
	else l_pRes = it->second;

	return l_pRes;
}

void ModelComponentFactory::initMeshes(std::shared_ptr<Instance> a_pInst, std::shared_ptr<ModelData> a_pSrc)
{
	unsigned int l_VertexCount = 0;
	unsigned int l_IndiciesCount = 0;
	auto &l_Meshes = a_pSrc->getMeshes();
	for( unsigned int i=0 ; i<l_Meshes.size() ; ++i )
	{
		l_VertexCount += l_Meshes[i]->m_Vertex.size();
		l_IndiciesCount += l_Meshes[i]->m_Indicies.size();
	}

	std::vector<glm::vec3> l_TempPos(l_VertexCount);
	std::vector<glm::vec4> l_TempUV[4]{std::vector<glm::vec4>(l_VertexCount), std::vector<glm::vec4>(l_VertexCount), std::vector<glm::vec4>(l_VertexCount), std::vector<glm::vec4>(l_VertexCount)};
    std::vector<glm::vec3> l_TempNormal(l_VertexCount);
    std::vector<glm::vec3> l_TempTangent(l_VertexCount);
    std::vector<glm::vec3> l_TempBinormal(l_VertexCount);
	std::vector<glm::ivec4> l_TempBoneId(l_VertexCount);
	std::vector<glm::vec4> l_TempWeight(l_VertexCount);
	std::vector<unsigned int> l_TempIndicies(l_IndiciesCount);

	a_pInst->m_pVtxBuffer->setNumVertex(l_VertexCount);

	unsigned int l_BaseVtx = 0;
	unsigned int l_BaseIndex = 0;
	for( unsigned int i=0 ; i<l_Meshes.size() ; ++i )
	{
		unsigned int l_MeshVtxCount = l_Meshes[i]->m_Vertex.size();
		for( unsigned int j=0 ; j<l_MeshVtxCount ; ++j )
		{
			unsigned int l_VtxIdx = l_BaseVtx + j;
			ModelData::Vertex &l_SrcVtx = l_Meshes[i]->m_Vertex[j];
			l_TempPos[l_VtxIdx] = l_SrcVtx.m_Position;
			l_TempUV[0][l_VtxIdx] = l_SrcVtx.m_Texcoord[0];
			l_TempUV[1][l_VtxIdx] = l_SrcVtx.m_Texcoord[1];
			l_TempUV[2][l_VtxIdx] = l_SrcVtx.m_Texcoord[2];
			l_TempUV[3][l_VtxIdx] = l_SrcVtx.m_Texcoord[3];
			l_TempNormal[l_VtxIdx] = l_SrcVtx.m_Normal;
			l_TempTangent[l_VtxIdx] = l_SrcVtx.m_Tangent;
			l_TempBinormal[l_VtxIdx] = l_SrcVtx.m_Binormal;
			l_TempBoneId[l_VtxIdx] = l_SrcVtx.m_BoneId;
			l_TempWeight[l_VtxIdx] = l_SrcVtx.m_Weight;
		}
		l_BaseVtx += l_MeshVtxCount;

		unsigned int l_MeshIndiciesCount = l_Meshes[i]->m_Indicies.size();
		memcpy(&(l_TempIndicies[l_BaseIndex]), l_Meshes[i]->m_Indicies.data(), sizeof(unsigned int) * l_MeshIndiciesCount);
		l_BaseIndex += l_MeshIndiciesCount;
	}
	a_pInst->m_pVtxBuffer->setVertex(VTXSLOT_POSITION, l_TempPos.data());
	a_pInst->m_pVtxBuffer->setVertex(VTXSLOT_TEXCOORD01, l_TempUV[0].data());
	a_pInst->m_pVtxBuffer->setVertex(VTXSLOT_TEXCOORD23, l_TempUV[1].data());
	a_pInst->m_pVtxBuffer->setVertex(VTXSLOT_TEXCOORD45, l_TempUV[2].data());
	a_pInst->m_pVtxBuffer->setVertex(VTXSLOT_TEXCOORD67, l_TempUV[3].data());
	a_pInst->m_pVtxBuffer->setVertex(VTXSLOT_NORMAL, l_TempNormal.data());
	a_pInst->m_pVtxBuffer->setVertex(VTXSLOT_TANGENT, l_TempTangent.data());
	a_pInst->m_pVtxBuffer->setVertex(VTXSLOT_BINORMAL, l_TempBinormal.data());
	a_pInst->m_pVtxBuffer->setVertex(VTXSLOT_BONE, l_TempBoneId.data());
	a_pInst->m_pVtxBuffer->setVertex(VTXSLOT_WEIGHT, l_TempWeight.data());
	a_pInst->m_pVtxBuffer->init();

	a_pInst->m_pIdxBuffer->setIndicies(true, l_TempIndicies.data(), l_IndiciesCount);
}

void ModelComponentFactory::initNodes(std::shared_ptr<SceneNode> a_pOwner, std::shared_ptr<Instance> a_pInst, std::shared_ptr<ModelData> a_pSrc, std::list<std::shared_ptr<RenderableMesh> > &a_OutputMeshComponent)
{
	std::list<ModelNode *> l_SrcContainer(1, a_pSrc->getRootNode());
	std::list< std::shared_ptr<SceneNode> > l_DstContainer(1, SceneNode::create(m_pSharedMember, a_pOwner, a_pSrc->getRootNode()->getName()));

	auto &l_SrcMeshes = a_pSrc->getMeshes();
	auto l_SrcIt = l_SrcContainer.begin();
	auto l_DstIt = l_DstContainer.begin();
	for( ; l_SrcIt != l_SrcContainer.end() ; ++l_SrcIt, ++l_DstIt )
	{
		for( unsigned int i=0 ; i<(*l_SrcIt)->getChildren().size() ;++i )
		{
			ModelNode *l_pChildSrcNode = (*l_SrcIt)->getChildren()[i];
			l_SrcContainer.push_back(l_pChildSrcNode);
			l_DstContainer.push_back(SceneNode::create(m_pSharedMember, *l_DstIt, l_pChildSrcNode->getName()));
		}

		for( unsigned int i=0 ; i<(*l_SrcIt)->getRefMesh().size() ; ++i )
		{
			unsigned int l_MeshIdx = (*l_SrcIt)->getRefMesh()[i];
			std::shared_ptr<RenderableMesh> l_pNewRenderMesh = EngineComponent::create<RenderableMesh>(m_pSharedMember, *l_DstIt);
			l_pNewRenderMesh->setMeshData(a_pInst->m_pVtxBuffer, a_pInst->m_pIdxBuffer, a_pInst->m_Meshes[l_MeshIdx], l_SrcMeshes[l_MeshIdx]->m_BoxSize);
			a_OutputMeshComponent.push_back(l_pNewRenderMesh);
		}
	}
}
#pragma endregion

}