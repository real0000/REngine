// PrefabAsset.h
//
// 2020/11/09 Ian Wu/Real0000
//

#ifndef _PREFAB_ASSET_H_
#define _PREFAB_ASSET_H_

#include "AssetBase.h"

namespace R
{

class PrefabAsset : public AssetComponent
{
public:
	PrefabAsset();
	virtual ~PrefabAsset();

	// for asset factory
	static void validImportExt(std::vector<wxString> &a_Output){}
	static wxString validAssetKey(){ return wxT("Prefab"); }

	virtual wxString getAssetExt(){ return PrefabAsset::validAssetKey(); }
	virtual void importFile(wxString a_File){}
	virtual void loadFile(boost::property_tree::ptree &a_Src);
	virtual void saveFile(boost::property_tree::ptree &a_Dst);

	boost::property_tree::ptree& getCache(){ return m_Cache; }

private:
	boost::property_tree::ptree m_Cache;
};

}
#endif