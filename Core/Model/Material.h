// Material.h
//
// 2015/11/12 Ian Wu/Real0000
//

#ifndef _MATERIAL_H_
#define _MATERIAL_H_

namespace R
{

class ShaderProgram;
class TextureUnit;

/*struct ProgramTextureDesc
{
	ProgramTextureDesc();
	~ProgramTextureDesc();

	wxString m_Describe;
	void *m_pRefRegInfo;
};
struct ProgramParamDesc
{
	ProgramParamDesc();
	~ProgramParamDesc();

	wxString m_Describe;
	unsigned int m_Offset;
	ShaderParamType::Key m_Type;
	char *m_pDefault;
	void *m_pRefRegInfo;
};
struct ProgramBlockDesc
{
	ProgramBlockDesc();
	~ProgramBlockDesc();

	unsigned int m_BlockSize;
	std::map<std::string, ProgramParamDesc *> m_ParamDesc;
	void *m_pRefRegInfo;
};*/
struct MaterialParam
{
	char *m_pVal;
	ShaderParamType::Key m_Type;
};
class MaterialBlock
{
public:
	MaterialBlock();
	virtual ~MaterialBlock();

	void addParam(std::string a_Name, ShaderParamType::Key a_Type);
	void init(ShaderRegType::Key a_Type, unsigned int a_NumSlot = 1);// ConstBuffer / Constant / StorageBuffer

private:
	struct InitVal
	{
		
	} *m_pInitVal;
};

class Material
{
public:
	Material();
	virtual ~Material();

	void setProgram(ShaderProgram *a_pRefProgram);
	ShaderProgram* getProgram();
	void setTexture(wxString a_Name, std::shared_ptr<TextureUnit> a_Texture);
	template<typename T>
	void setParam(std::string a_Name, T a_Param)
	{
		
	}
	template<typename T>
	T getParam(int a_BlockIdx, wxString a_Name)
	{
	
	}

	void setHide(bool a_bHide){ m_bHide = a_bHide; }
	bool isHide(){ return m_bHide; }

private:
	ShaderProgram *m_pRefProgram;
	std::vector<MaterialBlock *> m_BlockList;

	bool m_bHide;
};

}

#endif