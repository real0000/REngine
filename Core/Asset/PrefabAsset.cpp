// PrefabAsset.cpp
//
// 2020/11/09 Ian Wu/Real0000
//

#include "CommonUtil.h"
#include "RGDeviceWrapper.h"
#include "Core.h"
#include "PrefabAsset.h"
#include "Scene/Scene.h"

namespace R
{

#pragma region PrefabAsset
// 
// PrefabAsset
//
PrefabAsset::PrefabAsset()
{
}

PrefabAsset::~PrefabAsset()
{
}

void PrefabAsset::loadFile(boost::property_tree::ptree &a_Src)
{
	m_Cache = a_Src.get_child("root");
}

void PrefabAsset::saveFile(boost::property_tree::ptree &a_Dst)
{
	a_Dst.add_child("root", m_Cache);
}
#pragma endregion

}