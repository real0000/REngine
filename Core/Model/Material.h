// Material.h
//
// 2015/11/12 Ian Wu/Real0000
//

#ifndef _MATERIAL_H_
#define _MATERIAL_H_

namespace R
{

class GraphicCommander;
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

	std::map<std::string, ProgramParamDesc *> m_ParamDesc;
	void *m_pRefRegInfo;
};*/
struct MaterialParam
{
	std::vector<char *> m_pRefVal;
	unsigned int m_Byte;
	ShaderParamType::Key m_Type;
};
class MaterialBlock
{
public:
	MaterialBlock();
	virtual ~MaterialBlock();

	void addParam(std::string a_Name, ShaderParamType::Key a_Type, char *a_pInitVal = nullptr);
	void init(ShaderRegType::Key a_Type, unsigned int a_NumSlot = 1);// ConstBuffer / Constant(a_NumSlot == 1) / StorageBuffer

	template<typename T>
	void setParam(std::string a_Name, unsigned int a_Slot, T a_Param)
	{
		assert(a_Slot < m_NumSlot);

		auto it = m_Params.find(a_Name);
		if( m_Params.end() == it ) return;
		assert(it->second->m_Byte == sizeof(T));
		memcpy(it->second->m_pRefVal[a_Slot], &a_Param, it->second->m_Byte);
	}
	template<typename T>
	T getParam(wxString a_Name, unsigned int a_Slot)
	{
		assert(a_Slot < m_NumSlot);

		T l_Res;

		auto it = m_Params.find(a_Name);
		if( m_Params.end() == it ) return l_Res;
		assert(it->second->m_Byte == sizeof(T));
		memcpy(&l_Res, it->second->m_pRefVal[a_Slot], it->second->m_Byte);
		return l_Res;
	}
	void bind(GraphicCommander *a_pBinder);

private:
	struct InitVal
	{
		struct InitParamInfo
		{
			std::string m_Name;
			ShaderParamType::Key m_Type;
			char m_Buffer[64];
		};
		std::vector<InitParamInfo> m_InitParams;
	} *m_pInitVal;
	
	std::map<std::string, MaterialParam *> m_Params;
	std::string m_FirstParam;// constant block need this
	char *m_pBuffer;
	unsigned int m_BlockSize;
	unsigned int m_NumSlot;
	ShaderRegType::Key m_Type;
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