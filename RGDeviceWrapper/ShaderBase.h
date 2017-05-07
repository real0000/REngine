// ShaderBase.h
//
// 2015/09/25 Ian Wu/Real0000
//

#ifndef _SHADERBASE_H_
#define _SHADERBASE_H_

namespace R
{

//
// program part
//
struct ProgramTextureDesc
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
};

STRING_ENUM_CLASS(ShaderRegType,
	Srv2D,
	Srv2DArray,
	Srv3D,
	ConstBuffer,
	Constant,
	StorageBuffer,

	Sampler,)

STRING_ENUM_CLASS(ShaderStages,
	Compute,
	Vertex,
	TessCtrl,
	TessEval,
    Geometry,
	Fragment,
	
	NumStage)
class ShaderProgram
{
public:
	ShaderProgram();
	virtual ~ShaderProgram();

	void setup(boost::property_tree::ptree &a_Root);
	bool isCompute(){ return m_bCompute; }
	bool isIndexedDraw(){ return m_bIndexedDraw; }
	std::pair<int, int> shaderInUse(){ return m_ShaderInUse; }
	
protected:
	std::map<std::string, ProgramTextureDesc *>& getTextureDesc(){ return m_TextureDesc; }
	std::map<std::string, ProgramBlockDesc *>& getBlockDesc(){ return m_BlockDesc; }
	ProgramBlockDesc* newConstBlockDesc()
	{
		m_ConstBlockDesc.push_back(new ProgramBlockDesc());
		return m_ConstBlockDesc.back();
	}
	ProgramBlockDesc* newStorageBlockDesc()
	{
		m_StorageDesc.push_back(new ProgramBlockDesc());
		return m_StorageDesc.back();
	}
	virtual void init(boost::property_tree::ptree &a_Root) = 0;
	virtual unsigned int initParamOffset(unsigned int &a_Offset, ShaderParamType::Key a_Type) = 0;

private:
	void parseStructureDefineRoot(boost::property_tree::ptree &a_Root);
	void parseInitValue(ShaderParamType::Key a_Type, boost::property_tree::ptree &a_Src, char *a_pDst);

	std::map<std::string, ProgramTextureDesc *> m_TextureDesc;
	std::map<std::string, ProgramBlockDesc *> m_BlockDesc;
	std::vector<ProgramBlockDesc *> m_ConstBlockDesc, m_StorageDesc;
	bool m_bCompute;
	bool m_bIndexedDraw;
	std::pair<int, int> m_ShaderInUse;
};

//
// program manager part
//
class ProgramManagerComponent
{
public:
	virtual void* getShader(wxString a_Filename, ShaderStages::Key a_Stage, std::pair<int, int> a_Module, std::map<std::string, std::string> &a_ParamDefine) = 0;
	virtual ShaderProgram* newProgram() = 0;
};

class ProgramManager : public SearchPathSystem
{
public:
	static void init(ProgramManagerComponent *a_pComponent);
	static ProgramManager& singleton();

	ShaderProgram* getProgram(wxString a_Filename);
	ShaderProgram* getProgram(int a_ProgramID);
	int getProgramKey(wxString a_Filename);
	
	// call for program initial, DO NOT use this method directly
	void* getShader(wxString a_Filename, ShaderStages::Key a_Stage, std::pair<int, int> a_Module, std::map<std::string, std::string> &a_ParamDefine);

private:
	ProgramManager();
	virtual ~ProgramManager();

	void initDefaultProgram();

	std::vector<ShaderProgram *> m_Programs;
	std::map<wxString, int> m_ProgramMap;
	static ProgramManagerComponent *m_pShaderComponent; 
};

}

#endif