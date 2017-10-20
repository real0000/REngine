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
//
// RenderTextureAtlas
//
RenderTextureAtlas::RenderTextureAtlas(glm::ivec2 a_Size, PixelFormat::Key a_Format, unsigned int a_InitArraySize, bool a_bCube)
	: m_Atlas(a_Size, a_InitArraySize)
	, m_pTexture(nullptr)
	, m_bCube(a_bCube)

{
	assert(!m_bCube && 0 == (a_InitArraySize % 6));
	if( m_bCube ) m_Atlas.setExtendSize(6);

	assert(a_Size.x >= 64 && a_Size.y >= 64);
	m_pTexture = TextureManager::singleton().createRenderTarget(wxT("RenderTextureAtlas"), a_Size, a_Format, a_InitArraySize, a_bCube);
}

RenderTextureAtlas::~RenderTextureAtlas()
{
	TextureManager::singleton().recycle(m_pTexture->getName());
	m_pTexture = nullptr;
}

unsigned int RenderTextureAtlas::allocate(glm::ivec2 a_Size)
{
	unsigned int l_PrevSize = m_Atlas.getArraySize();
	unsigned int l_Res = m_Atlas.allocate(a_Size);
	if( l_PrevSize != m_Atlas.getArraySize() )
	{
		PixelFormat::Key l_Format = m_pTexture->getTextureFormat();
		m_pTexture = TextureManager::singleton().createRenderTarget(wxT("RenderTextureAtlas"), m_Atlas.getMaxSize(), l_Format, m_Atlas.getArraySize(), m_bCube);
	}
	return l_Res;
}

void RenderTextureAtlas::release(unsigned int a_ID)
{
	m_Atlas.release(a_ID);
}

void RenderTextureAtlas::releaseAll()
{
	m_Atlas.releaseAll();
}

void RenderTextureAtlas::setExtendSize(unsigned int a_Extend)
{
	assert(!m_bCube && 0 == (a_Extend % 6));
	m_Atlas.setExtendSize(a_Extend);
}
#pragma endregion

}