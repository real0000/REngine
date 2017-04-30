// DrawBuffer.h
//
// 2015/02/25 Ian Wu/Real0000
//

#ifndef _DRAW_BUFFER_H_
#define _DRAW_BUFFER_H_

namespace R
{

enum
{
	VTXSLOT_POSITION = 0,//in vec3 a_Position;
	VTXSLOT_TEXCOORD,//in vec4 a_UVs;
	VTXSLOT_NORMAL,//in vec3 a_Normal;
	VTXSLOT_TANGENT,//in vec3 a_Tangent;
	VTXSLOT_BINORMAL,//in vec3 a_Binormal;
	VTXSLOT_BONE,//in vec4 a_BoneID;
	VTXSLOT_WEIGHT,//in vec4 a_BoneWeight; 
	VTXSLOT_COLOR,//in vec4 a_Color;
};

template<typename T>
struct VertexBase
{
	static void bindVertexFormat(char *l_pBase){ assert(false && "all vertex format structure must implement this static function"); }
	static unsigned int getStride(){ return sizeof(T); }
};

struct VertexPosTex2NormalBone : public VertexBase<VertexPosTex2NormalBone>
{
	VertexPosTex2NormalBone();
	static void bindVertexFormat(char *l_pBase);

	glm::vec3 m_Position;
	glm::vec2 m_Texcoord[2];
	glm::vec3 m_Normal;
	glm::vec3 m_Tangent;
	glm::vec3 m_Binormal;
	unsigned char m_BoneID[4];
	glm::vec4 m_BoneWeight; 
};

struct VertexPositionColor : public VertexBase<VertexPositionColor>
{
	VertexPositionColor();
	static void bindVertexFormat(char *l_pBase);

	glm::vec3 m_Position;
	glm::vec4 m_Color;
};

class VertexBuffer
{
public:
	template<typename T>
	static VertexBuffer* create(T *l_pSrc, unsigned int l_NumVertex)
	{
		if( 0 == l_NumVertex || nullptr == l_pSrc ) return nullptr;

		VertexBuffer *l_pNewBuffer = new VertexBuffer(&T::bindVertexFormat);
		if( nullptr != l_pNewBuffer )
		{
			l_pNewBuffer->init(l_pSrc, l_NumVertex * l_pSrc->getStride());
			showOpenGLErrorCode(wxT("init vertex buffer"));
		}
		return l_pNewBuffer;
	}
	template<typename T>
	static VertexBuffer* create(){ return new VertexBuffer(&T::bindVertexFormat); }

	virtual ~VertexBuffer();

	void use(char *l_pBase = nullptr);

private:
	VertexBuffer(VertexBinder *l_pBinder);

	void init(void *l_pSrcData, unsigned int l_SizeInByte);

	VertexBinder *m_pBinder;
	GLuint m_VertexBuffer;
	static GLuint m_CachedID;
};

class IndexBuffer
{
public:
	template<typename T>
	static IndexBuffer* create(T *l_pSrc, unsigned int l_NumIndicies)
	{
		if( 0 == l_NumIndicies || nullptr == l_pSrc ) return nullptr;

		IndexBuffer *l_pRes = nullptr;
		if( sizeof(T) == sizeof(unsigned char) ) l_pRes = new IndexBuffer(l_pSrc, GL_UNSIGNED_BYTE, l_NumIndicies);
		else if( sizeof(T) == sizeof(unsigned short) ) l_pRes = new IndexBuffer(l_pSrc, GL_UNSIGNED_SHORT, l_NumIndicies);
		else l_pRes = new IndexBuffer(l_pSrc, GL_UNSIGNED_INT, l_NumIndicies);
		showOpenGLErrorCode(wxT("init index buffer"));

		return l_pRes;
	}
	virtual ~IndexBuffer();

	void use();
	GLuint getIndiciesType(){ return m_IndiciesType; }
    unsigned int getElementSize();
	GLuint getCount(){ return m_Count; }

private:
	IndexBuffer(void *l_pSrc, GLenum l_Type, unsigned int l_NumIndicies);

	GLuint m_IndexBuffer;
	GLuint m_IndiciesType;
	GLuint m_Count;
	static GLuint m_CachedID;
};

}

#endif