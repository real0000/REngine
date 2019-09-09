// Material.h
//
// 2015/11/12 Ian Wu/Real0000
//

#ifndef _MATERIAL_H_
#define _MATERIAL_H_

namespace R
{

struct ProgramBlockDesc;
class GraphicCommander;
class ShaderProgram;
class TextureUnit;

struct MaterialParam
{
	std::vector<char *> m_pRefVal;
	unsigned int m_Byte;
	ShaderParamType::Key m_Type;
	ProgramParamDesc *m_pRefDesc;
};
class MaterialBlock
{
	friend class MaterialBlock;
public:
	static std::shared_ptr<MaterialBlock> create(ShaderRegType::Key a_Type, ProgramBlockDesc *a_pDesc, unsigned int a_NumSlot = 1);
	virtual ~MaterialBlock();

	// uav only
	void extend(unsigned int a_Size);
	void sync(bool a_bToGpu);

	template<typename T>
	void setParam(std::string a_Name, unsigned int a_Slot, T& a_Param)
	{
		assert(a_Slot < m_NumSlot);

		auto it = m_Params.find(a_Name);
		if( m_Params.end() == it ) return;
		assert(it->second->m_Byte == sizeof(T));
		memcpy(it->second->m_pRefVal[a_Slot], &a_Param, it->second->m_Byte);
	}
	template<typename T>
	T getParam(std::string a_Name, unsigned int a_Slot)
	{
		assert(a_Slot < m_NumSlot);

		T l_Res;

		auto it = m_Params.find(a_Name);
		if( m_Params.end() == it ) return l_Res;
		assert(it->second->m_Byte == sizeof(T));
		memcpy(&l_Res, it->second->m_pRefVal[a_Slot], it->second->m_Byte);
		return l_Res;
	}
	char* getBlockPtr(unsigned int a_Slot);
	unsigned int getBlockSize(){ return m_BlockSize; }
	void bind(GraphicCommander *a_pBinder, int a_Stage);

	std::string getName(){ return m_BlockName; }
	unsigned int getID(){ return m_ID; }
	ShaderRegType::Key getType(){ return m_Type; }

private:
	MaterialBlock(ShaderRegType::Key a_Type, ProgramBlockDesc *a_pDesc, unsigned int a_NumSlot);// ConstBuffer / Constant(a_NumSlot == 1) / UavBuffer

	void bindConstant(GraphicCommander *a_pBinder, int a_Stage);// a_Stage unused
	void bindConstBuffer(GraphicCommander *a_pBinder, int a_Stage);
	void bindUavBuffer(GraphicCommander *a_pBinder, int a_Stage);
	std::function<void(GraphicCommander *, int)> m_BindFunc;
	
	std::map<std::string, MaterialParam *> m_Params;
	std::string m_BlockName;
	std::string m_FirstParam;// constant block need this
	char *m_pBuffer;
	unsigned int m_BlockSize;
	unsigned int m_NumSlot;
	int m_ID;
	ShaderRegType::Key m_Type;
};

class Material
{
	friend class Material;
public:
	static std::shared_ptr<Material> create(std::shared_ptr<ShaderProgram> a_pRefProgram);
	virtual ~Material();

	std::shared_ptr<Material> clone();
	std::shared_ptr<MaterialBlock> createExternalBlock(ShaderRegType::Key a_Type, std::string a_Name, unsigned int a_NumSlot = 1);

	std::shared_ptr<ShaderProgram> getProgram(){ return m_pRefProgram; }
	void setTexture(std::string a_Name, std::shared_ptr<TextureUnit> a_pTexture);
	void setBlock(unsigned int a_Idx, std::shared_ptr<MaterialBlock> a_pBlock);
	void setBlock(std::string a_Name, std::shared_ptr<MaterialBlock> a_pBlock);

	template<typename T>
	void setParam(std::string a_Name, unsigned int a_Slot, T a_Param)
	{
		auto it = m_pRefProgram->getParamIndexMap().find(a_Name);
		if( m_pRefProgram->getParamIndexMap().end() == it ) return;
		m_OwnBlocks[it->second].first->setParam(a_Name, a_Slot, a_Param);
		m_bNeedUavUpdate = true;
	}
	template<typename T>
	T getParam(std::string a_Name, unsigned int a_Slot)
	{
		T l_Empty;

		auto it = m_pRefProgram->getParamIndexMap().find(a_Name);
		if( m_pRefProgram->getParamIndexMap().end() == it ) return l_Empty;
		return 	m_OwnBlocks[it->second].first->getParam(a_Name, a_Slot);
	}

	// only valid if supportExtraIndirectCommand is true
	bool canBatch(std::shared_ptr<Material> a_Other);
	// if supportExtraIndirectCommand is false, assign only draw command
	void assignIndirectCommand(char *a_pOutput, std::shared_ptr<VertexBuffer> a_pVtxBuffer, std::shared_ptr<IndexBuffer> a_pIndexBuffer, IndirectDrawData a_DrawInfo);

	void bindTexture(GraphicCommander *a_pBinder);
	void bindBlocks(GraphicCommander *a_pBinder);
	void bindAll(GraphicCommander *a_pBinder);

	bool needRebatch(){ return m_bNeedRebatch; }
	bool needUavUpdate(){ return m_bNeedUavUpdate; }
	void syncEnd(){ m_bNeedRebatch = m_bNeedUavUpdate = false; }

private:
	Material(std::shared_ptr<ShaderProgram> a_pRefProgram);

	std::shared_ptr<ShaderProgram> m_pRefProgram;
	std::vector< std::pair<std::shared_ptr<MaterialBlock>, int> > m_OwnBlocks, m_ExternBlock;// [block, stage] ... 
	std::vector< std::shared_ptr<TextureUnit> > m_Textures;

	bool m_bNeedRebatch, m_bNeedUavUpdate;// valid if supportExtraIndirectCommand
};

}

#endif