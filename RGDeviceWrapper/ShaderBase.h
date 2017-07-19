// ShaderBase.h
//
// 2015/09/25 Ian Wu/Real0000
//

#ifndef _SHADERBASE_H_
#define _SHADERBASE_H_

namespace R
{

class ProgramManager;

//
// program part
//
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

struct RegisterInfo
{
	ShaderRegType::Key m_Type;
	int m_RootIndex;
	int m_Slot;
	int m_Offset;// for constant
	int m_Size;// for constant
	bool m_bReserved;
};

struct ProgramTextureDesc
{
	ProgramTextureDesc();
	~ProgramTextureDesc();

	wxString m_Describe;
	RegisterInfo *m_pRegInfo;
};

struct ProgramParamDesc
{
	ProgramParamDesc();
	~ProgramParamDesc();

	wxString m_Describe;
	unsigned int m_Offset;
	ShaderParamType::Key m_Type;
	char *m_pDefault;
	RegisterInfo *m_pRegInfo;
};

struct ProgramBlockDesc
{
	ProgramBlockDesc();
	~ProgramBlockDesc();

	unsigned int m_BlockSize;
	std::map<std::string, ProgramParamDesc *> m_ParamDesc;
	RegisterInfo *m_pRegInfo;
};

class ShaderProgram
{
	friend class ProgramManager;
public:
	ShaderProgram();
	virtual ~ShaderProgram();

	void setup(boost::property_tree::ptree &a_Root);
	bool isCompute(){ return m_bCompute; }
	bool isIndexedDraw(){ return m_bIndexedDraw; }
	std::pair<int, int> shaderInUse(){ return m_ShaderInUse; }

	std::map<std::string, ProgramTextureDesc *>& getTextureDesc(){ return m_TextureDesc; }
	std::map<std::string, ProgramBlockDesc *>& getBlockDesc(){ return m_BlockDesc; }
	std::vector<ProgramBlockDesc *>& getConstBlockDesc(){ return m_ConstBlockDesc; }
	std::vector<ProgramBlockDesc *>& getStorageBlockDesc(){ return m_StorageDesc; }
	
protected:
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

// don't use clearCache method, ProgramManager will crash
class ProgramManager : public SearchPathSystem<ShaderProgram>
{
public:
	static void init(ProgramManagerComponent *a_pComponent);
	static ProgramManager& singleton();

	// call for program initial, DO NOT use this method directly
	void* getShader(wxString a_Filename, ShaderStages::Key a_Stage, std::pair<int, int> a_Module, std::map<std::string, std::string> &a_ParamDefine);

private:
	ProgramManager();
	virtual ~ProgramManager();

	std::shared_ptr<ShaderProgram> allocator();
	void loadFile(std::shared_ptr<ShaderProgram> a_pInst, wxString a_Path);
	void initDefaultProgram();

	static ProgramManagerComponent *m_pShaderComponent; 
};

}

#endif