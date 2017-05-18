// DrawBuffer.cpp
//
// 2017/05/08 Ian Wu/Real0000
//

#include "RGDeviceWrapper.h"

namespace R
{

#pragma region IndexBuffer
//
// IndexBuffer
//
IndexBuffer::IndexBuffer()
	: m_pInitVal(new InitInfo{nullptr, std::string("")})
	, m_ID(-1)
	, m_b32Bit(false)
	, m_NumIndicies(0)
{
}

IndexBuffer::~IndexBuffer()
{
	SAFE_DELETE(m_pInitVal);
	if( -1 != m_ID ) GDEVICE()->freeIndexBuffer(m_ID);
}

void IndexBuffer::setIndicies(bool a_b32Bit, void *a_pSrc, unsigned int a_NumIndicies)
{
	assert(nullptr != m_pInitVal);
	m_b32Bit = a_b32Bit;
	m_pInitVal->m_pRefIndicies = a_pSrc;
	m_NumIndicies = a_NumIndicies;
}

void IndexBuffer::setName(std::string a_Name)
{
	assert(nullptr != m_pInitVal);
	m_pInitVal->m_Name = a_Name;
}

void IndexBuffer::bind(GraphicCanvas *a_pCanvas)
{
	assert(-1 != m_ID);
	GraphicCommander *l_pCommander = a_pCanvas->getCommander();
	if( nullptr == l_pCommander ) return ;
	l_pCommander->bindIndex(m_ID);
}

void IndexBuffer::init()
{
	assert(nullptr != m_pInitVal);
	m_ID = GDEVICE()->requestIndexBuffer(m_pInitVal->m_pRefIndicies, m_b32Bit ? PixelFormat::r32_uint : PixelFormat::r16_uint, m_NumIndicies, m_pInitVal->m_Name);
	if( -1 == m_ID ) return ;

	SAFE_DELETE(m_pInitVal);
}
#pragma endregion

#pragma region VertexBuffer
//
// VertexBuffer
//
VertexBuffer::VertexBuffer()
	: m_pInitVal(new InitInfo{{nullptr}, "", 0})
{
	memset(m_ID, -1, sizeof(int) * VTXSLOT_COUNT);
}

VertexBuffer::~VertexBuffer()
{
	for( unsigned int i=0 ; i<VTXSLOT_COUNT ; ++i )
	{
		if( -1 != m_ID[i] ) GDEVICE()->freeVertexBuffer(m_ID[i]);
	}
	SAFE_DELETE(m_pInitVal);
}

void VertexBuffer::setVertex(unsigned int a_Slot, void *a_pSrc)
{
	assert(nullptr != m_pInitVal);
	assert(VTXSLOT_COUNT > a_Slot);
	m_pInitVal->m_pRefVtx[a_Slot] = a_pSrc;
}

void VertexBuffer::setNumVertex(unsigned int a_NumVtx)
{
	assert(nullptr != m_pInitVal);
	assert(0 == a_NumVtx);
	m_pInitVal->m_NumVtx = a_NumVtx;
}

void VertexBuffer::setName(std::string a_Name)
{
	assert(nullptr != m_pInitVal);
	m_pInitVal->m_Name = a_Name;
}

void VertexBuffer::init()
{
	assert(nullptr != m_pInitVal);
	for( unsigned int i=0 ; i<VTXSLOT_COUNT ; ++i )
	{
		if( nullptr != m_pInitVal->m_pRefVtx[i] )
		{
			m_ID[i] = GDEVICE()->requestVertexBuffer(m_pInitVal->m_pRefVtx[i], m_pInitVal->m_NumVtx, m_pInitVal->m_Name);
			if( -1 == m_ID[i] )
			{
				for( unsigned int j=0 ; j<i ; ++j )
				{
					if( -1 != m_ID[i] ) GDEVICE()->freeVertexBuffer(m_ID[i]);
				}
				return;
			}
		}
	}

	SAFE_DELETE(m_pInitVal);
}

void VertexBuffer::bind(GraphicCanvas *a_pCanvas)
{
	assert(nullptr == m_pInitVal);
	GraphicCommander *l_pCommander = a_pCanvas->getCommander();
	if( nullptr == l_pCommander ) return ;

	for( unsigned int i=0 ; i<VTXSLOT_COUNT ; ++i )
	{
		if( -1 != m_ID[i] ) l_pCommander->bindVertex(m_ID[i]);
	}
}

void VertexBuffer::updateVertexData(unsigned int a_Slot, void *a_pData, unsigned int a_Count)
{
	assert(nullptr == m_pInitVal);
	unsigned int l_VtxSize = 0;
	switch( a_Slot )
	{
		case VTXSLOT_POSITION:	l_VtxSize = sizeof(glm::vec3); break;
		case VTXSLOT_TEXCOORD01:
		case VTXSLOT_TEXCOORD23:
		case VTXSLOT_TEXCOORD45:
		case VTXSLOT_TEXCOORD67:l_VtxSize = sizeof(glm::vec4); break;
		case VTXSLOT_NORMAL:	l_VtxSize = sizeof(glm::vec3); break;
		case VTXSLOT_TANGENT:	l_VtxSize = sizeof(glm::vec3); break;
		case VTXSLOT_BINORMAL:	l_VtxSize = sizeof(glm::vec3); break;
		case VTXSLOT_BONE:		l_VtxSize = sizeof(glm::ivec4); break;
		case VTXSLOT_WEIGHT:	l_VtxSize = sizeof(glm::vec4); break;
		case VTXSLOT_COLOR:		l_VtxSize = sizeof(unsigned int); break;
		default:break;
	}
	assert(0 != l_VtxSize);
	if( -1 != m_ID[a_Slot] ) GDEVICE()->updateVertexBuffer(m_ID[a_Slot], a_pData, a_Count * l_VtxSize);
	
}
#pragma endregion

}
