// WorldAsset.cpp
//
// 2020/09/11 Ian Wu/Real0000
//

#include "CommonUtil.h"
#include "RGDeviceWrapper.h"
#include "RImporters.h"

#include "Core.h"
#include "MaterialAsset.h"
#include "MeshAsset.h"
#include "RenderObject/Light.h"
#include "RenderObject/Mesh.h"
#include "Scene/Graph/ScenePartition.h"
#include "Scene/Scene.h"
#include "WorldAsset.h"

namespace R
{

#define LIGHTMAP_INTERSECT_ASSET_NAME wxT("LightmapIntersection.Material")

#pragma region WorldAsset
//
// WorldAsset
//
WorldAsset::WorldAsset()
	: m_bBaking(false)
	, m_pRayIntersectMat(AssetManager::singleton().createAsset(LIGHTMAP_INTERSECT_ASSET_NAME).second)
	, m_pRayIntersectMatInst(nullptr)
	, m_pTriangles(nullptr), m_pLightIdx(nullptr), m_pVertex(nullptr), m_pResult(nullptr), m_pBoxes(nullptr)
{
	m_pRayIntersectMatInst = m_pRayIntersectMat->getComponent<MaterialAsset>();
	m_pRayIntersectMatInst->init(ProgramManager::singleton().getData(DefaultPrograms::RayIntersection));
}

WorldAsset::~WorldAsset()
{
	stopBake();
}

void WorldAsset::loadFile(boost::property_tree::ptree &a_Src)
{
}

void WorldAsset::saveFile(boost::property_tree::ptree &a_Dst)
{
}

void WorldAsset::bake(SharedSceneMember *a_pMember)
{
	stopBake();

	unsigned int l_LightCount = 0;

	std::vector<std::shared_ptr<RenderableComponent>> l_Meshes, l_Lights;
	a_pMember->m_pGraphs[SharedSceneMember::GRAPH_STATIC_MESH]->getAllComponent(l_Meshes);
	a_pMember->m_pGraphs[SharedSceneMember::GRAPH_STATIC_LIGHT]->getAllComponent(l_Lights);

	std::vector<unsigned int> l_TempTriangleData;
	std::vector<LightMapVtxSrc> l_TempVertexData;
	std::map<std::shared_ptr<Asset>, unsigned int> l_TempMatSet;
	glm::daabb l_Bounding;

	// init max bounding, vtx, index, material
	for( unsigned int i=0 ; i<l_Meshes.size() ; ++i )
	{
		if( l_Meshes[i]->isHidden() ) continue;

		RenderableMesh *l_pMeshObj = reinterpret_cast<RenderableMesh*>(l_Meshes[i].get());
		std::shared_ptr<Asset> l_pMatAsset = l_pMeshObj->getMaterial(MaterialSlot::MATSLOT_LIGHTMAP);
		auto it = l_TempMatSet.find(l_pMatAsset);
		unsigned int l_MatID = 0;
		if( l_TempMatSet.end() == it )
		{
			l_MatID = l_TempMatSet.size();
			l_TempMatSet.insert(std::make_pair(l_pMatAsset, l_MatID));
		}
		else l_MatID = it->second;
		
		std::shared_ptr<Asset> l_pMeshAssetObj = l_pMeshObj->getMesh();
		MeshAsset *l_pMeshAssetInst = l_pMeshAssetObj->getComponent<MeshAsset>();
		MeshAsset::Instance *l_pSubMeshInfo = l_pMeshAssetInst->getMeshes()[l_pMeshObj->getMeshIdx()];
		 
		auto &l_Indicies = l_pMeshAssetInst->getIndicies();
		auto &l_Position = l_pMeshAssetInst->getPosition();
		auto &l_Normal = l_pMeshAssetInst->getNormal();
		auto &l_UV = l_pMeshAssetInst->getTexcoord(0);

		unsigned int l_MaxIdx = 0;
		unsigned int l_Base = l_TempVertexData.size();
		for( unsigned int j=0 ; j<l_pSubMeshInfo->m_IndexCount ; ++j )
		{
			unsigned int l_Idx = l_pSubMeshInfo->m_StartIndex + j;
			l_MaxIdx = std::max(l_Indicies[l_Idx], l_MaxIdx);

			l_TempTriangleData.push_back(l_Indicies[l_Idx] + l_Base);
			if( j % 3 == 2 ) l_TempTriangleData.push_back(l_MatID);
		}
		
		for( unsigned int j=0 ; j<l_MaxIdx ; ++j )
		{
			unsigned int l_VtxIdx = j + l_pSubMeshInfo->m_BaseVertex;

			glm::mat4x4 l_Matrix(l_pMeshObj->getOwnerNode()->getTransform());
			glm::vec4 l_WorldPos(glm::vec4(l_Position[l_VtxIdx], 1.0) * l_Matrix);
			glm::vec4 l_WorldNormal(glm::vec4(l_Normal[l_VtxIdx], 0.0) * l_Matrix);

			LightMapVtxSrc l_Temp;
			l_Temp.m_Position = glm::vec3(l_WorldPos.x, l_WorldPos.y, l_WorldPos.z);
			l_Temp.m_Normal = glm::vec3(l_WorldNormal.x, l_WorldNormal.y, l_WorldNormal.z);
			l_Temp.m_U = l_UV[l_VtxIdx].x;
			l_Temp.m_V = l_UV[l_VtxIdx].y;
			l_TempVertexData.push_back(l_Temp);

			l_Bounding.m_Size.x = std::max<double>(l_Bounding.m_Size.x * 0.5, l_Temp.m_Position.x) * 2.0;
			l_Bounding.m_Size.y = std::max<double>(l_Bounding.m_Size.y * 0.5, l_Temp.m_Position.x) * 2.0;
			l_Bounding.m_Size.z = std::max<double>(l_Bounding.m_Size.z * 0.5, l_Temp.m_Position.x) * 2.0;
		}
	}
	l_Bounding.m_Size.x = std::pow(2.0, std::ceil(std::log(l_Bounding.m_Size.x * 0.5))) * 2.0;
	l_Bounding.m_Size.y = std::pow(2.0, std::ceil(std::log(l_Bounding.m_Size.y * 0.5))) * 2.0;
	l_Bounding.m_Size.z = std::pow(2.0, std::ceil(std::log(l_Bounding.m_Size.z * 0.5))) * 2.0;

	std::vector<glm::ivec2> l_TempLightData;
	for( unsigned int i=0 ; i<l_Lights.size() ; ++i )
	{
	}
	//m_pLightIdxBlock = m_pRayIntersectMatInst->createExternalBlock(ShaderRegType::UavBuffer, 

	m_pRayIntersectMatInst->setBlock("g_DirLights", a_pMember->m_pDirLights->getMaterialBlock());
	m_pRayIntersectMatInst->setBlock("g_OmniLights", a_pMember->m_pOmniLights->getMaterialBlock());
	m_pRayIntersectMatInst->setBlock("g_SpotLights", a_pMember->m_pSpotLights->getMaterialBlock());
	/*"g_Triangles"
    "g_Lights"
    "g_Box"
    "g_Vertex"
	"g_Result"*/
}

void WorldAsset::stopBake()
{
	if( !m_bBaking ) return;

	m_pTriangles = nullptr;
	m_pLightIdx = nullptr;
	m_pVertex = nullptr;
	m_pResult = nullptr;
	m_pBoxes = nullptr;
	m_LightMap.clear();
}
#pragma endregion

}