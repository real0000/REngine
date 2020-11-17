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
SceneAsset::SceneAsset()
	: m_LightmapAssetPath(wxT(""))
{
}

SceneAsset::~SceneAsset()
{
}

void SceneAsset::loadFile(boost::property_tree::ptree &a_Src)
{
	boost::property_tree::ptree &l_Root = a_Src.get_child("root");
	
	m_LightmapAssetPath = l_Root.get("<xmlattr>.lightmap", "");

	m_NodeTree = l_Root.get_child("Node");
	m_SceneGraghSetting = l_Root.get_child("Graph");
	m_PipelineSetting = l_Root.get_child("Pipeline");
	m_ShadowSetting = l_Root.get_child("Shadow");
}

void SceneAsset::saveFile(boost::property_tree::ptree &a_Dst)
{
	boost::property_tree::ptree l_Root;

	boost::property_tree::ptree l_RootAttr;
	l_RootAttr.add("lightmap", m_LightmapAssetPath.mbc_str());
	l_Root.add_child("<xmlattr>", l_RootAttr);

	l_Root.add_child("Node", m_NodeTree);
	l_Root.add_child("Graph", m_SceneGraghSetting);
	l_Root.add_child("Pipeline", m_PipelineSetting);
	l_Root.add_child("Shadow", m_ShadowSetting);

	a_Dst.add_child("root", l_Root);
}

void SceneAsset::updateCache(std::shared_ptr<Scene> a_pScene)
{
	m_NodeTree.clear();
	a_pScene->getRootNode()->bakeNode(m_NodeTree);

	m_PipelineSetting.clear();
	m_ShadowSetting.clear();
	a_pScene->saveRenderSetting(m_PipelineSetting, m_ShadowSetting);
	
	m_SceneGraghSetting.clear();
	a_pScene->saveSceneGraphSetting(m_SceneGraghSetting);

	if( nullptr != a_pScene->getLightmap() ) m_LightmapAssetPath = a_pScene->getLightmap()->getKey();
}
#pragma endregion

}