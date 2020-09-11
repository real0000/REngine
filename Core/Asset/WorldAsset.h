// WorldAsset.h
//
// 2020/09/11 Ian Wu/Real0000
//

#ifndef _WORLD_ASSET_H_
#define _WORLD_ASSET_H_

#include "AssetBase.h"

namespace R
{

class WorldAsset : public AssetComponent
{
public:

private:
	struct LightMapBox
	{
		glm::daabb m_Box;
		glm::vec4 m_SHResult[1024];// 64(4*4*4), 16
		int m_Children[8];
		glm::ivec2 m_TriangleRange;
		glm::ivec2 m_LightRange;
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

	boost::property_tree::ptree m_Cache;
	unsigned int m_UavID;
	std::vector<LightMapBox *> m_LightMap;
	std::vector<std::shared_ptr<Asset>> m_LightMapMaterialCache;
};
/*

*/

}

#endif