// SceneAsset.cpp
//
// 2020/11/10 Ian Wu/Real0000
//

#include "CommonUtil.h"
#include "RGDeviceWrapper.h"
#include "Core.h"
#include "SceneAsset.h"
#include "Scene/Scene.h"

namespace R
{

#pragma region SceneAsset
//
// SceneAsset
//
std::map<std::string, std::function<RenderPipeline*(boost::property_tree::ptree&)>> SceneAsset::m_RenderPipeLineReflectors;
std::map<std::string, std::function<ScenePartition*(boost::property_tree::ptree&)>> SceneAsset::m_SceneGraphReflectors;
SceneAsset::SceneAsset()
	: m_pRenderer(nullptr)
	, m_pRootNode(nullptr)
	, m_pGraphs{nullptr, nullptr, nullptr, nullptr, nullptr}
	, m_pBatcher(nullptr)
	, m_pDirLights(nullptr), m_pOmniLights(nullptr), m_pSpotLights(nullptr)
	, m_pRefLightmap(nullptr)
{
}

SceneAsset::~SceneAsset()
{
}

void SceneAsset::loadFile(boost::property_tree::ptree &a_Src)
{/*
	loadScenePartitionSetting(l_XMLTree.get_child("root.ScenePartition"));
		
		
		boost::property_tree::ptree &l_Attr = a_Node.get_child("<xmlattr>");
		switch( ScenePartitionType::fromString(l_Attr.get("type", "Octree")) )
		{
			case ScenePartitionType::None:{
				for( unsigned int i=0 ; i<Scene::NUM_GRAPH_TYPE ; ++i ) m_pGraphs[i] = new NoPartition();
				}break;

			case ScenePartitionType::Octree:{
				double l_RootEdge = a_Node.get("RootEdge", DEFAULT_OCTREE_ROOT_SIZE);
				double l_MinEdge = a_Node.get("MinEdge", DEFAULT_OCTREE_EDGE);
				for( unsigned int i=0 ; i<Scene::NUM_GRAPH_TYPE ; ++i ) m_pGraphs[i] = new OctreePartition(l_RootEdge, l_MinEdge);
				}break;
		}

	loadRenderPipelineSetting(l_XMLTree.get_child("root.RenderPipeline"));
	loadAssetSetting(l_XMLTree.get_child("root.Assets"));
*/
}

void SceneAsset::saveFile(boost::property_tree::ptree &a_Dst)
{
}

void SceneAsset::updateCache(std::shared_ptr<Scene> a_pScene)
{
}
#pragma endregion

}