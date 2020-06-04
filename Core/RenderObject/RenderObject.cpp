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
// ModelCache
//
ModelCache::ModelCache()
{
}

ModelCache::~ModelCache()
{
	clearCache();
}

std::shared_ptr<ModelCache::Instance> ModelCache::loadMesh(wxString a_Filename)
{
	bool l_bNeedInitVertex = false;
	std::shared_ptr<ModelData> l_pModel = ModelManager::singleton().getData(a_Filename).second;
	std::shared_ptr<Instance> l_pRes = getInstance(a_Filename, l_pModel, l_bNeedInitVertex);

	if( l_bNeedInitVertex ) initMeshes(l_pRes, l_pModel);
	return l_pRes;
}


void ModelCache::clearCache()
{
	m_FileCache.clear();
	ModelManager::singleton().gc();
}

std::shared_ptr<ModelCache::Instance> ModelCache::getInstance(wxString a_Filename, std::shared_ptr<ModelData> a_pSrc, bool &a_bNeedInitInstance)
{
	std::lock_guard<std::mutex> l_Guard(m_CacheLock);
	
	std::shared_ptr<Instance> l_pRes = nullptr;

	auto it = m_FileCache.find(a_Filename);
	if( m_FileCache.end() == it )
	{
		a_bNeedInitInstance = true;
		
		l_pRes = std::shared_ptr<Instance>(new Instance);
		l_pRes->m_pModel = ModelManager::singleton().getData(a_Filename).second;
		l_pRes->m_pVtxBuffer = std::shared_ptr<VertexBuffer>(new VertexBuffer());
		l_pRes->m_pVtxBuffer->setName(a_Filename + wxT("_Vertex"));
		l_pRes->m_pIdxBuffer = std::shared_ptr<IndexBuffer>(new IndexBuffer());
		l_pRes->m_pIdxBuffer->setName(a_Filename + wxT("_Index"));
		
		m_FileCache.insert(std::make_pair(a_Filename, l_pRes));
	}
	else l_pRes = it->second;

	return l_pRes;
}

void ModelCache::initMeshes(std::shared_ptr<Instance> a_pInst, std::shared_ptr<ModelData> a_pSrc)
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
		IndirectDrawData l_PartData;
		l_PartData.m_InstanceCount = 0;
		l_PartData.m_StartInstance = 0;

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

		unsigned int l_MeshIndiciesCount = l_Meshes[i]->m_Indicies.size();
		memcpy(&(l_TempIndicies[l_BaseIndex]), l_Meshes[i]->m_Indicies.data(), sizeof(unsigned int) * l_MeshIndiciesCount);
		
		l_PartData.m_BaseVertex = l_BaseVtx;
		l_PartData.m_StartIndex = l_BaseIndex;
		l_PartData.m_IndexCount = l_MeshIndiciesCount;
		a_pInst->m_SubMeshes.insert(std::make_pair(l_Meshes[i]->m_Name, l_PartData));

		l_BaseVtx += l_MeshVtxCount;
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
	a_pInst->m_pIdxBuffer->init();
}
#pragma endregion

}