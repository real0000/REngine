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
	void stopBake();

private:
	struct LightMapBox
	{
		glm::daabb m_Box;
		glm::vec4 m_SHResult[1024];// 64(4*4*4), 16
		int m_Children[8];
		glm::ivec2 m_TriangleRange;
		glm::ivec2 m_LightRange;
	};
	struct LightMapBoxCache
	{
		glm::daabb m_Box;
		LightMapBoxCache *m_pChildren[8];
		std::vector<unsigned int> m_Triangles;
		std::vector<glm::ivec2> m_Lights;
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
		int m_StateAndDepth;
		glm::vec3 m_Color;
		int m_BoxID;
		glm::vec3 m_Emmited;
		int m_HarmonicsID;
	};
	void assignTriangle(glm::vec3 &a_Pos1, glm::vec3 &a_Pos2, glm::vec3 &a_Pos3, LightMapBoxCache *a_pRoot, std::set<LightMapBoxCache*> &a_Output);
	void assignTriangle(glm::vec3 &a_Pos1, glm::vec3 &a_Pos2, LightMapBoxCache *a_pCurrNode, std::set<LightMapBoxCache*> &a_Output);
	void assignLight(Light *a_pLight, LightMapBoxCache *a_pRoot, std::vector<LightMapBoxCache*> &a_Output);

	boost::property_tree::ptree m_Cache;

	bool m_bBaking;
	std::shared_ptr<Asset> m_pRayIntersectMat;
	MaterialAsset *m_pRayIntersectMatInst;
	std::shared_ptr<MaterialBlock> m_pTriangles, m_pLightIdx, m_pVertex, m_pResult;
	std::mutex m_BakeLock;

	std::shared_ptr<MaterialBlock> m_pBoxes;
	std::vector<LightMapBox> m_LightMap;
	std::vector<std::shared_ptr<Asset>> m_LightMapMaterialCache;

};

}

#endif