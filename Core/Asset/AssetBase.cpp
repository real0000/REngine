//
// 2019/10/29 Ian Wu/Real0000
//

#include "CommonUtil.h"
#include "RGDeviceWrapper.h"

#include "AssetBase.h"
#include "LightmapAsset.h"
#include "MaterialAsset.h"
#include "MeshAsset.h"
#include "TextureAsset.h"

namespace R
{

#pragma region AssetManager
//
// AssetManager
//
AssetManager& AssetManager::singleton()
{
	static AssetManager sm_Inst;
	return sm_Inst;
}

AssetManager::AssetManager()
	: SearchPathSystem(std::bind(&AssetManager::loadFile, this, std::placeholders::_1, std::placeholders::_2), std::bind(&defaultNewFunc<Asset>))
{
	registAssetComponent<LightmapAsset>();
	registAssetComponent<MaterialAsset>();
	registAssetComponent<MeshAsset>();
	registAssetComponent<TextureAsset>();
}

AssetManager::~AssetManager()
{
}

std::shared_ptr<Asset> AssetManager::createAsset(wxString a_Path)
{
	std::pair<int, std::shared_ptr<Asset>> l_Res = getData(a_Path);
	if(-1 != l_Res.first) return l_Res.second;

	wxString l_Ext(getFileExt(a_Path).MakeLower());
	auto l_LoaderIt = m_LoaderMap.find(l_Ext);
	assert(l_LoaderIt != m_LoaderMap.end());

	l_Res = addData(a_Path);
	l_Res.second->m_SerialKey = l_Res.first;
	l_Res.second->m_Key = replaceFileExt(a_Path, l_Res.second->getAssetExt());
	l_Res.second->m_pComponent = l_LoaderIt->second();
	return l_Res.second;
}

std::shared_ptr<Asset> AssetManager::getAsset(wxString a_Path)
{
	std::pair<int, std::shared_ptr<Asset>> l_Res = getData(a_Path);
	assert(-1 != l_Res.first);
	l_Res.second->m_Key = replaceFileExt(a_Path, l_Res.second->getAssetExt());
	l_Res.second->m_SerialKey = l_Res.first;
	return l_Res.second;
}

void AssetManager::saveAsset(std::shared_ptr<Asset> a_pInst, wxString a_Path)
{
	if( a_Path.IsEmpty() ) a_Path = a_pInst->m_Key;
	else a_pInst->m_Key = replaceFileExt(a_Path, a_pInst->getAssetExt());
	
	boost::property_tree::ptree l_XMLTree;
	a_pInst->m_pComponent->saveFile(l_XMLTree);
	boost::property_tree::xml_parser::write_xml(static_cast<const char *>(a_Path.c_str()), l_XMLTree, std::locale()
		, boost::property_tree::xml_writer_make_settings<std::string>(' ', 4));
}

void AssetManager::loadFile(std::shared_ptr<Asset> a_pInst, wxString a_Path)
{
	wxString l_Ext(getFileExt(a_Path).MakeLower());
	
	// load asset
	auto l_LoaderIt = m_LoaderMap.find(l_Ext);
	if( m_LoaderMap.end() != l_LoaderIt )
	{
		boost::property_tree::ptree l_XMLTree;
		boost::property_tree::xml_parser::read_xml(static_cast<const char *>(a_Path.c_str()), l_XMLTree, boost::property_tree::xml_parser::no_comments);
		a_pInst->m_pComponent = l_LoaderIt->second();
		a_pInst->m_pComponent->loadFile(l_XMLTree);
		return;
	}

	// import new asset by raw resources
	auto l_ImporterIt = m_ImporterMap.find(l_Ext);
	assert(l_ImporterIt != m_ImporterMap.end());
	a_pInst->m_pComponent = l_ImporterIt->second();
	a_pInst->m_pComponent->importFile(a_Path);
}
#pragma endregion

}