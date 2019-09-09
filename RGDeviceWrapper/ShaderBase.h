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
	SrvCube,
	SrvCubeArray,
	Srv3D,
	ConstBuffer,
	Constant,
	UavBuffer,

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
	RegisterInfo();

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
	bool m_bWrite;
	ShaderParamType::Key m_Type;// if write is true, default float4
	RegisterInfo *m_pRegInfo;
};

struct ProgramParamDesc
{
	ProgramParamDesc();
	~ProgramParamDesc();

	wxString m_Describe;
	unsigned int m_ArraySize;
	unsigned int m_Offset;
	ShaderParamType::Key m_Type;
	char *m_pDefault;
	RegisterInfo *m_pRegInfo;
};

struct ProgramBlockDesc
{
	ProgramBlockDesc();
	~ProgramBlockDesc();
	
	bool m_bReserved;
	bool m_bWrite;// for uav
	std::string m_StructureName;// for uav
	std::string m_Name;
	unsigned int m_BlockSize;
	std::map<std::string, ProgramParamDesc *> m_ParamDesc;
	RegisterInfo *m_pRegInfo;
};

struct IndirectDrawData
{
	unsigned int m_IndexCount;
	unsigned int m_StartIndex;
	unsigned int m_BaseVertex;
	unsigned int m_StartInstance;
	unsigned int m_InstanceCount;
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
	std::vector<ProgramBlockDesc *>& getBlockDesc(ShaderRegType::Key a_Type);// only ConstBuffer/Constant/UavBuffer valid

	const std::map<std::string, int>& getParamIndexMap(){ return m_ParamIndexMap; }
	const std::map<std::string, int>& getBlockIndexMap(ShaderRegType::Key a_Type);// const buffer/ uav only
	const std::vector<std::string>& getReservedBlockName(){ return m_ReservedBlockNameList; }

	void setExtraFlag(int a_ID, bool a_bVal){ m_ExtraFlags[a_ID] = a_bVal; }
	bool getExtraFlag(int a_ID){ return m_ExtraFlags[a_ID]; }

	// vertex -> index -> constant -> const buffer -> uav buffer -> draw command
	virtual unsigned int getIndirectCommandSize() = 0;
	virtual void assignIndirectVertex(unsigned int &a_Offset, char *a_pOutput, VertexBuffer *a_pVtx) = 0;
	virtual void assignIndirectIndex(unsigned int &a_Offset, char *a_pOutput, IndexBuffer *a_pIndex) = 0;
	// use memcpy instead
	//virtual void assignIndirectConstant(unsigned int &a_Size, char *a_pOutput, char *a_pSrc, unsigned int a_SizeInf)
	virtual void assignIndirectBlock(unsigned int &a_Offset, char *a_pOutput, ShaderRegType::Key a_Type, std::vector<int> &a_IDList) = 0;
	virtual void assignIndirectDrawComaand(unsigned int &a_Offset, char *a_pOutput, unsigned int a_IndexCount, unsigned int a_InstanceCount, unsigned int a_StartIndex, int a_BaseVertex, unsigned int a_StartInstance) = 0;
	virtual void assignIndirectDrawComaand(unsigned int &a_Offset, char *a_pOutput, IndirectDrawData a_DrawInfo);
	
protected:
	ProgramBlockDesc* newConstBlockDesc();
	VertexBuffer* getNullVertex(){ return m_pNullVtxBuffer; }
	virtual void init(boost::property_tree::ptree &a_Root) = 0;

private:
	void parseInitValue(ShaderParamType::Key a_Type, boost::property_tree::ptree &a_Src, char *a_pDst);
	
	std::map<std::string, ProgramTextureDesc *> m_TextureDesc;
	std::vector<ProgramBlockDesc *> m_BlockDesc[ShaderRegType::UavBuffer - ShaderRegType::ConstBuffer + 1];// const buffer, constant, uav
	bool m_bCompute;
	bool m_bIndexedDraw;
	std::pair<int, int> m_ShaderInUse;

	std::map<std::string, int> m_ParamIndexMap;// only parameter in not reserved block here, param name : block index
	std::map<std::string, int> m_ConstBufferIndexMap, m_UavBufferIndexMap;
	std::vector<std::string> m_ReservedBlockNameList;// only extern block here
	std::map<int, bool> m_ExtraFlags;

	static VertexBuffer *m_pNullVtxBuffer;
	static int m_NullVtxRefCount;
};

//
// program manager part
//
class ProgramManagerComponent
{
public:
	virtual ~ProgramManagerComponent(){}

	virtual void* getShader(ShaderProgram *a_pProgrom, wxString a_Filename, ShaderStages::Key a_Stage, std::pair<int, int> a_Module, std::map<std::string, std::string> &a_ParamDefine) = 0;
	virtual ShaderProgram* newProgram() = 0;
	virtual unsigned int calculateParamOffset(unsigned int &a_Offset, ShaderParamType::Key a_Type, unsigned int a_ArraySize = 1) = 0;
};

// don't use clearCache method, ProgramManager will crash
class ProgramManager : public SearchPathSystem<ShaderProgram>
{
public:
	static void init(ProgramManagerComponent *a_pComponent);
	static ProgramManager& singleton();

	// call for program initial, DO NOT use these method directly
	void initBlockDefine(wxString a_Filepath);
	void initDefaultProgram();
	void* getShader(ShaderProgram *a_pProgrom, wxString a_Filename, ShaderStages::Key a_Stage, std::pair<int, int> a_Module, std::map<std::string, std::string> &a_ParamDefine);

	unsigned int calculateParamOffset(unsigned int &a_Offset, ShaderParamType::Key a_Type, unsigned int a_ArraySize = 1);
	ProgramBlockDesc* createBlockFromDesc(boost::property_tree::ptree &a_Node);
	ProgramBlockDesc* createBlockFromDesc(std::string a_BlockName);// block defined in m_BlockDefineMap

private:
	ProgramManager();
	virtual ~ProgramManager();

	void parseInitValue(ShaderParamType::Key a_Type, boost::property_tree::ptree &a_Src, char *a_pDst);

	std::shared_ptr<ShaderProgram> allocator();
	void loadFile(std::shared_ptr<ShaderProgram> a_pInst, wxString a_Path);

	boost::property_tree::ptree m_BlockDefineFile;
	std::map<std::string, boost::property_tree::ptree*> m_BlockDefineMap;
	static ProgramManagerComponent *m_pShaderComponent; 
};

}

#endif