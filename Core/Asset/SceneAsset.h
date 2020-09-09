// SceneAsset.h
//
// 2020/06/15 Ian Wu/Real0000
//

#ifndef _SCENE_ASSET_H_
#define _SCENE_ASSET_H_

namespace R
{

struct SharedSceneMember;
class SceneNode;

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
	
	void bake(std::shared_ptr<SceneNode> a_pRefSceneNode, SharedSceneMember *a_pMember);
	void setup(std::shared_ptr<SceneNode> a_pRefSceneNode);

private:
	struct LightMapUnit
	{
		glm::daabb m_Box;
		glm::vec4 m_SHResult[64][16];
		int m_Children[8];
	};
	struct LightMapNode
	{
		unsigned int m_UavOffset;
		LightMapUnit *m_pRefUnit;
		
		glm::ivec2 m_SrcRange;
	};
	struct LightMapVtxSrc
	{
		glm::vec3 m_Position;
		glm::vec3 m_Normal;
		glm::vec2 m_UV;
		unsigned int m_MaterialID;
	};
	struct LightIntersectResult
	{
		glm::vec3 m_Origin;
		unsigned int m_TriangleStartIdx;
		glm::vec3 m_Direction;
		unsigned int m_Depth;
		glm::vec3 m_Color;
		unsigned int m_BoxID;
		glm::vec3 m_Emmited;
		unsigned int m_HarmonicsID;
	};

	boost::property_tree::ptree m_Cache;
	unsigned int m_UavID;
	LightMapNode *m_pLightMapRoot;
	std::vector<std::shared_ptr<Asset>> m_LightMapMaterialCache;
};

}

#endif