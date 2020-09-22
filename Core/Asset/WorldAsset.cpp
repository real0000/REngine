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
#include "Scene/Graph/Octree.h"
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
	std::lock_guard<std::mutex> l_Locker(m_BakeLock);

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
	{
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
	}

	// create boxes, assign triangles
	LightMapBoxCache *l_pRoot = new LightMapBoxCache();
	{
		l_pRoot->m_Box = l_Bounding;
		l_pRoot->m_pParent = nullptr;
		memset(l_pRoot->m_pChildren, 0, sizeof(LightMapBoxCache *) * 8);
		for( unsigned int i=0 ; i<l_TempTriangleData.size() ; i+=3 )
		{
			std::set<LightMapBoxCache *> l_IntersectBox;
			l_IntersectBox.clear();
			assignTriangle(l_TempVertexData[i].m_Position, l_TempVertexData[i+1].m_Position, l_TempVertexData[i+2].m_Position, l_pRoot, l_IntersectBox);
			for( auto it=l_IntersectBox.begin() ; it!=l_IntersectBox.end() ; ++it ) (*it)->m_Triangles.push_back(i);
		}
	}

	// assign lights
	for( unsigned int i=0 ; i<l_Lights.size() ; ++i )
	{
		if( l_Lights[i]->isHidden() ) continue;

		Light *l_pLightObj = reinterpret_cast<Light*>(l_Lights[i].get());
		
		std::vector<LightMapBoxCache*> l_IntersectBox;
		assignLight(l_pLightObj, l_pRoot, l_IntersectBox);

		glm::ivec2 l_TempData;
		l_TempData.x = l_pLightObj->typeID();
		l_TempData.y = l_pLightObj->getID();
		for( unsigned int j=0 ; j<l_IntersectBox.size() ; ++j ) l_IntersectBox[j]->m_Lights.push_back(l_TempData);
	}

	// LightMapBoxCache -> LightMapBox, init uav triangle/light data
	std::vector<glm::ivec4> l_UavTriangleData(1, glm::ivec4(0, 0, 0, 0));
	std::vector<glm::ivec2> l_UavLightData;
	{
		std::vector<LightMapBoxCache *> l_CacheList(1, l_pRoot);
		std::map<LightMapBoxCache *, int> l_ParentMap;
		for( unsigned int i=0 ; i<l_CacheList.size() ; ++i )
		{
			LightMapBoxCache *l_pThisNode = l_CacheList[i];
			l_ParentMap.insert(std::make_pair(l_pThisNode, i));

			LightMapBox l_Temp;
			l_Temp.m_BoxCenter = l_pThisNode->m_Box.m_Center;
			l_Temp.m_BoxSize = l_pThisNode->m_Box.m_Size;
			l_Temp.m_Parent = 0 == i ? -1 : l_ParentMap[l_pThisNode->m_pParent];
			
			for( unsigned int j=0 ; j<8 ; ++j )
			{
				if( nullptr == l_pThisNode->m_pChildren[j] )
				{
					l_Temp.m_Children[j] = -1;
					continue;
				}
				
				l_Temp.m_Children[j] = l_CacheList.size();
				l_CacheList.push_back(l_pThisNode->m_pChildren[j]);
			}

			l_Temp.m_LightRange.x = l_UavLightData.size();
			l_Temp.m_LightRange.y = l_pThisNode->m_Lights.size();
			for( unsigned int j=0 ; j<l_Temp.m_LightRange.y ; ++j ) l_UavLightData.push_back(l_pThisNode->m_Lights[j]);

			l_Temp.m_TriangleRange.x = l_UavTriangleData.size();
			l_Temp.m_TriangleRange.y = l_pThisNode->m_Triangles.size() / 4;
			for( unsigned int j=0 ; j<l_Temp.m_TriangleRange.y ; ++j )
			{
				glm::ivec4 l_Triangle(l_pThisNode->m_Triangles[j*4], l_pThisNode->m_Triangles[j*4 + 1], l_pThisNode->m_Triangles[j*4 + 2], l_pThisNode->m_Triangles[j*4 + 3]);
				l_UavTriangleData.push_back(l_Triangle);
			}
			l_Temp.m_TriangleRange.y += l_Temp.m_TriangleRange.x;

			memset(l_Temp.m_SHResult, 0, sizeof(glm::vec4) * 1024);

			m_LightMap.push_back(l_Temp);
			delete l_pThisNode;
		}
		l_UavTriangleData[0].z = l_CacheList.size();
		l_UavTriangleData[0].w = EngineSetting::singleton().m_LightMapSample;
	}

	m_pTriangles = m_pRayIntersectMatInst->createExternalBlock(ShaderRegType::UavBuffer, "g_Triangles", l_UavTriangleData.size());
	memcpy(m_pTriangles->getBlockPtr(0), l_UavTriangleData.data(), sizeof(glm::ivec4) * l_UavTriangleData.size());
	m_pTriangles->sync(true);

	m_pLightIdx = m_pRayIntersectMatInst->createExternalBlock(ShaderRegType::UavBuffer, "g_Lights", l_UavLightData.size() / 2);
	memcpy(m_pLightIdx->getBlockPtr(0), l_UavLightData.data(), sizeof(glm::ivec2) * l_UavLightData.size());
	m_pLightIdx->sync(true);

	m_pBoxes = m_pRayIntersectMatInst->createExternalBlock(ShaderRegType::UavBuffer, "g_Box", m_LightMap.size());
	memcpy(m_pBoxes->getBlockPtr(0), m_LightMap.data(), sizeof(LightMapBox) * m_LightMap.size());
	m_pBoxes->sync(true);

	m_pVertex = m_pRayIntersectMatInst->createExternalBlock(ShaderRegType::UavBuffer, "g_Vertex", l_TempVertexData.size());
	memcpy(m_pVertex->getBlockPtr(0), l_TempVertexData.data(), sizeof(LightMapVtxSrc) * l_TempVertexData.size());
	m_pVertex->sync(true);
	
	m_pResult = m_pRayIntersectMatInst->createExternalBlock(ShaderRegType::UavBuffer, "g_Result", EngineSetting::singleton().m_LightMapSample);
	memset(m_pResult->getBlockPtr(0), 0, sizeof(LightIntersectResult) * EngineSetting::singleton().m_LightMapSample);
	m_pResult->sync(true);
	
	m_pRayIntersectMatInst->setBlock("g_Triangles", m_pTriangles);
	m_pRayIntersectMatInst->setBlock("g_Lights", m_pLightIdx);
	m_pRayIntersectMatInst->setBlock("g_DirLights", a_pMember->m_pDirLights->getMaterialBlock());
	m_pRayIntersectMatInst->setBlock("g_OmniLights", a_pMember->m_pOmniLights->getMaterialBlock());
	m_pRayIntersectMatInst->setBlock("g_SpotLights", a_pMember->m_pSpotLights->getMaterialBlock());
	m_pRayIntersectMatInst->setBlock("g_Box", m_pBoxes);
	m_pRayIntersectMatInst->setBlock("g_Result", m_pResult);

	m_bBaking = true;
}

