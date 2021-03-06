// LightmapAsset.h
//
// 2020/09/11 Ian Wu/Real0000
//

#ifndef _LIGHTMAP_ASSET_H_
#define _LIGHTMAP_ASSET_H_

#include "AssetBase.h"

namespace R
{

class Light;
class MaterialAsset;
class MaterialBlock;
class Scene;

class LightmapAsset : public AssetComponent
{
public:
	LightmapAsset();
	virtual ~LightmapAsset();

	static void validImportExt(std::vector<wxString> &a_Output){}
	static wxString validAssetKey(){ return wxT("Lightmap"); }

	virtual wxString getAssetExt(){ return LightmapAsset::validAssetKey(); }
	virtual void importFile(wxString a_File){}
	virtual void loadFile(boost::property_tree::ptree &a_Src);
	virtual void saveFile(boost::property_tree::ptree &a_Dst);

	void bake(std::shared_ptr<Scene> a_pScene);
	void stepBake(GraphicCommander *a_pCmd);
	void stopBake();

	bool isBaking(){ return m_bBaking; }
	unsigned int getMaxBoxLevel(){ return m_MaxBoxLevel; }
	std::shared_ptr<MaterialBlock> getHarmonics(){ return m_pHarmonics; }
	std::shared_ptr<MaterialBlock> getBoxes(){ return m_pBoxes; }

private:
	struct LightMapBox
	{
		glm::vec3 m_BoxCenter;
		int m_Level;
		glm::vec3 m_BoxSize;
		int m_Padding;
		int m_SHResult[64];
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
	struct LightMapTextureCache
	{
		LightMapTextureCache() : m_pBaseColor(nullptr), m_pNormal(nullptr), m_pMetal(nullptr), m_pRoughness(nullptr), m_pRefract(nullptr){}
		~LightMapTextureCache()
		{
			m_pBaseColor = nullptr;
			m_pNormal = nullptr;
			m_pMetal = nullptr;
			m_pRoughness = nullptr;
			m_pRefract = nullptr;
		}

		std::shared_ptr<Asset> m_pBaseColor, m_pNormal, m_pMetal, m_pRoughness, m_pRefract;
	};
	struct LightMapVtxSrc
	{
		glm::vec3 m_Position;
		float m_U;
		glm::vec3 m_Normal;
		float m_V;
		glm::vec3 m_Tangent;
		float m_Padding1;
		glm::vec3 m_Binormal;
		float m_Padding2;
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
	void assignInitRaytraceInfo();
	void assignRaytraceInfo();
	void moveCacheData();
	
	bool m_bBaking;
	std::shared_ptr<Asset> m_pRayIntersectMat, m_pRayScatterMat;
	MaterialAsset *m_pRayIntersectMatInst, *m_pRayScatterMatInst;
	std::vector<LightMapBoxCache> m_BoxCache;
	std::shared_ptr<MaterialBlock> m_pIndicies, m_pHarmonicsCache, m_pBoxCache, m_pVertex, m_pResult;
	std::vector<LightMapTextureCache> m_LightMapMaterialCache;
	std::mutex m_BakeLock;

	unsigned int m_MaxBoxLevel;
	std::shared_ptr<MaterialBlock> m_pHarmonics, m_pBoxes;
};

}

#endif