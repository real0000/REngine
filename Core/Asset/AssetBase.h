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
#define COPY_ASSET_NAME wxT("Copy.Material")
#define COPYDEPTH_ASSET_NAME wxT("CopyDepth.Material")
#define CLEAR_MAT_ASSET_NAME wxT("ShadowMapClear.Material")
#define DEFAULT_DIR_SHADOWMAP_MAT_ASSET_NAME wxT("DefaultDirShadowMap.Material")
#define DEFAULT_OMNI_SHADOWMAP_MAT_ASSET_NAME wxT("DefaultOmniShadowMap.Material")
#define DEFAULT_SPOT_SHADOWMAP_MAT_ASSET_NAME wxT("DefaultSpotShadowMap.Material")

#define WHITE_TEXTURE_ASSET_NAME wxT("White.Image")
#define BLACK_TEXTURE_ASSET_NAME wxT("Black.Image")
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
	virtual bool canSave(){ return !m_bRuntimeOnly; }
	virtual wxString getAssetExt() = 0;
	virtual void importFile(wxString a_File) = 0;
	virtual void loadFile(boost::property_tree::ptree &a_Src) = 0;
	virtual void saveFile(boost::property_tree::ptree &a_Dst) = 0;

	bool getRuntimeOnly(){ return m_bRuntimeOnly; }
	void setRuntimeOnly(bool a_bRuntimeOnly){ m_bRuntimeOnly = a_bRuntimeOnly; }

protected:
	void setDirty(){ m_bDirty = true; }

private:
	bool m_bDirty;
	bool m_bRuntimeOnly;
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
	
	bool dirty(){ return m_pComponent->dirty(); }
	bool getRuntimeOnly(){ return m_pComponent->getRuntimeOnly(); }
	void setRuntimeOnly(bool a_bRuntimeOnly){ m_pComponent->setRuntimeOnly(a_bRuntimeOnly); }

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
	template<typename T>
	std::shared_ptr<Asset> createRuntimeAsset()
	{
		wxString l_AssetName(wxString::Format(wxT("RuntimeAsset_%d.%s"), m_TempAssetSerial++, T::validAssetKey()));
		std::shared_ptr<Asset> l_Res = createAsset(l_AssetName);
		l_Res->setRuntimeOnly(true);
		return l_Res;
	}
	std::shared_ptr<Asset> createAsset(wxString a_Path);
	void removeAsset(std::shared_ptr<Asset> a_pInst);
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
	unsigned int m_TempAssetSerial;
	static unsigned int sm_AssetCounter;
};

}

#endif