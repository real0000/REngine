// AssetBase.h
//
// 2019/10/07 Ian Wu/Real0000
//

#ifndef _ASSET_BASE_H_
#define _ASSET_BASE_H_

namespace R
{

class AssetComponent
{
public:
	AssetComponent(){}
	virtual ~AssetComponent(){}

	virtual void importFile(wxString a_File) = 0;
	virtual void loadFile(wxString a_File) = 0;
	virtual void saveFile(wxString a_File) = 0;
};

class Asset
{
public:
	Asset(){}
	virtual ~Asset(){}

	void initComponent(AssetComponent *a_pComponent);
	//void 

private:
	AssetComponent *m_pComponent;
};

class AssetManager : public SearchPathSystem<Asset>
{
public:
	static AssetManager& singleton();


private:
	AssetManager();
	virtual ~AssetManager();

	template<typename T>
	void registAssetComponent()
	{
		std::vector<wxString> l_List;
		T::validImportExt(l_List);
		std::function<AssetComponent*()> l_pFunc = []() -> AssetComponent*{ return new T(); };
		for( unsigned int i=0 ; i<l_List.size() ; ++i ) m_ImporterMap.insert(std::make_pair(l_List[i], l_pFunc));
		m_LoaderMap.insert(std::make_pair(T::validAssetKey(), l_pFunc));
	}

	void loadFile(std::shared_ptr<Asset> a_pInst, wxString a_Path);
	//void importFile();

	std::map<wxString, std::function<AssetComponent *()>> m_ImporterMap, m_LoaderMap;
};

}

#endif