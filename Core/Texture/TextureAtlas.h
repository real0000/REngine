// TextureAtlas.h
//
// 2014/04/10 Ian Wu/Real0000
//

#ifndef _TEXTURE_ATLAS_H_
#define _TEXTURE_ATLAS_H_

#include "RGDeviceWrapper.h"

namespace R
{

class TextureManager;
class TextureUnit;

// to do : split algorithm to uniq class
class RenderTextureAtlas
{
public:
	struct NodeID
	{
		NodeID() : m_Index(0), m_ID(0), m_Offset(0, 0), m_Size(0, 0){}

		unsigned int m_Index;
		unsigned int m_ID;
		glm::ivec2 m_Offset;
		glm::ivec2 m_Size;
	};

	RenderTextureAtlas(glm::ivec2 a_Size, PixelFormat::Key a_Format = PixelFormat::rgba8_unorm, unsigned int a_InitArraySize = 1, bool a_bCube = false);
	virtual ~RenderTextureAtlas();

	void allocate(glm::ivec2 a_Size, NodeID &a_Output);
	void release(NodeID &a_Region);
	
	std::shared_ptr<TextureUnit> getTexture(){ return m_pTexture; }
	void setExtendSize(unsigned int a_Extend);

private:
	struct SplitNode
	{
		SplitNode();
		~SplitNode();

		SplitNode* find(unsigned int a_ID);
	    
		SplitNode *m_pParent;
		SplitNode *m_pLeft, *m_pRight;
		NodeID m_NodeID;
		bool m_bUsed;
	};

	bool insertNode(SplitNode *a_pNode, glm::ivec2 a_Size, NodeID &a_Output);
	
	std::vector<SplitNode *> m_Roots;
	std::shared_ptr<TextureUnit> m_pTexture;
	glm::ivec2 m_MaxSize;
	unsigned int m_Serial;
	unsigned int m_CurrArraySize;
	unsigned int m_ExtendSize;
	bool m_bCube;
};

}

#endif