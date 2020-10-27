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
#define NEIGHBOR_INDEX(x, y, z) (9*z+3*y+x)
#define NP 0
#define ZP 1
#define PP 2

#pragma region WorldAsset
//
// WorldAsset
//
WorldAsset::WorldAsset()
	: m_bBaking(false)
	, m_pRayIntersectMat(AssetManager::singleton().createAsset(LIGHTMAP_INTERSECT_ASSET_NAME).second)
	, m_pRayIntersectMatInst(nullptr)
	, m_pIndicies(nullptr), m_pHarmonicsCache(nullptr), m_pBoxCache(nullptr), m_pVertex(nullptr), m_pResult(nullptr)
	, m_pHarmonics(nullptr), m_pBoxes(nullptr)
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
	std::vector<std::vector<int>> l_TriangleInBox;
	std::vector<std::vector<glm::ivec2>> l_LightInBox;
	m_BoxCache.push_back(LightMapBoxCache());
	{
		LightMapBoxCache &l_Root = m_BoxCache[0];
		l_Root.m_BoxCenter = l_Bounding.m_Center;
		l_Root.m_BoxSize = l_Bounding.m_Size;
		for( unsigned int i=0 ; i<l_TempTriangleData.size() ; i+=3 )
		{
			std::set<int> l_IntersectBox;
			l_IntersectBox.clear();
			assignTriangle(l_TempVertexData[i].m_Position, l_TempVertexData[i+1].m_Position, l_TempVertexData[i+2].m_Position, 0, l_IntersectBox);
			for( auto it=l_IntersectBox.begin() ; it!=l_IntersectBox.end() ; ++it )
			{
				while( l_TriangleInBox.size() <= *it ) l_TriangleInBox.push_back(std::vector<int>());
				l_TriangleInBox[*it].push_back(i);
			}
		}
	}

	// assign lights
	for( unsigned int i=0 ; i<l_Lights.size() ; ++i )
	{
		if( l_Lights[i]->isHidden() ) continue;

		Light *l_pLightObj = reinterpret_cast<Light*>(l_Lights[i].get());
		
		std::vector<int> l_IntersectBox;
		assignLight(l_pLightObj, 0, l_IntersectBox);

		glm::ivec2 l_TempData;
		l_TempData.x = l_pLightObj->typeID();
		l_TempData.y = l_pLightObj->getID();
		for( unsigned int j=0 ; j<l_IntersectBox.size() ; ++j )
		{
			while( l_Lights.size() <= l_IntersectBox[j] ) l_LightInBox.push_back(std::vector<glm::ivec2>());
			l_LightInBox[l_IntersectBox[j]].push_back(l_TempData);
		}
	}

	// init uav triangle/light/harmonics data
	std::vector<glm::ivec4> l_UavTriangleData(2, glm::ivec4(0, 0, 0, 0));
	unsigned int l_NumHarmonics = 0;
	{
		std::vector<glm::ivec2> l_UavLightData;
		for( unsigned int i=0 ; i<m_BoxCache.size() ; ++i )
		{
			LightMapBoxCache &l_ThisNode = m_BoxCache[i];
			for( unsigned int j=0 ; j<8 ; ++j )
			{
				if( -1 != l_ThisNode.m_Children[j] ) continue;
				glm::ivec2 l_Range[3];
				
				glm::ivec3 l_OctreeSign((i & 0x04) >> 2, (i & 0x02) >> 1, i & 0x01);
				for( unsigned int axis=0 ; axis<3 ; ++axis )
				{
					if( 0 == l_OctreeSign[axis] )
					{
						l_Range[axis].x = 0;
						l_Range[axis].y = 1;
					}
					else
					{
						l_Range[axis].x = 2;
						l_Range[axis].y = 3;
					}
				}
				for( int z=l_Range[2].x ; z<=l_Range[2].y ; ++z )
				{
					for( int y=l_Range[1].x ; y<=l_Range[1].y ; ++y )
					{
						for( int x=l_Range[0].x ; x<=l_Range[0].y ; ++x )
						{
							l_ThisNode.m_SHResult[z*16 + y*4 + x] = l_NumHarmonics;
							l_NumHarmonics += 16;
						}
					}
				}
			}

			l_ThisNode.m_Light.x = l_UavLightData.size();
			l_ThisNode.m_Light.y = l_LightInBox[i].size();
			for( int j=0 ; j<l_ThisNode.m_Light.y ; ++j ) l_UavLightData.push_back(l_LightInBox[i][j]);
			l_ThisNode.m_Light.y += l_ThisNode.m_Light.x;

			l_ThisNode.m_Triangle.x = l_UavTriangleData.size();
			l_ThisNode.m_Triangle.y = l_TriangleInBox[i].size() / 4;
			for( int j=0 ; j<l_ThisNode.m_Triangle.y ; ++j )
			{
				glm::ivec4 l_Triangle(l_TriangleInBox[i][j*4], l_TriangleInBox[i][j*4 + 1], l_TriangleInBox[i][j*4 + 2], l_TriangleInBox[i][j*4 + 3]);
				l_UavTriangleData.push_back(l_Triangle);
			}
			l_ThisNode.m_Triangle.y += l_ThisNode.m_Triangle.x;
		}

		// packed info data, [0] : {curr box, curr harmonic, curr sample, max sample}, [1] : {light start offset, next box, next harmonic, num running thread}
		l_UavTriangleData[0].z = m_BoxCache.size();
		l_UavTriangleData[0].w = EngineSetting::singleton().m_LightMapSample;
		l_UavTriangleData[1].x = l_UavTriangleData.size();
		l_UavTriangleData[1].w = EngineSetting::singleton().m_LightMapSample;
		if( 0 != (l_UavLightData.size() % 2) ) l_UavLightData.push_back(glm::ivec2(0, 0));
		for( unsigned int i=0 ; l_UavLightData.size() ; i+=2 )
		{
			glm::ivec4 l_Pack(l_UavLightData[i].x, l_UavLightData[i].y, l_UavLightData[i+1].x, l_UavLightData[i+1].y);
			l_UavTriangleData.push_back(l_Pack);
		}
	}
	assignNeighbor(0);

	m_pIndicies = m_pRayIntersectMatInst->createExternalBlock(ShaderRegType::UavBuffer, "g_Indicies", l_UavTriangleData.size());
	memcpy(m_pIndicies->getBlockPtr(0), l_UavTriangleData.data(), sizeof(glm::ivec4) * l_UavTriangleData.size());
	m_pIndicies->sync(true);

	m_pHarmonicsCache = m_pRayIntersectMatInst->createExternalBlock(ShaderRegType::UavBuffer, "g_Harmonics", l_NumHarmonics);
	memset(m_pHarmonicsCache->getBlockPtr(0), 0, sizeof(glm::vec4) * l_NumHarmonics);
	m_pHarmonicsCache->sync(true);

	m_pBoxCache = m_pRayIntersectMatInst->createExternalBlock(ShaderRegType::UavBuffer, "g_Box", m_BoxCache.size());
	memcpy(m_pBoxes->getBlockPtr(0), m_BoxCache.data(), sizeof(LightMapBoxCache) * m_BoxCache.size());
	m_pBoxCache->sync(true);

	m_pVertex = m_pRayIntersectMatInst->createExternalBlock(ShaderRegType::UavBuffer, "g_Vertex", l_TempVertexData.size());
	memcpy(m_pVertex->getBlockPtr(0), l_TempVertexData.data(), sizeof(LightMapVtxSrc) * l_TempVertexData.size());
	m_pVertex->sync(true);
	
	m_pResult = m_pRayIntersectMatInst->createExternalBlock(ShaderRegType::UavBuffer, "g_Result", EngineSetting::singleton().m_LightMapSample);
	memset(m_pResult->getBlockPtr(0), 0, sizeof(LightIntersectResult) * EngineSetting::singleton().m_LightMapSample);
	m_pResult->sync(true);
	
	m_pRayIntersectMatInst->setBlock("g_Indicies", m_pIndicies);
	m_pRayIntersectMatInst->setBlock("g_Harmonics", m_pHarmonicsCache);
	m_pRayIntersectMatInst->setBlock("g_DirLights", a_pMember->m_pDirLights->getMaterialBlock());
	m_pRayIntersectMatInst->setBlock("g_OmniLights", a_pMember->m_pOmniLights->getMaterialBlock());
	m_pRayIntersectMatInst->setBlock("g_SpotLights", a_pMember->m_pSpotLights->getMaterialBlock());
	m_pRayIntersectMatInst->setBlock("g_Box", m_pBoxCache);
	m_pRayIntersectMatInst->setBlock("g_Vertex", m_pVertex);
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
	
	m_BoxCache.clear();
	m_pIndicies = nullptr;
	m_pBoxCache = nullptr;
	m_pVertex = nullptr;
	m_pResult = nullptr;
	m_pHarmonicsCache = nullptr;
	m_LightMap.clear();

	m_bBaking = false;
}

