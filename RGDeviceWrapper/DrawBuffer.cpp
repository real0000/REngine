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

void IndexBuffer::setName(wxString a_Name)
{
	assert(nullptr != m_pInitVal);
	m_pInitVal->m_Name = a_Name;
}

void IndexBuffer::init()
{
	assert(nullptr != m_pInitVal);
	m_ID = GDEVICE()->requestIndexBuffer(m_pInitVal->m_pRefIndicies, m_b32Bit ? PixelFormat::r32_uint : PixelFormat::r16_uint, m_NumIndicies, m_pInitVal->m_Name);
	if( -1 == m_ID ) return ;

	SAFE_DELETE(m_pInitVal);
}

void IndexBuffer::updateIndexData(void *a_pData, unsigned int a_Count, unsigned int a_Offset)
{
	assert(nullptr == m_pInitVal);
	unsigned int l_Stride = m_b32Bit ? sizeof(unsigned int) : sizeof(unsigned short);
	GDEVICE()->updateIndexBuffer(m_ID, a_pData, a_Count * l_Stride, a_Offset * l_Stride);
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
	assert(0 != a_NumVtx);
	m_pInitVal->m_NumVtx = a_NumVtx;
}

void VertexBuffer::setName(wxString a_Name)
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
			wxString a_Name(wxString::Format(wxT("%s[%d]"), m_pInitVal->m_Name, i));
			m_ID[i] = GDEVICE()->requestVertexBuffer(m_pInitVal->m_pRefVtx[i], i, m_pInitVal->m_NumVtx, a_Name);
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

void VertexBuffer::updateVertexData(unsigned int a_Slot, void *a_pData, unsigned int a_Count, unsigned int a_Offset)
{
	assert(nullptr == m_pInitVal);
	unsigned int l_Stride = GDEVICE()->getVertexSlotStride(a_Slot);
	if( -1 != m_ID[a_Slot] ) GDEVICE()->updateVertexBuffer(m_ID[a_Slot], a_pData, a_Count * l_Stride, a_Offset * l_Stride);
}
#pragma endregion

#pragma region IndirectDrawBuffer
//
// IndirectDrawBuffer
//
IndirectDrawBuffer::IndirectDrawBuffer()
	: m_pBuff(nullptr)
	, m_ID(-1)
	, m_CurrOffset(0)
	, m_MaxSize(1024)
{
	m_ID = GDEVICE()->requestIndrectCommandBuffer(m_pBuff, sizeof(IndirectDrawData) * m_MaxSize);
}

IndirectDrawBuffer::~IndirectDrawBuffer()
{
	GDEVICE()->freeIndrectCommandBuffer(m_ID);
}

void IndirectDrawBuffer::assign(IndirectDrawData &a_Data)
{
	if( m_CurrOffset >= m_MaxSize )
	{
		char *l_pNewBuff = nullptr;
		int l_NewID = GDEVICE()->requestIndrectCommandBuffer(l_pNewBuff, sizeof(IndirectDrawData) * (m_MaxSize + 1024));
		memcpy(l_pNewBuff, m_pBuff, m_MaxSize * sizeof(IndirectDrawData));
		GDEVICE()->freeIndrectCommandBuffer(m_ID);
		m_ID = l_NewID;
		m_pBuff = l_pNewBuff;
		m_MaxSize += 1024;
	}

	memcpy(m_pBuff + sizeof(IndirectDrawData) * m_CurrOffset, &a_Data, sizeof(IndirectDrawData));
	++m_CurrOffset;
}

void IndirectDrawBuffer::reset()
{
	m_CurrOffset = 0;
}
#pragma endregion

}

