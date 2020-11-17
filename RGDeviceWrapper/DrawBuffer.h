// DrawBuffer.h
//
// 2017/05/08 Ian Wu/Real0000
//

#ifndef _DRAW_BUFFER_H_
#define _DRAW_BUFFER_H_

namespace R
{

class IndexBuffer
{
public:
	IndexBuffer();
	virtual ~IndexBuffer();

	void setIndicies(bool a_b32Bit, void *a_pSrc, unsigned int a_NumIndicies);
	void setName(wxString a_Name);
	void init();

	bool valid(){ return nullptr == m_pInitVal; }

	int getBufferID(){ return m_ID; }
	unsigned int getNumIndicies(){ return m_NumIndicies; }

private:
	struct InitInfo
	{
		void *m_pRefIndicies;
		wxString m_Name;
	} *m_pInitVal;

	int m_ID;
	bool m_b32Bit;
	unsigned int m_NumIndicies;
};

class VertexBuffer
{
public:
	VertexBuffer();
	virtual ~VertexBuffer();

	void setVertex(unsigned int a_Slot, void *a_pSrc);
	void setNumVertex(unsigned int a_NumVtx);
	void setName(wxString a_Name);
	void init();
	
	void updateVertexData(unsigned int a_Slot, void *a_pData, unsigned int a_Count);
	bool valid(){ return nullptr == m_pInitVal; }

	int getBufferID(unsigned int a_Slot){ return m_ID[a_Slot]; }

private:
	struct InitInfo
	{
		void *m_pRefVtx[VTXSLOT_COUNT];
		wxString m_Name;
		unsigned int m_NumVtx;
	} *m_pInitVal;

	int m_ID[VTXSLOT_COUNT];
};

// for simple indirect draw only (IndirectDrawData)
class IndirectDrawBuffer
{
public:
	IndirectDrawBuffer();
	virtual ~IndirectDrawBuffer();

	void assign(IndirectDrawData &a_Data);
	void reset();
	unsigned int getCurrCount(){ return m_CurrOffset; }

private:
	char *m_pBuff;
	int m_ID;
	unsigned int m_CurrOffset;
	unsigned int m_MaxSize;
};

}

#endif