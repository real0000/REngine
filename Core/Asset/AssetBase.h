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

#define DEFFERRED_GBUFFER_NORMAL_ASSET_NAME wxT("DefferredGBufferNormal_%d.Image")
#define DEFFERRED_GBUFFER_MATERIAL_ASSET_NAME wxT("DefferredGBufferMaterial_%d.Image")
#define DEFFERRED_GBUFFER_BASE_COLOR_ASSET_NAME wxT("DefferredGBufferBaseColor_%d.Image")
#define DEFFERRED_GBUFFER_MASK_ASSET_NAME wxT("DefferredGBufferMask_%d.Image")
#define DEFFERRED_GBUFFER_FACTOR_ASSET_NAME wxT("DefferredGBufferFactor_%d.Image")
#define DEFFERRED_GBUFFER_MOTION_BLUR_ASSET_NAME wxT("DefferredGBufferMotionBlur_%d.Image")
#define DEFFERRED_GBUFFER_DEPTH_ASSET_NAME wxT("DefferredGBufferDepth_%d.Image")

#define DEFFERRED_GBUFFER_DEBUG_NORMAL_ASSET_NAME wxT("DefferredGBufferNormal_%d.Material")
#define DEFFERRED_GBUFFER_DEBUG_MATERIAL_ASSET_NAME wxT("DefferredGBufferMaterial_%d.Material")
#define DEFFERRED_GBUFFER_DEBUG_BASE_COLOR_ASSET_NAME wxT("DefferredGBufferBaseColor_%d.Material")
#define DEFFERRED_GBUFFER_DEBUG_MASK_ASSET_NAME wxT("DefferredGBufferMask_%d.Material")
#define DEFFERRED_GBUFFER_DEBUG_FACTOR_ASSET_NAME wxT("DefferredGBufferFactor_%d.Material")
#define DEFFERRED_GBUFFER_DEBUG_MOTION_BLUR_ASSET_NAME wxT("DefferredGBufferMotionBlur_%d.Material")
#define DEFFERRED_GBUFFER_DEBUG_DEPTH_ASSET_NAME wxT("DefferredGBufferDepth_%d.Material")

#define DEFFERRED_FRAMEBUFFER_ASSET_NAME wxT("DefferredFrame_%d.Image")
#define DEFFERRED_DEPTHMINMAX_ASSET_NAME wxT("DefferredDepthMinmax_%d.Image")

#define DEFFERRED_LIGHTINDEX_ASSET_NAME wxT("DefferredLightIndex_%d.Material")
#define DEFFERRED_LIGHTING_ASSET_NAME wxT("TiledDefferredLighting_%d.Material")

#define COPY_ASSET_NAME wxT("Copy.Material")
#define SHADOWMAP_ASSET_NAME wxT("DefferredRenderTextureAtlasDepth.Image")
#define CLEAR_MAT_ASSET_NAME wxT("ShadowMapClear.Material")

#define WHITE_TEXTURE_ASSET_NAME wxT("White.Image")
#define BLUE_TEXTURE_ASSET_NAME wxT("Blue.Image")
#define DARK_GRAY_TEXTURE_ASSET_NAME wxT("Darkgray.Image")
#define QUAD_MESH_ASSET_NAME wxT("Quad.Mesh")

class AssetComponent
{
	friend class AssetManager;
public:
	AssetComponent();
	virtual ~AssetComponent();

	bool dirty(){ return m_bDirty; }
	virtual bool canSave(){ return true; }
	virtual wxString getAssetExt() = 0;
	virtual void importFile(wxString a_File) = 0;
	virtual void loadFile(boost::property_tree::ptree &a_Src) = 0;
	virtual void saveFile(boost::property_tree::ptree &a_Dst) = 0;

protected:
	void setDirty(){ m_bDirty = true; }

private:
	bool m_bDirty;
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
		for( unsigned int i=0 ; i<l_List.size() ; ++i )
		{
			wxString l_LowerExt(l_List[i].MakeLower());
			m_ImporterMap.insert(std::make_pair(l_LowerExt, l_pFunc));
			m_ImportExtMap.insert(std::make_pair(l_LowerExt, T::validAssetKey()));
		}
		m_LoaderMap.insert(std::make_pair(T::validAssetKey().MakeLower(), l_pFunc));
	}
	void loadFile(std::shared_ptr<Asset> a_pInst, wxString a_Path);

	std::map<wxString, std::function<AssetComponent *()>> m_ImporterMap, m_LoaderMap;
	std::map<wxString, wxString> m_ImportExtMap;
	static unsigned int sm_AssetCounter;
};

}

#endif