void WorldAsset::assignTriangle(glm::vec3 &a_Pos1, glm::vec3 &a_Pos2, glm::vec3 &a_Pos3, int a_CurrNode, std::set<int> &a_Output)
{
	a_Output.insert(a_CurrNode);

	LightMapBoxCache *l_pCurrNode = &m_BoxCache[a_CurrNode];
	if( l_pCurrNode->m_BoxSize.x - DEFAULT_OCTREE_EDGE < FLT_EPSILON ) return;
	
	glm::aabb l_ThisBox(l_pCurrNode->m_BoxCenter, l_pCurrNode->m_BoxSize);
	for( unsigned int i=0 ; i<8 ; ++i )
	{
		glm::aabb l_Box;
		int l_TargetNode = l_pCurrNode->m_Children[i];
		if( -1 == l_TargetNode )
		{
			l_Box.m_Size = l_pCurrNode->m_BoxSize * 0.5f;

			glm::vec3 l_Edge;
			switch(i)
			{
				case OCT_NX_NY_NZ:
					l_Edge = l_ThisBox.getMin();
					break;

				case OCT_NX_NY_PZ:
					l_Edge = glm::dvec3(l_ThisBox.getMinX(), l_ThisBox.getMinY(), l_ThisBox.getMaxZ());
					break;

				case OCT_NX_PY_NZ:
					l_Edge = glm::dvec3(l_ThisBox.getMinX(), l_ThisBox.getMaxY(), l_ThisBox.getMinZ());
					break;

				case OCT_NX_PY_PZ:
					l_Edge = glm::dvec3(l_ThisBox.getMinX(), l_ThisBox.getMaxY(), l_ThisBox.getMaxZ());
					break;

				case OCT_PX_NY_NZ:
					l_Edge = glm::dvec3(l_ThisBox.getMaxX(), l_ThisBox.getMinY(), l_ThisBox.getMinZ());
					break;

				case OCT_PX_NY_PZ:
					l_Edge = glm::dvec3(l_ThisBox.getMaxX(), l_ThisBox.getMinY(), l_ThisBox.getMaxZ());
					break;

				case OCT_PX_PY_NZ:
					l_Edge = glm::dvec3(l_ThisBox.getMaxX(), l_ThisBox.getMaxY(), l_ThisBox.getMinZ());
					break;

				case OCT_PX_PY_PZ:
					l_Edge = l_ThisBox.getMax();
					break;
			}
			l_Box.m_Center = (l_ThisBox.m_Center + l_Edge) * 0.5f;
		}
		else
		{
			LightMapBoxCache &l_BoxOwner = m_BoxCache[l_TargetNode];
			l_Box = glm::aabb(l_BoxOwner.m_BoxCenter, l_BoxOwner.m_BoxSize);
		}
		
		if( !l_Box.intersect(a_Pos1, a_Pos2, a_Pos3) ) continue;
		
		if( -1 == l_TargetNode )
		{
			LightMapBoxCache l_NewBox;
			l_TargetNode = m_BoxCache.size();
			l_pCurrNode->m_Children[i] = l_TargetNode;

			l_NewBox.m_BoxCenter = l_Box.m_Center;
			l_NewBox.m_BoxSize = l_Box.m_Size;
			l_NewBox.m_Level = l_pCurrNode->m_Level + 1;
			l_NewBox.m_Parent = a_CurrNode;
			m_BoxCache.push_back(l_NewBox);
		}

		assignTriangle(a_Pos1, a_Pos2, a_Pos3, l_TargetNode, a_Output);
	}
}

