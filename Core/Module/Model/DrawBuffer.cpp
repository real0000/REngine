// DrawBuffer.cpp
//
// 2015/02/25 Ian Wu/Real0000
//

#include "CommonUtil.h"
#include "DrawBuffer.h"

namespace R
{

#pragma region VertexPosTex2NormalBone
//
// VertexPosTex2NormalBone
//
VertexPosTex2NormalBone::VertexPosTex2NormalBone()
	: m_Position(0.0f, 0.0f, 0.0f)
	, m_Normal(0.0f, 1.0f, 0.0f), m_Tangent(0.0f, 0.0f, 1.0f), m_Binormal(1.0f, 0.0f, 0.0f)
	, m_BoneWeight(1.0f, 0.0f, 0.0f, 0.0f)
{
	m_Texcoord[0] = m_Texcoord[1] = glm::vec2(0.0f, 0.0f);
	m_BoneID[0] = m_BoneID[1] = m_BoneID[2] = m_BoneID[3] = -1;
}

void VertexPosTex2NormalBone::bindVertexFormat(char *l_pBase)
{
	const unsigned int c_PositionBase = 0;
	const unsigned int c_TexcoordBase = c_PositionBase + sizeof(glm::vec3);
	const unsigned int c_NormalBase = c_TexcoordBase + sizeof(glm::vec2) * 2;
	const unsigned int c_TangentBase = c_NormalBase + sizeof(glm::vec3);
	const unsigned int c_BinormalBase = c_TangentBase + sizeof(glm::vec3);
	const unsigned int c_BoneBase = c_BinormalBase + sizeof(glm::vec3);
	const unsigned int c_WeightBase = c_BoneBase + sizeof(unsigned char) * 4;
	
	for( unsigned int i=0 ; i<7 ; ++i ) glEnableVertexAttribArray(i);
	glVertexAttribPointer(VTXSLOT_POSITION, 3, GL_FLOAT, GL_FALSE, getStride(), l_pBase + c_PositionBase);
	glVertexAttribPointer(VTXSLOT_TEXCOORD, 4, GL_FLOAT, GL_FALSE, getStride(), l_pBase + c_TexcoordBase);
	glVertexAttribPointer(VTXSLOT_NORMAL, 3, GL_FLOAT, GL_FALSE, getStride(), l_pBase + c_NormalBase);
	glVertexAttribPointer(VTXSLOT_TANGENT, 3, GL_FLOAT, GL_FALSE, getStride(), l_pBase + c_TangentBase);
	glVertexAttribPointer(VTXSLOT_BINORMAL, 3, GL_FLOAT, GL_FALSE, getStride(), l_pBase + c_BinormalBase);
	glVertexAttribPointer(VTXSLOT_BONE, 4, GL_UNSIGNED_BYTE, GL_FALSE, getStride(), l_pBase + c_BoneBase);
	glVertexAttribPointer(VTXSLOT_WEIGHT, 4, GL_FLOAT, GL_FALSE, getStride(), l_pBase + c_WeightBase);
	glDisableVertexAttribArray(7);
}
#pragma endregion

#pragma region VertexPositionColor
//
// VertexPositionColor
//
VertexPositionColor::VertexPositionColor()
	: m_Position(0.0f, 0.0f, 0.0f)
	, m_Color(0.0f, 0.0f, 0.0f, 0.0f)
{

}

void VertexPositionColor::bindVertexFormat(char *l_pBase)
{
	const unsigned int c_PositionBase = 0;
	const unsigned int c_ColorBase = c_PositionBase + sizeof(glm::vec3);
	glEnableVertexAttribArray(VTXSLOT_POSITION);
	glDisableVertexAttribArray(VTXSLOT_TEXCOORD);
	glDisableVertexAttribArray(VTXSLOT_NORMAL);
	glDisableVertexAttribArray(VTXSLOT_TANGENT);
	glDisableVertexAttribArray(VTXSLOT_BINORMAL);
	glDisableVertexAttribArray(VTXSLOT_BONE);
	glDisableVertexAttribArray(VTXSLOT_WEIGHT);
	glEnableVertexAttribArray(VTXSLOT_COLOR);

	glVertexAttribPointer(VTXSLOT_POSITION, 3, GL_FLOAT, GL_FALSE, getStride(), l_pBase + c_PositionBase);
	glVertexAttribPointer(VTXSLOT_COLOR, 4, GL_FLOAT, GL_FALSE, getStride(), l_pBase + c_ColorBase);
}
#pragma endregion

#pragma region VertexBuffer
//
// VertexBuffer
//
GLuint VertexBuffer::m_CachedID = 0;
VertexBuffer::VertexBuffer(VertexBinder *l_pBinder)
	: m_pBinder(l_pBinder)
	, m_VertexBuffer(0)
{
}
	
VertexBuffer::~VertexBuffer()
{
	if( 0 != m_VertexBuffer ) glDeleteBuffers(1, &m_VertexBuffer);
}

void VertexBuffer::init(void *l_pSrcData, unsigned int l_SizeInByte)
{
	glGenBuffers(1, &m_VertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, m_VertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, l_SizeInByte, l_pSrcData, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void VertexBuffer::use(char *l_pBase)
{
	if( m_CachedID == m_VertexBuffer ) return;
	
	glBindBuffer(GL_ARRAY_BUFFER, m_VertexBuffer);
	showOpenGLErrorCode("glBindBuffer");
	(*m_pBinder)(l_pBase);

	m_CachedID = m_VertexBuffer;
}
#pragma endregion

#pragma region IndexObject
//
// IndexObject
//
IndexBuffer::IndexBuffer(void *l_pSrc, GLenum l_Type, unsigned int l_NumIndicies)
	: m_IndexBuffer(0)
	, m_IndiciesType(l_Type)
	, m_Count(l_NumIndicies)
{
	glGenBuffers(1, &m_IndexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IndexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, getElementSize() * l_NumIndicies, l_pSrc, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

IndexBuffer::~IndexBuffer()
{
	glDeleteBuffers(1, &m_IndexBuffer);
}

void IndexBuffer::use()
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IndexBuffer);
    showOpenGLErrorCode("glBindBuffer(GL_ELEMENT_ARRAY_BUFFER)");
}

unsigned int IndexBuffer::getElementSize()
{
    switch( m_IndiciesType )
    {
        case GL_UNSIGNED_BYTE: return sizeof(char);
        case GL_UNSIGNED_SHORT: return sizeof(short);
    }
    return sizeof(int);
}
#pragma endregion

}