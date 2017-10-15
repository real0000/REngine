// TextureAtlas.cpp
//
// 2014/04/10 Ian Wu/Real0000
//

#include "CommonUtil.h"

#include "Texture.h"
#include "TextureAtlas.h"

namespace R
{
	
#pragma region RenderTextureAtlas
#pragma region RenderTextureAtlas::SplitNode
//
// RenderTextureAtlas::SplitNode
//
RenderTextureAtlas::SplitNode::SplitNode()
	: m_pParent(nullptr), m_pLeft(nullptr), m_pRight(nullptr)
	, m_bUsed(false)
{
	memset(&m_NodeID, 0, sizeof(NodeID));
}

RenderTextureAtlas::SplitNode::~SplitNode()
{
	SAFE_DELETE(m_pLeft);
	SAFE_DELETE(m_pRight);
}
RenderTextureAtlas::SplitNode* RenderTextureAtlas::SplitNode::find(unsigned int a_ID)
{
	if( a_ID == m_NodeID.m_ID ) return this;

	RenderTextureAtlas::SplitNode *l_pOutput = nullptr;
	if( nullptr != m_pLeft ) l_pOutput = m_pLeft->find(a_ID);
	if( nullptr != l_pOutput ) return l_pOutput;
	if( nullptr != m_pRight ) l_pOutput = m_pRight->find(a_ID);
	return l_pOutput;
}
#pragma endregion

//
// RenderTextureAtlas
//
RenderTextureAtlas::RenderTextureAtlas(glm::ivec2 a_Size, PixelFormat::Key a_Format, unsigned int a_InitArraySize, bool a_bCube)
	: m_pTexture(nullptr)
	, m_MaxSize(a_Size)
	, m_Serial(0)
	, m_CurrArraySize(1)
	, m_ExtendSize(a_bCube ? 6 : 1)
	, m_bCube(a_bCube)

{
	assert(a_Size.x >= 64 && a_Size.y >= 64);
	m_pTexture = TextureManager::singleton().createRenderTarget(wxT("RenderTextureAtlas"), a_Size, a_Format, a_InitArraySize, a_bCube);

	m_Roots.push_back(new SplitNode());
	m_Roots.front()->m_NodeID.m_Size = a_Size;
	m_Roots.front()->m_NodeID.m_Index = 0;
	m_Roots.front()->m_NodeID.m_ID = 0;
	m_Roots.front()->m_NodeID.m_Offset = glm::zero<glm::ivec2>();
}

RenderTextureAtlas::~RenderTextureAtlas()
{
	TextureManager::singleton().recycle(m_pTexture->getName());
	m_pTexture = nullptr;
	for( unsigned int i=0 ; i<m_Roots.size() ; ++i ) delete m_Roots[i];
	m_Roots.clear();
}

void RenderTextureAtlas::allocate(glm::ivec2 a_Size, NodeID &a_Output)
{
	assert(a_Size.x <= m_MaxSize.x && a_Size.y <= m_MaxSize.y);
	a_Output.m_Offset = a_Output.m_Size = glm::ivec2(0, 0);
	bool l_bFound = false;
	for( unsigned int i=0 ; i<m_Roots.size() ; ++i )
	{
		if( insertNode(m_Roots[i], a_Size, a_Output) )
		{
			l_bFound = true;
			break;
		}
	}

	if( l_bFound ) return;

	PixelFormat::Key l_Format = m_pTexture->getTextureFormat();
	m_pTexture->release();
	m_CurrArraySize += m_ExtendSize;
	m_pTexture = TextureManager::singleton().createRenderTarget(wxT("RenderTextureAtlas"), m_MaxSize, l_Format, m_CurrArraySize, m_bCube);
	for( unsigned int i=0 ; i<m_ExtendSize ; ++i )
	{
		m_Roots.push_back(new SplitNode());
		m_Roots.back()->m_NodeID.m_Size = m_MaxSize;
		m_Roots.back()->m_NodeID.m_Index = m_Roots.size() - 1;
		m_Roots.back()->m_NodeID.m_ID = m_Serial;
		m_Roots.back()->m_NodeID.m_Offset = glm::zero<glm::ivec2>();
		++m_Serial;
	}
	insertNode(m_Roots[m_Roots.size() - m_ExtendSize], a_Size, a_Output);
}

void RenderTextureAtlas::release(NodeID &a_Region)
{
	SplitNode *l_pNode = m_Roots[a_Region.m_Index]->find(a_Region.m_ID);
	if( nullptr == l_pNode ) return;
	if( nullptr == l_pNode->m_pParent )
	{
		l_pNode->m_bUsed = false;
		return;
	}

	SplitNode *l_pParent = l_pNode->m_pParent;

	if( l_pParent->m_pLeft == l_pNode ) l_pParent->m_pLeft = nullptr;
	else l_pParent->m_pRight = nullptr;
 
	delete l_pNode;

	if( nullptr == l_pParent->m_pLeft && nullptr == l_pParent->m_pRight ) release(l_pParent->m_NodeID);
}

void RenderTextureAtlas::setExtendSize(unsigned int a_Extend)
{
	assert(a_Extend >= 1);
	m_ExtendSize = a_Extend;
	if( m_bCube && 0 != (m_ExtendSize % 6) ) m_ExtendSize = 6 - (m_ExtendSize % 6);
}

bool RenderTextureAtlas::insertNode(SplitNode *a_pNode, glm::ivec2 a_Size, NodeID &a_Output)
{
	if( nullptr != a_pNode->m_pLeft || nullptr != a_pNode->m_pRight )
    {
		if( insertNode(a_pNode->m_pLeft, a_Size, a_Output) ) return true;
		return insertNode(a_pNode->m_pRight, a_Size, a_Output);
    }
    else
    {
		if( a_pNode->m_bUsed ) return false;
        
        if( a_pNode->m_NodeID.m_Size.x < a_Size.x || a_pNode->m_NodeID.m_Size.y < a_Size.y ) return false;
        if( a_pNode->m_NodeID.m_Size.x == a_Size.x && a_pNode->m_NodeID.m_Size.y == a_Size.y )
        {
			a_pNode->m_bUsed = true;
			a_Output = a_pNode->m_NodeID;

            return true;
        }
        
        SplitNode *l_pLeft = new SplitNode();
		a_pNode->m_pLeft = l_pLeft;
		l_pLeft->m_NodeID.m_ID = m_Serial;
		l_pLeft->m_NodeID.m_Index = a_pNode->m_NodeID.m_Index;
		++m_Serial;

		SplitNode *l_pRight = new SplitNode();
		a_pNode->m_pRight = l_pRight;
		l_pRight->m_NodeID.m_ID = m_Serial;
		l_pRight->m_NodeID.m_Index = a_pNode->m_NodeID.m_Index;
		++m_Serial;

		unsigned int l_DeltaWidth = a_pNode->m_NodeID.m_Size.x - a_Size.x;
        unsigned int l_DeltaHeight = a_pNode->m_NodeID.m_Size.y - a_Size.y;
        if( l_DeltaWidth < l_DeltaHeight )
        {
            l_pLeft->m_NodeID.m_Offset = a_pNode->m_NodeID.m_Offset;
            l_pLeft->m_NodeID.m_Size.x = a_pNode->m_NodeID.m_Size.x;
            l_pLeft->m_NodeID.m_Size.y = a_Size.y;
            
            l_pRight->m_NodeID.m_Offset.x = a_pNode->m_NodeID.m_Offset.x;
            l_pRight->m_NodeID.m_Offset.y = a_pNode->m_NodeID.m_Offset.y + a_Size.y;
            l_pRight->m_NodeID.m_Size.x = a_pNode->m_NodeID.m_Size.x;
            l_pRight->m_NodeID.m_Size.y = a_pNode->m_NodeID.m_Size.y - a_Size.y;
        }
        else
        {
            l_pLeft->m_NodeID.m_Offset = a_pNode->m_NodeID.m_Offset;
            l_pLeft->m_NodeID.m_Size.x = a_Size.x;
            l_pLeft->m_NodeID.m_Size.y = a_pNode->m_NodeID.m_Size.y;
            
            l_pRight->m_NodeID.m_Offset.x = a_pNode->m_NodeID.m_Offset.x + a_Size.x;
            l_pRight->m_NodeID.m_Offset.y = a_pNode->m_NodeID.m_Offset.y;
            l_pRight->m_NodeID.m_Size.x = a_pNode->m_NodeID.m_Size.x - a_Size.x;
            l_pRight->m_NodeID.m_Size.y = a_pNode->m_NodeID.m_Size.y;
        }
        
        return insertNode(a_pNode->m_pLeft, a_Size, a_Output);
    }

    return false;
}
#pragma endregion

}