void WorldAsset::assignLight(Light *a_pLight, int a_CurrNode, std::vector<int> &a_Output)
{
	a_Output.clear();
	a_Output.push_back(a_CurrNode);

	std::function<bool(Light*, LightMapBoxCache*)> l_pCheckFunc = nullptr;
	switch( a_pLight->typeID() )
	{
		case COMPONENT_OMNI_LIGHT:
			l_pCheckFunc = [&](Light *a_pThisLight, LightMapBoxCache *a_pCurrNode) -> bool
			{
				OmniLight *l_pInst = reinterpret_cast<OmniLight *>(a_pThisLight);
				glm::sphere l_Sphere(l_pInst->getPosition(), l_pInst->getRange());
				glm::aabb l_Box(a_pCurrNode->m_BoxCenter, a_pCurrNode->m_BoxSize);
				return l_Box.intersect(l_Sphere);
			};
			break;

		case COMPONENT_SPOT_LIGHT:
			l_pCheckFunc = [&](Light *a_pThisLight, LightMapBoxCache *a_pCurrNode) -> bool
			{
				SpotLight *l_pInst = reinterpret_cast<SpotLight *>(a_pThisLight);
				glm::daabb l_SpotBox(l_pInst->getPosition() + 0.5f * l_pInst->getRange() * l_pInst->getDirection(), l_pInst->getDirection() * l_pInst->getRange());
				glm::daabb l_Box(a_pCurrNode->m_BoxCenter, a_pCurrNode->m_BoxSize);
				return l_Box.intersect(l_SpotBox);
			};
			break;

		case COMPONENT_DIR_LIGHT:
			l_pCheckFunc = [&](Light*, LightMapBoxCache*) -> bool{ return true; };
			break;

		default:break;
	}

	for( unsigned int i=0 ; i<a_Output.size() ; ++i )
	{
		LightMapBoxCache &l_ThisBox = m_BoxCache[a_Output[i]];
		for( unsigned int j=0 ; j<8 ; ++j )
		{
			if( -1 != l_ThisBox.m_Children[j] &&
				l_pCheckFunc(a_pLight, &m_BoxCache[l_ThisBox.m_Children[j]])) a_Output.push_back(l_ThisBox.m_Children[j]);
		}
	}
}

