// SceneAsset.cpp
//
// 2020/06/15 Ian Wu/Real0000
//

#include "CommonUtil.h"
#include "RGDeviceWrapper.h"
#include "RImporters.h"

#include "Core.h"
#include "MaterialAsset.h"
#include "MeshAsset.h"
#include "RenderObject/Mesh.h"
#include "SceneAsset.h"
#include "Scene/Graph/ScenePartition.h"
#include "Scene/Scene.h"

namespace R
{

#pragma region SceneAsset
//
// SceneAsset
//
SceneAsset::SceneAsset()
{
}

SceneAsset::~SceneAsset()
{
}
	
void SceneAsset::loadFile(boost::property_tree::ptree &a_Src)
{
	m_Cache = a_Src.get_child("NodeTree");
}

void SceneAsset::saveFile(boost::property_tree::ptree &a_Dst)
{
	a_Dst.add_child("NodeTree", m_Cache);
}

void SceneAsset::setup(std::shared_ptr<SceneNode> a_pRefSceneNode)
{
	
}
#pragma endregion

}