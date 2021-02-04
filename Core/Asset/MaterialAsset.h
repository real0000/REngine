// MaterialAsset.h
//
// 2015/11/12 Ian Wu/Real0000
//

#ifndef _MATERIAL_ASSET_H_
#define _MATERIAL_ASSET_H_

#include "AssetBase.h"

namespace R
{

struct ProgramBlockDesc;
class GraphicCommander;
class ShaderProgram;
class TextureAsset;

struct MaterialParam
{
	char *m_pRefValBase;
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

	// for const block
	virtual void loadFile(boost::property_tree::ptree &a_Parent);
	virtual void saveFile(boost::property_tree::ptree &a_Parent);

	// uav only
	void extend(unsigned int a_Size);
	void sync(bool a_bToGpu);
	void sync(bool a_bToGpu, unsigned int a_Offset, unsigned int a_SizeInByte);

	template<typename T>
	void setParam(std::string a_Name, unsigned int a_Slot, T& a_Param)
	{
		assert(a_Slot < m_NumSlot);

		auto it = m_Params.find(a_Name);
		if( m_Params.end() == it ) return;
		assert(it->second->m_Byte == sizeof(T));
		memcpy(it->second->m_pRefValBase + m_BlockSize * a_Slot, &a_Param, it->second->m_Byte);
	}
	template<typename T>
	T getParam(std::string a_Name, unsigned int a_Slot, T a_Defalt)
	{
		assert(a_Slot < m_NumSlot);

		T l_Res;

		auto it = m_Params.find(a_Name);
		if( m_Params.end() == it ) return a_Defalt;
		assert(it->second->m_Byte == sizeof(T));
		memcpy(&l_Res, it->second->m_pRefValBase + m_BlockSize * a_Slot, it->second->m_Byte);
		return l_Res;
	}
	char* getBlockPtr(unsigned int a_Slot);
	unsigned int getBlockSize(){ return m_BlockSize; }
	unsigned int getNumSlot(){ return m_NumSlot; }
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

class MaterialAsset : public AssetComponent
{
public:
	MaterialAsset();
	virtual ~MaterialAsset();
	
	// for asset factory
	static void validImportExt(std::vector<wxString> &a_Output){}
	static wxString validAssetKey(){ return wxT("Material"); }

	virtual wxString getAssetExt(){ return MaterialAsset::validAssetKey(); }
	virtual void importFile(wxString a_File){}
	virtual void loadFile(boost::property_tree::ptree &a_Src);
	virtual void saveFile(boost::property_tree::ptree &a_Dst);

	void init(std::shared_ptr<ShaderProgram> a_pRefProgram);
	std::shared_ptr<MaterialBlock> createExternalBlock(ShaderRegType::Key a_Type, std::string a_Name, unsigned int a_NumSlot = 1);

	std::shared_ptr<ShaderProgram> getProgram(){ return m_pRefProgram; }
	void setTexture(std::string a_Name, std::shared_ptr<Asset> a_pTexture);
	std::shared_ptr<Asset> getTexture(std::string a_Name);
	void setBlock(std::string a_Name, std::shared_ptr<MaterialBlock> a_pBlock);
	void setTopology(Topology::Key a_Topology){ m_Topology = a_Topology; }
	Topology::Key getTopology(){ return m_Topology; }

	template<typename T>
	void setParam(std::string a_Name, unsigned int a_Slot, T a_Param)
	{
		RegisterInfo *l_pRegInfo = m_pRefProgram->getRegisterInfo(a_Name);
		if( nullptr == l_pRegInfo ) return;

		std::shared_ptr<MaterialBlock> l_pTargetBlock = nullptr;
		switch( l_pRegInfo->m_Type )
		{
			case ShaderRegType::ConstBuffer:
			case ShaderRegType::Constant:
				l_pTargetBlock = m_ConstBlocks[l_pRegInfo->m_Slot];
				break;

			case ShaderRegType::UavBuffer:
				l_pTargetBlock = m_UavBlocks[l_pRegInfo->m_Slot];
				break;

			default:break;
		}
		if( nullptr == l_pTargetBlock ) return;

		l_pTargetBlock->setParam(a_Name, a_Slot, a_Param);
	}
	template<typename T>
	T getParam(std::string a_Name, unsigned int a_Slot)
	{
		T l_Empty;

		RegisterInfo *l_pRegInfo = m_pRefProgram->getRegisterInfo(a_Name);
		if( nullptr == l_pRegInfo ) return l_Empty;

		std::shared_ptr<MaterialBlock> l_pTargetBlock = nullptr;
		switch( l_pRegInfo->m_Type )
		{
			case ShaderRegType::ConstBuffer:
			case ShaderRegType::Constant:
				l_pTargetBlock = m_ConstBlocks[l_pRegInfo->m_Slot];
				break;

			case ShaderRegType::UavBuffer:
				l_pTargetBlock = m_UavBlocks[l_pRegInfo->m_Slot];
				break;

			default:break;
		}
		if( nullptr == l_pTargetBlock ) return l_Empty;
		return l_pTargetBlock->getParam(a_Name, a_Slot);
	}
	
	int requestInstanceSlot();
	void freeInstanceSlot(int a_Slot);

	void bindTexture(GraphicCommander *a_pBinder);
	void bindTexture(GraphicCommander *a_pBinder, std::string a_Name, std::shared_ptr<Asset> a_pTexture);
	void bindBlocks(GraphicCommander *a_pBinder);
	void bindBlock(GraphicCommander *a_pBinder, std::string a_Name, std::shared_ptr<MaterialBlock> a_pExtBlock);
	void bindAll(GraphicCommander *a_pBinder);
	
private:
	std::shared_ptr<ShaderProgram> m_pRefProgram;
	std::vector< std::shared_ptr<MaterialBlock> > m_ConstBlocks, m_UavBlocks;
	std::set<unsigned int> m_ReservedCBV, m_ReservedSRV;
	std::vector<std::shared_ptr<Asset>> m_Textures, m_RWTexture;
	Topology::Key m_Topology;

	// instance part
	std::deque<int> m_FreeInstanceSlot;
	std::mutex m_InstanceLock;
	int m_InstanceUavIndex;
	unsigned int m_CurrInstanceSize;
};

}

#endif