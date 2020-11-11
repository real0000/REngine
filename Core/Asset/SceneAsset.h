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
	template<typename T>
	static void registSceneGraphReflector()
	{
		std::string l_TypeName(T::typeName());
		assert(m_SceneGraphReflectors.end() == m_SceneGraphReflectors.find(l_TypeName));
		m_SceneGraphReflectors.insert(std::make_pair(l_TypeName, [=](boost::property_tree::ptree &a_Src) -> ScenePartition*
		{
			return T::create(a_Src);
		}));
	}
	template<typename T>
	static void registRenderPipelineReflector()
	{
		std::string l_TypeName(T::typeName());
		assert(m_RenderPipeLineReflectors.end() == m_RenderPipeLineReflectors.find(l_TypeName));
		m_RenderPipeLineReflectors.insert(std::make_pair(l_TypeName, [=](boost::property_tree::ptree &a_Src) -> RenderPipeline*
		{
			return T::create(a_Src);
		}));
	}

	virtual wxString getAssetExt(){ return SceneAsset::validAssetKey(); }
	virtual void importFile(wxString a_File){}
	virtual void loadFile(boost::property_tree::ptree &a_Src);
	virtual void saveFile(boost::property_tree::ptree &a_Dst);

	void updateCache(std::shared_ptr<Scene> a_pScene);

private:
	boost::property_tree::ptree m_Cache;
	
	RenderPipeline *m_pRenderer;
	std::shared_ptr<SceneNode> m_pRootNode;
	ScenePartition *m_pGraphs[NUM_GRAPH_TYPE];
	SceneBatcher *m_pBatcher;
	LightContainer<DirLight> *m_pDirLights;
	LightContainer<OmniLight> *m_pOmniLights;
	LightContainer<SpotLight> *m_pSpotLights;
	std::shared_ptr<Asset> m_pRefLightmap;

	static std::map<std::string, std::function<RenderPipeline*(boost::property_tree::ptree&)>> m_RenderPipeLineReflectors;
	static std::map<std::string, std::function<ScenePartition*(boost::property_tree::ptree&)>> m_SceneGraphReflectors;
};

}
#endif