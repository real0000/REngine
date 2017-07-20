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
/*
class TextureAtlas
{
public:
	struct NodeID
	{
		unsigned int m_Index;
		unsigned int m_ID;
		glm::ivec2 m_Offset;
		glm::ivec2 m_Size;
	};

	TextureAtlas(unsigned int a_Size, unsigned int a_InitArraySize = 1, PixelFormat::Key a_Format = PixelFormat::rgba8_unorm);
	virtual ~TextureAtlas();

	bool allocate(unsigned int a_Width, unsigned int a_Heigth, glm::vec4 &a_Output);
	void release(unsigned int a_NodeID);
	void updateBuffer(void *a_pData, unsigned int a_OffsetX, unsigned int a_OffsetY, int a_Width, int a_Height);// nullptr means cleanup
	void updateBuffer(void *a_pData, glm::ivec2 a_Offset, glm::ivec2 a_Size);
	bool empty();
	
	TextureUnitPtr getTexture(){ return m_pTexture; }

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

	bool insertNode(SplitNode *a_pNode, unsigned int a_Width, unsigned int a_Height, glm::vec4 &a_Output, unsigned int &a_NodeID);
	
	SplitNode *m_pRoot;
	TextureUnitPtr m_pTexture;//Texture2DArray
	char *m_pUpdateBuffer;
	unsigned int m_Serial;
	unsigned int m_CurrArraySize;
	unsigned int m_ExtendSize;
};*/

}

#endif