void WorldAsset::stepBake(GraphicCommander *a_pCmd)
{
	//a_pCmd->useProgram(m_p);
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

	m_bBaking = false;
}

void WorldAsset::assignTriangle(glm::vec3 &a_Pos1, glm::vec3 &a_Pos2, glm::vec3 &a_Pos3, LightMapBoxCache *a_pCurrNode, std::set<LightMapBoxCache*> &a_Output)
{
	a_Output.insert(a_pCurrNode);
	if( a_pCurrNode->m_Box.m_Size.x - DEFAULT_OCTREE_EDGE < FLT_EPSILON ) return;

	for( unsigned int i=0 ; i<8 ; ++i )
	{
		glm::daabb l_Box;
		if( nullptr == a_pCurrNode->m_pChildren[i] )
		{
			l_Box.m_Size = a_pCurrNode->m_Box.m_Size * 0.5;
			glm::dvec3 l_Edge;
			switch(i)
			{
				case OctreePartition::NX_NY_NZ:
					l_Edge = a_pCurrNode->m_Box.getMin();
					break;

				case OctreePartition::NX_NY_PZ:
					l_Edge = glm::dvec3(a_pCurrNode->m_Box.getMinX(), a_pCurrNode->m_Box.getMinY(), a_pCurrNode->m_Box.getMaxZ());
					break;

				case OctreePartition::NX_PY_NZ:
					l_Edge = glm::dvec3(a_pCurrNode->m_Box.getMinX(), a_pCurrNode->m_Box.getMaxY(), a_pCurrNode->m_Box.getMinZ());
					break;

				case OctreePartition::NX_PY_PZ:
					l_Edge = glm::dvec3(a_pCurrNode->m_Box.getMinX(), a_pCurrNode->m_Box.getMaxY(), a_pCurrNode->m_Box.getMaxZ());
					break;

				case OctreePartition::PX_NY_NZ:
					l_Edge = glm::dvec3(a_pCurrNode->m_Box.getMaxX(), a_pCurrNode->m_Box.getMinY(), a_pCurrNode->m_Box.getMinZ());
					break;

				case OctreePartition::PX_NY_PZ:
					l_Edge = glm::dvec3(a_pCurrNode->m_Box.getMaxX(), a_pCurrNode->m_Box.getMinY(), a_pCurrNode->m_Box.getMaxZ());
					break;

				case OctreePartition::PX_PY_NZ:
					l_Edge = glm::dvec3(a_pCurrNode->m_Box.getMaxX(), a_pCurrNode->m_Box.getMaxY(), a_pCurrNode->m_Box.getMinZ());
					break;

				case OctreePartition::PX_PY_PZ:
					l_Edge = a_pCurrNode->m_Box.getMax();
					break;
			}
			l_Box.m_Center = (a_pCurrNode->m_Box.m_Center + l_Edge) * 0.5;
		}
		else l_Box = a_pCurrNode->m_pChildren[i]->m_Box;
		
		if( !l_Box.intersect(a_Pos1, a_Pos2, a_Pos3) ) continue;
		
		LightMapBoxCache *a_pTargetNode = a_pCurrNode->m_pChildren[i];
		if( nullptr == a_pTargetNode )
		{
			a_pTargetNode = a_pCurrNode->m_pChildren[i] = new LightMapBoxCache();
			a_pTargetNode->m_pParent = a_pCurrNode;
			a_pTargetNode->m_Box = l_Box;
			memset(a_pTargetNode->m_pChildren, 0, sizeof(LightMapBoxCache *) * 8);
		}

		assignTriangle(a_Pos1, a_Pos2, a_Pos3, a_pTargetNode, a_Output);
	}
}

