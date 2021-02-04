// AssetBase.h
//
// 2019/10/07 Ian Wu/Real0000
//

#ifndef _ASSET_BASE_H_
#define _ASSET_BASE_H_

namespace R
{

class AssetManager;

// default assets
#define LIGHTMAP_INTERSECT_ASSET_NAME wxT("LightmapIntersection.Material")
#define LIGHTMAP_SCATTER_ASSET_NAME wxT("LightmapScatter.Material")

#define FRAMEBUFFER_ASSET_NAME wxT("DefferredFrame.Image")
#define DEPTHMINMAX_ASSET_NAME wxT("DefferredDepthMinmax.Image")

#define LIGHTINDEX_ASSET_NAME wxT("DefferredLightIndex.Material")
#define LIGHTING_ASSET_NAME wxT("TiledDefferredLighting.Material")
#define COPY_ASSET_NAME wxT("Copy.Material")

#define SHADOWMAP_ASSET_NAME wxT("DefferredRenderTextureAtlasDepth.Image")
#define CLEAR_MAT_ASSET_NAME wxT("ShadowMapClear.Material")

#define WHITE_TEXTURE_ASSET_NAME wxT("White.Image")
#define BLUE_TEXTURE_ASSET_NAME wxT("Blue.Image")
#define DARK_GRAY_TEXTURE_ASSET_NAME wxT("Darkgray.Image")
#define QUAD_MESH_ASSET_NAME wxT("Quad.Mesh")

class AssetComponent
{
public:
	AssetComponent();
	virtual ~AssetComponent();

	virtual bool canSave(){ return true; }
	virtual wxString getAssetExt() = 0;
	virtual void importFile(wxString a_File) = 0;
	virtual void loadFile(boost::property_tree::ptree &a_Src) = 0;
	virtual void saveFile(boost::property_tree::ptree &a_Dst) = 0;
};

class Asset
{
	friend class AssetManager;
public:
	Asset();
	virtual ~Asset();

	template<typename T>
	inline T* getComponent(){ return reinterpret_cast<T*>(m_pComponent); }
	wxString getAssetExt(){ return nullptr == m_pComponent ? wxT("") : m_pComponent->getAssetExt(); }
	wxString getKey(){ return m_Key; }
	int getSerialKey(){ return m_SerialKey; }

private:
	int m_SerialKey;
	wxString m_Key;
	AssetComponent *m_pComponent;
};

class AssetManager : public SearchPathSystem<Asset>
{
	friend class AssetComponent;
public:
	static AssetManager& singleton();

	// use these method instead getData
	std::shared_ptr<Asset> createAsset(wxString a_Path);
	std::shared_ptr<Asset> getAsset(wxString a_Path);
	std::shared_ptr<Asset> getAsset(int a_ID){ return getData(a_ID); }
	void saveAsset(int a_ID, wxString a_Path = ""){ saveAsset(getData(a_ID), a_Path); }
	void saveAsset(std::shared_ptr<Asset> a_pInst, wxString a_Path = "");

	void waitAssetClear();

private:
	AssetManager();
	virtual ~AssetManager();

	template<typename T>
	void registAssetComponent()
	{
		std::vector<wxString> l_List;
		T::validImportExt(l_List);
		std::function<AssetComponent*()> l_pFunc = []() -> AssetComponent*{ return new T(); };
		for( unsigned int i=0 ; i<l_List.size() ; ++i ) m_ImporterMap.insert(std::make_pair(l_List[i].MakeLower(), l_pFunc));
		m_LoaderMap.insert(std::make_pair(T::validAssetKey().MakeLower(), l_pFunc));
	}
	void loadFile(std::shared_ptr<Asset> a_pInst, wxString a_Path);

	std::map<wxString, std::function<AssetComponent *()>> m_ImporterMap, m_LoaderMap;
	static unsigned int sm_AssetCounter;
};

}

#endif