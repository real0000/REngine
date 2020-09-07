// AssetBase.h
//
// 2019/10/07 Ian Wu/Real0000
//

#ifndef _ASSET_BASE_H_
#define _ASSET_BASE_H_

namespace R
{

class AssetManager;

class AssetComponent
{
public:
	AssetComponent(){}
	virtual ~AssetComponent(){}

	virtual wxString getAssetExt() = 0;
	virtual void importFile(wxString a_File) = 0;
	virtual void loadFile(boost::property_tree::ptree &a_Src) = 0;
	virtual void saveFile(boost::property_tree::ptree &a_Dst) = 0;
};

class Asset
{
	friend class AssetManager;
public:
	Asset() : m_Key(wxT("")), m_pComponent(nullptr){}
	virtual ~Asset(){ SAFE_DELETE(m_pComponent) }

	template<typename T>
	inline T* getComponent(){ return reinterpret_cast<T*>(m_pComponent); }
	wxString getAssetExt(){ return nullptr == m_pComponent ? wxT("") : m_pComponent->getAssetExt(); }
	wxString getKey(){ return m_Key; }

private:
	wxString m_Key;
	AssetComponent *m_pComponent;
};

class AssetManager : public SearchPathSystem<Asset>
{
public:
	static AssetManager& singleton();

	// use these method instead getData
	std::pair<int, std::shared_ptr<Asset>> createAsset(wxString a_Path);
	std::pair<int, std::shared_ptr<Asset>> getAsset(wxString a_Path);
	std::shared_ptr<Asset> getAsset(int a_ID){ return getData(a_ID); }
	void saveAsset(int a_ID, wxString a_Path = ""){ saveAsset(getData(a_ID), a_Path); }
	void saveAsset(std::shared_ptr<Asset> a_pInst, wxString a_Path = "");

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
};

}

#endif