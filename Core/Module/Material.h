// Material.h
//
// 2015/11/12 Ian Wu/Real0000
//

#ifndef _MATERIAL_H_
#define _MATERIAL_H_

namespace R
{

class TextureUnit;
typedef wxSharedPtr<TextureUnit> TextureUnitPtr;

struct MaterialParam
{
	MaterialParam();
	~MaterialParam();

	ProgramParamDesc *m_pRefDesc;
	char *m_pRefVal;
};
struct MaterialBlock
{
	MaterialBlock();
	~MaterialBlock();

	template<typename T>
	void setParam(wxString l_Name, T l_Param)
	{
	
	}

	unsigned int m_HeapID;
	char *m_pRefBlockBuff;
	std::map<wxString, MaterialParam *> m_Param;
};
struct MaterialTexture
{
	MaterialTexture();
	~MaterialTexture();

	ProgramTextureDesc *m_pRefDesc;
	TextureUnitPtr m_Texture;
};
struct MaterialExternBlock
{
	MaterialExternBlock();
	~MaterialExternBlock();

	void addParam(wxString l_Name, ShaderParamType::Key l_Format);
	void init();

	ProgramBlockDesc *m_pBlockDesc;
	MaterialBlock *m_Block;
};
class Material
{
public:
	Material();
	virtual ~Material();

	void setProgram(ProgramBase *l_pRefProgram);
	ProgramBase* getProgram();
	void setExternalBlock(MaterialExternBlock *l_pExternalBlock);
	void setTexture(wxString l_Name, TextureUnitPtr l_Texture);
	template<typename T>
	void setParam(int l_BlockIdx, wxString l_Name, T l_Param)
	{
		
	}
	template<typename T>
	T getParam(int l_BlockIdx, wxString l_Name)
	{
	
	}

private:
	ProgramBase *m_pRefProgram;
	std::vector<MaterialBlock *> m_BlockList;
	std::map<wxString, MaterialTexture *> m_TextureList;
};

}

#endif