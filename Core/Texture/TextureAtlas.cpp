// TextureAtlas.cpp
//
// 2014/04/10 Ian Wu/Real0000
//

#include "CommonUtil.h"

#include "Texture.h"
#include "TextureAtlas.h"

namespace R
{
	/*
#pragma region TextureAtlas
#pragma region TextureAtlas::SplitNode
//
// TextureAtlas::SplitNode
//
TextureAtlas::SplitNode::SplitNode()
	: m_pParent(nullptr), m_pLeft(nullptr), m_pRight(nullptr)
	, m_bUsed(false)
{
	memset(&m_NodeID, 0, sizeof(NodeID));
}

TextureAtlas::SplitNode::~SplitNode()
{
	SAFE_DELETE(m_pLeft);
	SAFE_DELETE(m_pRight);
}
TextureAtlas::SplitNode* TextureAtlas::SplitNode::find(unsigned int a_ID)
{
	if( a_ID == m_NodeID.m_ID ) return this;

	TextureAtlas::SplitNode *l_pOutput = nullptr;
	if( nullptr != m_pLeft ) l_pOutput = m_pLeft->find(a_ID);
	if( nullptr != l_pOutput ) return l_pOutput;
	if( nullptr != m_pRight ) l_pOutput = m_pRight->find(a_ID);
	return l_pOutput;
}
#pragma endregion

//
// TextureAtlas
//
TextureAtlas::TextureAtlas(unsigned int a_Size, unsigned int a_InitArraySize, PixelFormat::Key a_Format)
	: m_pRoot(nullptr), m_pTexture(nullptr)
	, m_pUpdateBuffer(nullptr)
	, m_Serial(0)
	, m_CurrArraySize(1)
	, m_ExtendSize()

{
	assert(a_Size >= 64);
	m_pTexture = TextureManager::singleton().newTexture2DArray();
	m_pTexture->init(a_Size, a_Size, a_Format, nullptr);
	m_pUpdateBuffer = new char[l_Size * l_Size * m_pTexture->getPixelSize()];
	memset(m_pUpdateBuffer, 0, sizeof(char) * l_Size * l_Size);

	m_pRoot = new SplitNode();
	m_pRoot->m_Size[0] = m_pRoot->m_Size[1] = m_pTexture->getSize().x;
}

TextureAtlas::~TextureAtlas()
{
	TextureManager::singleton().recycle(m_pTexture);
	delete[] m_pUpdateBuffer;
	delete m_pRoot;
}

bool TextureAtlas::allocate(unsigned int l_Width, unsigned int l_Heigth, glm::vec4 &l_Output, unsigned int &l_NodeID)
{
	l_Output = glm::vec4(0.0f);

	if( int(l_Width) > m_pTexture->getSize().x || int(l_Heigth) > m_pTexture->getSize().y ) return false;
	return insertNode(m_pRoot, l_Width, l_Heigth, l_Output, l_NodeID);
}

void TextureAtlas::release(unsigned int l_NodeID)
{
	TextureAtlas::SplitNode *l_pNode = m_pRoot->find(l_NodeID);
	if( NULL == l_pNode || l_pNode == m_pRoot ) return;

	TextureAtlas::SplitNode *l_pParent = l_pNode->m_pParent;

	if( l_pParent->m_pLeft == l_pNode ) l_pParent->m_pLeft = NULL;
	else l_pParent->m_pRight = NULL;
 
	delete l_pNode;

	if( NULL == l_pParent->m_pLeft && NULL == l_pParent->m_pRight )
		release(l_pParent->m_NodeID);
}

void TextureAtlas::updateBuffer(void *l_pData, unsigned int l_OffsetX, unsigned int l_OffsetY, int l_Width, int l_Height, int l_Pitch)
{
	unsigned int l_PixelSize = m_pTexture->getPixelSize();
	memset(m_pUpdateBuffer, 0, l_Pitch * l_Height * l_PixelSize);
    for( int y=0 ; y<l_Height ; y++ )
    {
        memcpy(m_pUpdateBuffer + y * l_Pitch * l_PixelSize, (void *)( ((char *)l_pData) + (l_Pitch - l_Width) * l_PixelSize + l_Pitch * (l_Height - y - 1)), l_Width * l_PixelSize);
    }
	m_pTexture->update(m_pUpdateBuffer, l_OffsetX, l_OffsetY, l_Width, l_Height, l_Pitch);
}

bool TextureAtlas::empty()
{
	return NULL == m_pRoot->m_pLeft && NULL == m_pRoot->m_pRight;
}

bool TextureAtlas::insertNode(SplitNode *l_pNode, unsigned int l_Width, unsigned int l_Height, glm::vec4 &l_Output, unsigned int &l_NodeID)
{
	if( NULL != l_pNode->m_pLeft || NULL != l_pNode->m_pRight )
    {
		if( insertNode(l_pNode->m_pLeft, l_Width, l_Height, l_Output, l_NodeID) )
            return true;
		return insertNode(l_pNode->m_pRight, l_Width, l_Height, l_Output, l_NodeID);
    }
    else
    {
		if( l_pNode->m_bUsed ) return false;
        
        if( l_pNode->m_Size[0] < l_Width || l_pNode->m_Size[1] < l_Height ) return false;
        if( l_pNode->m_Size[0] == l_Width && l_pNode->m_Size[1] == l_Height )
        {
			l_pNode->m_bUsed = true;
			l_Output.x = l_pNode->m_Position[0];
			l_Output.y = l_pNode->m_Position[1];
			l_Output.z = l_Width;
			l_Output.w = l_Height;
			l_NodeID = l_pNode->m_NodeID;

            return true;
        }
        
        SplitNode *l_pLeft = new SplitNode();
		l_pNode->m_pLeft = l_pLeft;
		l_pLeft->m_NodeID = m_Serial;
		m_Serial++;

		SplitNode *l_pRight = new SplitNode();
		l_pNode->m_pRight = l_pRight;
		l_pRight->m_NodeID = m_Serial;
		m_Serial++;

		unsigned int l_DeltaWidth = l_pNode->m_Size[0] - l_Width;
        unsigned int l_DeltaHeight = l_pNode->m_Size[1] - l_Height;
        if( l_DeltaWidth < l_DeltaHeight )
        {
            memcpy(l_pLeft->m_Position, l_pNode->m_Position, sizeof(unsigned int) * 2);
            l_pLeft->m_Size[0] = l_pNode->m_Size[0];
            l_pLeft->m_Size[1] = l_Height;
            
            l_pRight->m_Position[0] = l_pNode->m_Position[0];
            l_pRight->m_Position[1] = l_pNode->m_Position[1] + l_Height;
            l_pRight->m_Size[0] = l_pNode->m_Size[0];
            l_pRight->m_Size[1] = l_pNode->m_Size[1] - l_Height;
        }
        else
        {
            memcpy(l_pLeft->m_Position, l_pNode->m_Position, sizeof(unsigned int) * 2);
            l_pLeft->m_Size[0] = l_Width;
            l_pLeft->m_Size[1] = l_pNode->m_Size[1];
            
            l_pRight->m_Position[0] = l_pNode->m_Position[0] + l_Width;
            l_pRight->m_Position[1] = l_pNode->m_Position[1];
            l_pRight->m_Size[0] = l_pNode->m_Size[0] - l_Width;
            l_pRight->m_Size[1] = l_pNode->m_Size[1];
        }
        
        return insertNode(l_pNode->m_pLeft, l_Width, l_Height, l_Output, l_NodeID);
    }

    return false;
}

//
// TextureAtlasSet
//
static unsigned int s_TextureSerial = 0;
TextureAtlasSet::TextureAtlasSet(unsigned int l_Size, GLenum l_Format)
	: m_Size(l_Size)
	, m_Format(l_Format)
{
	TextureAtlas *l_pBase = new TextureAtlas(l_Size, l_Format);
	m_Textures[s_TextureSerial] = l_pBase;
	++s_TextureSerial;
}

TextureAtlasSet::~TextureAtlasSet()
{
	for( auto it = m_Textures.begin() ; it != m_Textures.end() ; it++ ) delete it->second;
	m_Textures.clear();
}

void TextureAtlasSet::allocate(unsigned int l_Width, unsigned int l_Heigth, glm::vec4 &l_Output, unsigned int &l_TextureID, unsigned int &l_NodeID)
{
	for( auto it = m_Textures.begin() ; it != m_Textures.end() ; it++ )
	{
		if( it->second->allocate(l_Width, l_Heigth, l_Output, l_NodeID) )
		{
			l_TextureID = it->first;
			return;
		}
	}

	TextureAtlas *l_pNewTexture = new TextureAtlas(m_Size, m_Format);
	m_Textures[s_TextureSerial] = l_pNewTexture;
	++s_TextureSerial;
	l_pNewTexture->allocate(l_Width, l_Heigth, l_Output, l_NodeID);
}

void TextureAtlasSet::release(unsigned int l_TextureID, unsigned int l_NodeID)
{
	auto it = m_Textures.find(l_TextureID);
	assert(it != m_Textures.end() && "invalid texture ID");
	it->second->release(l_NodeID);
	if( it->second->empty() )
	{
		delete it->second;
		m_Textures.erase(it);
	}
}

void TextureAtlasSet::updateBuffer(unsigned int l_TextureID, void *l_pData, unsigned int l_OffsetX, unsigned int l_OffsetY, int l_Width, int l_Height, int l_Pitch)
{
	std::map<GLuint, TextureAtlas *>::iterator it = m_Textures.find(l_TextureID);
	assert(it != m_Textures.end() && "invalid texture ID");
	it->second->updateBuffer(l_pData, l_OffsetX, l_OffsetY, l_Width, l_Height, l_Pitch);
}
#pragma endregion
*/
}