void WorldAsset::assignLight(Light *a_pLight, LightMapBoxCache *a_pRoot, std::vector<LightMapBoxCache*> &a_Output)
{
	a_Output.clear();
	a_Output.push_back(a_pRoot);

	std::function<bool(Light*, LightMapBoxCache*)> l_pCheckFunc = nullptr;
	switch( a_pLight->typeID() )
	{
		case COMPONENT_OMNI_LIGHT:
			l_pCheckFunc = [&](Light *a_pThisLight, LightMapBoxCache *a_pCurrNode) -> bool
			{
				OmniLight *l_pInst = reinterpret_cast<OmniLight *>(a_pThisLight);
				glm::sphere l_Sphere(l_pInst->getPosition(), l_pInst->getRange());
				return a_pCurrNode->m_Box.intersect(l_Sphere);
			};
			break;

		case COMPONENT_SPOT_LIGHT:
			l_pCheckFunc = [&](Light *a_pThisLight, LightMapBoxCache *a_pCurrNode) -> bool
			{
				SpotLight *l_pInst = reinterpret_cast<SpotLight *>(a_pThisLight);
				glm::daabb l_SpotBox(l_pInst->getPosition() + 0.5f * l_pInst->getRange() * l_pInst->getDirection(), l_pInst->getDirection() * l_pInst->getRange());
				return a_pCurrNode->m_Box.intersect(l_SpotBox);
			};
			break;

		case COMPONENT_DIR_LIGHT:
			l_pCheckFunc = [&](Light*, LightMapBoxCache*) -> bool{ return true; };
			break;

		default:break;
	}

	for( unsigned int i=0 ; i<a_Output.size() ; ++i )
	{
		for( unsigned int j=0 ; j<8 ; ++j )
		{
			if( nullptr != a_Output[i]->m_pChildren[j] &&
				l_pCheckFunc(a_pLight, a_Output[i]->m_pChildren[j])) a_Output.push_back(a_Output[i]->m_pChildren[j]);
		}
	}
}
#pragma endregion

}