void WorldAsset::assignNeighbor(int a_CurrNode)
{
	LightMapBoxCache &l_CurrNode = m_BoxCache[a_CurrNode];
	l_CurrNode.m_Neighbor[NEIGHBOR_INDEX(ZP, ZP, ZP)] = a_CurrNode;

#define ASSIGN_NEIGHBOR(dst, src, oct) {						\
	int l_Corner = l_CurrNode.m_Neighbor[src];					\
	if( -1 != l_Corner ) {										\
		LightMapBoxCache &l_CornerNode = m_BoxCache[l_Corner];	\
		int l_Neighbor = l_CornerNode.m_Children[oct];			\
		l_TargetNode.m_Neighbor[dst] = -1 != l_Neighbor ? l_Neighbor : l_Corner; }}

	for( unsigned int i=0 ; i<8 ; ++i )
	{
		if( -1 == l_CurrNode.m_Children[i] ) continue;

		LightMapBoxCache &l_TargetNode = m_BoxCache[l_CurrNode.m_Children[i]];
		int l_X = (i & 0x04) >> 2;
		int l_Y = (i & 0x02) >> 1;
		int l_Z = i & 0x01;
		for( unsigned int x=NP ; x<=PP ; ++x )
		{
			for( unsigned int y=NP ; y<=PP ; ++y )
			{
				for( unsigned int z=NP ; z<=PP ; ++z )
				{
					if( ZP == x && ZP == y && ZP == z ) continue;

					int l_SrcIdx = NEIGHBOR_INDEX(x, y, z);
					int l_Block = i;
					glm::ivec3 l_BlockOffset(x + l_X - 1, y + l_Y - 1, z + l_Z - 1);
					glm::ivec3 l_Neighbor(ZP, ZP, ZP);
					for( int axis=0 ; axis<3 ; ++i )
					{
						if( l_BlockOffset[axis] < 0 )
						{
							l_BlockOffset[axis] = 1;
							l_Neighbor[axis] = NP;
						}
						else if( l_BlockOffset[axis] >= 2 )
						{
							l_BlockOffset[axis] = 0;
							l_Neighbor[axis] = PP;
						}
					}
					ASSIGN_NEIGHBOR(l_SrcIdx, NEIGHBOR_INDEX(l_Neighbor.x, l_Neighbor.y, l_Neighbor.z), l_BlockOffset.x << 4 | l_BlockOffset.y << 2 | l_BlockOffset.z)
				}
			}
		}
		
		assignNeighbor(l_CurrNode.m_Children[i]);
	}

#undef ASSIGN_NEIGHBOR
}
#pragma endregion

}