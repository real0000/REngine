//
// 2019/10/29 Ian Wu/Real0000
//

#include "CommonUtil.h"
#include "RGDeviceWrapper.h"

#include "AssetBase.h"
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
	registAssetComponent<TextureAsset>();
}

AssetManager::~AssetManager()
{
}

void AssetManager::loadFile(std::shared_ptr<Asset> a_pInst, wxString a_Path)
{
	
}
#pragma endregion
}