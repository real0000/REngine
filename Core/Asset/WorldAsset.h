// WorldAsset.h
//
// 2020/09/11 Ian Wu/Real0000
//

#ifndef _WORLD_ASSET_H_
#define _WORLD_ASSET_H_

#include "AssetBase.h"

namespace R
{

struct SharedSceneMember;

class WorldAsset : public AssetComponent
{
public:
	WorldAsset();
	virtual ~WorldAsset();

	static void validImportExt(std::vector<wxString> &a_Output){}
	static wxString validAssetKey(){ return wxT("World"); }

	virtual wxString getAssetExt(){ return WorldAsset::validAssetKey(); }
	virtual void importFile(wxString a_File){}
	virtual void loadFile(boost::property_tree::ptree &a_Src);
	virtual void saveFile(boost::property_tree::ptree &a_Dst);

	void bake(SharedSceneMember *a_pMember);
	void stepBake(GraphicCommander *a_pCmd);
	void stopBake();

private:
	struct LightMapBox
	{
		glm::vec3 m_BoxCenter;
		int m_Parent;
		glm::vec3 m_BoxSize;
		int m_Edge;
		int m_SHResult[16];
		int m_Children[8];
	};
	struct LightMapBoxCache
	{
		LightMapBoxCache()
			: m_BoxCenter()
			, m_Parent(-1)
			, m_BoxSize()
			, m_Level(0)
			, m_Triangle(0, 0)
			, m_Light(0, 0)
		{
			memset(m_Neighbor, -1, sizeof(int) * 28);
			memset(m_SHResult, -1, sizeof(int) * 64);
			memset(m_Children, -1, sizeof(int) * 8);
		}

		glm::vec3 m_BoxCenter;
		int m_Parent;
		glm::vec3 m_BoxSize;
		int m_Level;
		int m_Neighbor[28];// ---, --0, --+, -0-, -00, -0+, -+-, -+0, -++, 0--, 0-0, 0-+, 00-, 000, 00+, 0+-, 0+0, 0++, +--, +-0, +-+, +0-, +00, +0+, ++-, ++0, +++, padding
		glm::ivec2 m_Triangle;
		glm::ivec2 m_Light;
		int m_SHResult[64];
		int m_Children[8];
	};
	struct LightMapVtxSrc
	{
		glm::vec3 m_Position;
		float m_U;
		glm::vec3 m_Normal;
		float m_V;
	};
	//struct LightMapIndexSrc index * 3, material id
	struct LightIntersectResult
	{
		glm::vec3 m_Origin;
		int m_TriangleStartIdx;
		glm::vec3 m_Direction;
		int m_StateDepth;
		glm::vec3 m_Color;
		int m_BoxHarmonicsID;
		glm::vec3 m_Emmited;
		int m_CurrBoxID;
	};
	void assignTriangle(glm::vec3 &a_Pos1, glm::vec3 &a_Pos2, glm::vec3 &a_Pos3, int a_CurrNode, std::set<int> &a_Output);
	void assignLight(Light *a_pLight, int a_CurrNode, std::vector<int> &a_Output);
	void assignNeighbor(int a_CurrNode);
	
	bool m_bBaking;
	std::shared_ptr<Asset> m_pRayIntersectMat;
	MaterialAsset *m_pRayIntersectMatInst;
	std::vector<LightMapBoxCache> m_BoxCache;
	std::shared_ptr<MaterialBlock> m_pIndicies, m_pHarmonicsCache, m_pBoxCache, m_pVertex, m_pResult;
	std::mutex m_BakeLock;

	std::shared_ptr<MaterialBlock> m_pHarmonics, m_pBoxes;
	std::vector<LightMapBox> m_LightMap;
	std::vector<std::shared_ptr<Asset>> m_LightMapMaterialCache;

};

}

#endif