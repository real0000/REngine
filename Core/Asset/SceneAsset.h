// SceneAsset.h
//
// 2020/06/15 Ian Wu/Real0000
//

#ifndef _SCENE_ASSET_H_
#define _SCENE_ASSET_H_

namespace R
{

class SceneAsset : public AssetComponent
{
public:
	SceneAsset();
	virtual ~SceneAsset();
	
	static void validImportExt(std::vector<wxString> &a_Output){}
	static wxString validAssetKey(){ return wxT("Scene"); }

	virtual wxString getAssetExt(){ return MaterialAsset::validAssetKey(); }
	virtual void importFile(wxString a_File){}
	virtual void loadFile(boost::property_tree::ptree &a_Src);
	virtual void saveFile(boost::property_tree::ptree &a_Dst);

};

}

#endif