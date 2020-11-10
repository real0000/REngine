// SceneAsset.h
//
// 2020/11/10 Ian Wu/Real0000
//

#ifndef _SCENE_ASSET_H_
#define _SCENE_ASSET_H_

#include "AssetBase.h"

namespace R
{

class ScenePartition;

class SceneAsset : public AssetComponent
{
public:
	SceneAsset();
	virtual ~SceneAsset();

	// for asset factory
	static void validImportExt(std::vector<wxString> &a_Output){}
	static wxString validAssetKey(){ return wxT("Scene"); }

	virtual wxString getAssetExt(){ return SceneAsset::validAssetKey(); }
	virtual void importFile(wxString a_File);
	virtual void loadFile(boost::property_tree::ptree &a_Src);
	virtual void saveFile(boost::property_tree::ptree &a_Dst);

private:
	boost::property_tree::ptree m_Cache;
	std::shared_ptr<Asset> m_pRefLightmap;
};

}
#endif