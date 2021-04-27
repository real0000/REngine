// SceneAsset.h
//
// 2020/11/10 Ian Wu/Real0000
//

#ifndef _SCENE_ASSET_H_
#define _SCENE_ASSET_H_

#include "AssetBase.h"

namespace R
{

class Camera;
class DirLight;
class OmniLight;
class RenderPipeline;
class SceneBatcher;
class ScenePartition;
class SpotLight;

template<typename T>
class LightContainer;

class SceneAsset : public AssetComponent
{
public:
	SceneAsset();
	virtual ~SceneAsset();

	// for asset factory
	static void validImportExt(std::vector<wxString> &a_Output){}
	static wxString validAssetKey(){ return wxT("Scene"); }

	virtual wxString getAssetExt(){ return SceneAsset::validAssetKey(); }
	virtual void importFile(wxString a_File){}
	virtual void loadFile(boost::property_tree::ptree &a_Src);
	virtual void saveFile(boost::property_tree::ptree &a_Dst);

	void updateCache(std::shared_ptr<Scene> a_pScene);
	boost::property_tree::ptree& getNodeTree(){ return m_NodeTree; }
	boost::property_tree::ptree& getPipelineSetting(){ return m_PipelineSetting; }
	boost::property_tree::ptree& getShadowSetting(){ return m_ShadowSetting; }
	wxString getLightmapAssetPath(){ return m_LightmapAssetPath; }

private:
	boost::property_tree::ptree m_NodeTree;
	boost::property_tree::ptree m_PipelineSetting;
	boost::property_tree::ptree m_ShadowSetting;
	wxString m_LightmapAssetPath;
};

}
#endif