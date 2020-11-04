// MaterialAsset.cpp
//
// 2015/11/12 Ian Wu/Real0000
//
#include "CommonUtil.h"
#include "RGDeviceWrapper.h"
#include "Core.h"
#include "MaterialAsset.h"
#include "TextureAsset.h"

namespace R
{

#pragma region MaterialBlock
//
// MaterialBlock
//
std::shared_ptr<MaterialBlock> MaterialBlock::create(ShaderRegType::Key a_Type, ProgramBlockDesc *a_pDesc, unsigned int a_NumSlot)
{
	return std::shared_ptr<MaterialBlock>(new MaterialBlock(a_Type, a_pDesc, a_NumSlot));
}

MaterialBlock::MaterialBlock(ShaderRegType::Key a_Type, ProgramBlockDesc *a_pDesc, unsigned int a_NumSlot)
	: m_BindFunc(nullptr)
	, m_BlockName(a_pDesc->m_Name)
	, m_FirstParam("")
	, m_pBuffer(nullptr)
	, m_BlockSize(a_pDesc->m_BlockSize)
	, m_NumSlot(a_NumSlot)
	, m_ID(-1)
	, m_Type(a_Type)
{
	assert(0 != a_NumSlot);

	if( ShaderRegType::Constant == a_Type ) m_NumSlot = 1;
	
	m_Type = a_Type;
	switch( a_Type )
	{
		case ShaderRegType::Constant:
			m_BindFunc = std::bind(&MaterialBlock::bindConstant, this, std::placeholders::_1, std::placeholders::_2);
			m_pBuffer = new char[m_BlockSize];
			break;

		case ShaderRegType::ConstBuffer:
			m_BindFunc = std::bind(&MaterialBlock::bindConstBuffer, this, std::placeholders::_1, std::placeholders::_2);
			m_ID = GDEVICE()->requestConstBuffer(m_pBuffer, m_BlockSize * m_NumSlot);
			break;

		case ShaderRegType::UavBuffer:
			m_BindFunc = std::bind(&MaterialBlock::bindUavBuffer, this, std::placeholders::_1, std::placeholders::_2);
			m_ID = GDEVICE()->requestUavBuffer(m_pBuffer, m_BlockSize, m_NumSlot);
			break;
	}

	for( auto it = a_pDesc->m_ParamDesc.begin() ; it != a_pDesc->m_ParamDesc.end() ; ++it )
	{
		if( 0 == it->second->m_Offset ) m_FirstParam = it->first;

		MaterialParam *l_pNewParam = new MaterialParam();
		m_Params.insert(std::make_pair(it->first, l_pNewParam));

		l_pNewParam->m_Type = it->second->m_Type;
		l_pNewParam->m_Byte = GDEVICE()->getParamAlignmentSize(l_pNewParam->m_Type);
		l_pNewParam->m_pRefDesc = it->second;
		for( unsigned int i=0 ; i<m_NumSlot ; ++i )
		{
			char *l_pTarget = m_pBuffer + it->second->m_Offset + i * m_BlockSize;
			l_pNewParam->m_pRefVal.push_back(l_pTarget);
			memcpy(l_pTarget, it->second->m_pDefault, l_pNewParam->m_Byte);
		}
	}
}

MaterialBlock::~MaterialBlock()
{
	if( nullptr == m_pBuffer ) return;

	switch( m_Type )
	{
		case ShaderRegType::Constant:
			delete[] m_pBuffer;
			break;

		case ShaderRegType::ConstBuffer:
			GDEVICE()->freeConstBuffer(m_ID);
			break;

		case ShaderRegType::UavBuffer:
			GDEVICE()->freeUavBuffer(m_ID);
			break;

		default:break;
	}

	for( auto it = m_Params.begin() ; it != m_Params.end() ; ++it ) delete it->second;
	m_Params.clear();
}

void MaterialBlock::loadFile(boost::property_tree::ptree &a_Parent)
{
	for( auto it=a_Parent.begin() ; it!=a_Parent.end() ; ++it )
	{
		if( "<xmlattr>" == it->first ) continue;

		MaterialParam *l_pTarget = m_Params[it->second.get<std::string>("<xmlattr>.name")];

		for( auto it2=it->second.begin() ; it2!=it->second.end() ; ++it2 )
		{
			if( "<xmlattr>" == it2->first ) continue;
			parseShaderParamValue(l_pTarget->m_Type, it2->second.data(), l_pTarget->m_pRefVal[atoi(it->first.c_str())]);
		}
	}
}

void MaterialBlock::saveFile(boost::property_tree::ptree &a_Parent)
{
	for( auto it=m_Params.begin() ; it!=m_Params.end() ; ++it )
	{
		boost::property_tree::ptree l_Param;
		boost::property_tree::ptree l_ParamAttr;
		l_ParamAttr.add("name", it->first.c_str());
		l_ParamAttr.add("type", ShaderParamType::toString(it->second->m_Type).c_str());
		l_Param.add_child("<xmlattr>", l_ParamAttr);

		for( unsigned int i=0 ; i<m_NumSlot ; ++i )
		{
			char l_Buff[16];
			snprintf(l_Buff, 16, "%d", i);
			l_Param.put(l_Buff, convertParamValue(it->second->m_Type, it->second->m_pRefVal[i]));
		}
		boost::property_tree::ptree l_Layer;
		a_Parent.add_child("Param", l_Param);
	}
}

void MaterialBlock::extend(unsigned int a_Size)
{
	assert(ShaderRegType::UavBuffer == m_Type);
	m_NumSlot += a_Size;
	GDEVICE()->resizeUavBuffer(m_ID, m_pBuffer, m_NumSlot);
	for( auto it = m_Params.begin() ; it != m_Params.end() ; ++it )
	{
		it->second->m_pRefVal.resize(m_NumSlot);
		for( unsigned int i=0 ; i<m_NumSlot ; ++i )
		{
			it->second->m_pRefVal[i] = m_pBuffer + it->second->m_pRefDesc->m_Offset + i * m_BlockSize;
		}
	}
}

void MaterialBlock::sync(bool a_bToGpu)
{
	assert(ShaderRegType::UavBuffer == m_Type);
	GDEVICE()->syncUavBuffer(a_bToGpu, 1, m_ID);
}

void MaterialBlock::sync(bool a_bToGpu, unsigned int a_Offset, unsigned int a_SizeInByte)
{
	assert(ShaderRegType::UavBuffer == m_Type);
	std::vector<std::tuple<unsigned int, unsigned int, unsigned int>> l_BuffIDList(1, std::make_tuple(m_ID, a_Offset, a_SizeInByte));
	GDEVICE()->syncUavBuffer(a_bToGpu, l_BuffIDList);
}

char* MaterialBlock::getBlockPtr(unsigned int a_Slot)
{
	return m_pBuffer + a_Slot * m_BlockSize;
}

void MaterialBlock::bind(GraphicCommander *a_pBinder, int a_Stage)
{
	assert(nullptr != m_pBuffer);
	m_BindFunc(a_pBinder, a_Stage);
}

void MaterialBlock::bindConstant(GraphicCommander *a_pBinder, int a_Stage)
{
	a_pBinder->bindConstant(m_FirstParam, m_pBuffer, m_BlockSize / sizeof(unsigned int));
}

void MaterialBlock::bindConstBuffer(GraphicCommander *a_pBinder, int a_Stage)
{
	a_pBinder->bindConstBlock(m_ID, a_Stage);
}

void MaterialBlock::bindUavBuffer(GraphicCommander *a_pBinder, int a_Stage)
{
	a_pBinder->bindUavBlock(m_ID, a_Stage);
}
#pragma endregion

#pragma region MaterialAsset
//
// MaterialAsset
//
MaterialAsset::MaterialAsset()
	: m_Stage(0)
	, m_pRefProgram(nullptr)
	, m_InstanceUavIndex(-1)
	, m_CurrInstanceSize(0)
{
}

MaterialAsset::~MaterialAsset()
{
	m_pRefProgram = nullptr;
	m_ConstBlocks.clear();
	m_UavBlocks.clear();
	m_Textures.clear();
}

void MaterialAsset::loadFile(boost::property_tree::ptree &a_Src)
{
	boost::property_tree::ptree &l_Root = a_Src.get_child("root");
	
	init(ProgramManager::singleton().getData(l_Root.get<std::string>("<xmlattr>.refProgram")).second);
	m_Stage = l_Root.get<unsigned int>("<xmlattr>.stage");
	
	boost::property_tree::ptree l_ConstBlocks = l_Root.get_child("ConstBlocks");
	for( auto it=l_ConstBlocks.begin() ; it!=l_ConstBlocks.end() ; ++it )
	{
		if( "<xmlattr>" == it->first ) continue;

		boost::property_tree::ptree &l_Block = it->second;
		unsigned int l_BlockIdx = l_Block.get<unsigned int>("<xmlattr>.slot");

		m_ConstBlocks[l_BlockIdx]->loadFile(l_Block.get_child("Block"));
	}
	
	boost::property_tree::ptree l_Textures = l_Root.get_child("Textures");
	for( auto it=l_Textures.begin() ; it!=l_Textures.end() ; ++it )
	{
		if( "<xmlattr>" == it->first ) continue;

		unsigned int l_Slot = it->second.get<unsigned int>("<xmlattr>.slot");
		if( m_ReservedSRV.end() != m_ReservedSRV.find(l_Slot) ) continue;// modified manual

		m_Textures[l_Slot] = AssetManager::singleton().getAsset(it->second.get<std::string>("<xmlattr>.asset")).second;
	}
}

void MaterialAsset::saveFile(boost::property_tree::ptree &a_Dst)
{
	boost::property_tree::ptree l_Root;
	boost::property_tree::ptree l_RootAttr;
	l_RootAttr.add("refProgram", m_pRefProgram->getName());
	l_RootAttr.add("stage", m_Stage);
	l_Root.add_child("<xmlattr>", l_RootAttr);

	boost::property_tree::ptree l_ConstBlocks;
	for( unsigned int i=0 ; i<m_ConstBlocks.size() ; ++i )
	{
		if(  m_ReservedCBV.end() != m_ReservedCBV.find(i) ) continue;

		boost::property_tree::ptree l_Block;
		l_Block.add("<xmlattr>.slot", i);
		l_Block.add("<xmlattr>.name", m_ConstBlocks[i]->getName().c_str());

		m_ConstBlocks[i]->saveFile(l_Block);
		l_ConstBlocks.add_child("Block", l_Block);
	}
	l_Root.add_child("ConstBlocks", l_ConstBlocks);
	
	boost::property_tree::ptree l_Textures;
	for( unsigned int i=0 ; i<m_Textures.size() ; ++i )
	{
		if( m_ReservedSRV.end() != m_ReservedSRV.find(i) ) continue;

		boost::property_tree::ptree l_Texture;
		l_Texture.add("<xmlattr>.asset", m_Textures[i]->getKey().c_str());
		l_Texture.add("<xmlattr>.slot", i);
		l_Textures.add_child("Texture", l_Texture);
	}
	l_Root.add_child("Textures", l_Textures);

	a_Dst.add_child("root", l_Root);
}

std::shared_ptr<MaterialBlock> MaterialAsset::createExternalBlock(ShaderRegType::Key a_Type, std::string a_Name, unsigned int a_NumSlot)
{
	std::vector<ProgramBlockDesc *> &l_DescList = m_pRefProgram->getBlockDesc(a_Type);
	auto it = std::find_if(l_DescList.begin(), l_DescList.end(), [=](ProgramBlockDesc *a_pDesc) -> bool{ return a_Name == a_pDesc->m_Name; });
	if( l_DescList.end() == it ) return nullptr;
	return MaterialBlock::create(a_Type, *it, a_NumSlot);
}

void MaterialAsset::init(std::shared_ptr<ShaderProgram> a_pRefProgram)
{
	assert(nullptr == m_pRefProgram);
	m_pRefProgram = a_pRefProgram;

	auto &l_CbvBlock = m_pRefProgram->getBlockDesc(ShaderRegType::ConstBuffer);
	auto &l_ConstBlock = m_pRefProgram->getBlockDesc(ShaderRegType::Constant);
	auto &l_UavBlock = m_pRefProgram->getBlockDesc(ShaderRegType::UavBuffer);
	m_ConstBlocks.resize(l_ConstBlock.size() + l_CbvBlock.size(), nullptr);
	for( unsigned int i=0 ; i<l_CbvBlock.size() ; ++i )
	{
		ProgramBlockDesc *l_pDesc = l_CbvBlock[i];
		if( l_pDesc->m_bReserved )
		{
			m_ReservedCBV.insert((unsigned int)l_pDesc->m_pRegInfo->m_Slot);
			continue;
		}
		m_ConstBlocks[l_pDesc->m_pRegInfo->m_Slot] = MaterialBlock::create(ShaderRegType::ConstBuffer, l_pDesc);
	}
	for( unsigned int i=0 ; i<l_ConstBlock.size() ; ++i )
	{
		ProgramBlockDesc *l_pDesc = l_ConstBlock[i];
		if( l_pDesc->m_bReserved )
		{
			m_ReservedCBV.insert((unsigned int)l_pDesc->m_pRegInfo->m_Slot);
			continue;
		}
		m_ConstBlocks[l_pDesc->m_pRegInfo->m_Slot] = MaterialBlock::create(ShaderRegType::Constant, l_pDesc);
	}
	
	m_UavBlocks.resize(l_UavBlock.size(), nullptr);
	auto it = std::find_if(l_UavBlock.begin(), l_UavBlock.end(), [=](ProgramBlockDesc *a_pBlock) -> bool { return 0 == strcmp(a_pBlock->m_StructureName.c_str(), "InstanceInfo"); });
	if( l_UavBlock.end() != it )
	{
		unsigned int l_Index = it - l_UavBlock.begin();
		m_UavBlocks[l_Index] = MaterialBlock::create(ShaderRegType::UavBuffer, *it, BATCHDRAW_UNIT);
		for( int j=0 ; j<BATCHDRAW_UNIT ; ++j ) m_FreeInstanceSlot.push_back(j);
		m_InstanceUavIndex = (int)l_Index;
		m_CurrInstanceSize = BATCHDRAW_UNIT;
	}

	auto &l_TextureMap = a_pRefProgram->getTextureDesc();
	m_Textures.resize(l_TextureMap.size(), nullptr);
	for( auto it=l_TextureMap.begin() ; it!=l_TextureMap.end() ; ++it )
	{
		if( !it->second->m_bReserved ) continue;
		m_ReservedSRV.insert(it->second->m_pRegInfo->m_Slot);
	}
}

void MaterialAsset::setTexture(std::string a_Name, std::shared_ptr<Asset> a_pTexture)
{
	auto &l_TextureSlotMap = m_pRefProgram->getTextureDesc();
	auto it = l_TextureSlotMap.find(a_Name);
	if( l_TextureSlotMap.end() == it ) return;
	m_Textures[it->second->m_pRegInfo->m_Slot] = a_pTexture;
}

void MaterialAsset::setBlock(std::string a_Name, std::shared_ptr<MaterialBlock> a_pBlock)
{
	int l_TargetSlot = -1;
	RegisterInfo *l_pRegInfo = m_pRefProgram->getRegisterInfo(a_Name);
	if( nullptr == l_pRegInfo ) return;
	switch( l_pRegInfo->m_Type )
	{
		case ShaderRegType::ConstBuffer:
			m_ConstBlocks[l_pRegInfo->m_Slot] = a_pBlock;
			break;

		case ShaderRegType::UavBuffer:
			m_UavBlocks[l_pRegInfo->m_Slot] = a_pBlock;
			break;

		default:return;
	}
}

int MaterialAsset::requestInstanceSlot()
{
	if( -1 == m_InstanceUavIndex ) return -1;

	std::lock_guard<std::mutex> l_Lock(m_InstanceLock);
	int l_Res = -1;
	if( m_FreeInstanceSlot.empty() )
	{
		m_UavBlocks[m_InstanceUavIndex]->extend(BATCHDRAW_UNIT);
		for( unsigned int i=0 ; i<BATCHDRAW_UNIT ; ++i ) m_FreeInstanceSlot.push_back(i + m_CurrInstanceSize);
		m_CurrInstanceSize += BATCHDRAW_UNIT;
	}
	l_Res = m_FreeInstanceSlot.front();
	m_FreeInstanceSlot.pop_front();
	return l_Res;
}

void MaterialAsset::freeInstanceSlot(int a_Slot)
{
	if( -1 == a_Slot ) return;

	std::lock_guard<std::mutex> l_Lock(m_InstanceLock);
	assert(m_FreeInstanceSlot.end() == std::find(m_FreeInstanceSlot.begin(), m_FreeInstanceSlot.end(), a_Slot));
	m_FreeInstanceSlot.push_back(a_Slot);
}

void MaterialAsset::bindTexture(GraphicCommander *a_pBinder)
{
	for( unsigned int i=0 ; i<m_Textures.size() ; ++i )
	{
		if( nullptr == m_Textures[i] ) continue;

		TextureAsset *l_pTextureComp = m_Textures[i]->getComponent<TextureAsset>();
		TextureType l_Type = l_pTextureComp->getTextureType();
		a_pBinder->bindTexture(l_pTextureComp->getTextureID(), i, TextureType::TEXTYPE_RENDER_TARGET_VIEW == l_Type || TextureType::TEXTYPE_DEPTH_STENCIL_VIEW == l_Type);
		a_pBinder->bindSampler(l_pTextureComp->getSamplerID(), i);
	}
}

void MaterialAsset::bindBlocks(GraphicCommander *a_pBinder)
{
	for( unsigned int i=0 ; i<m_ConstBlocks.size() ; ++i )
	{
		if( nullptr == m_ConstBlocks[i] ) continue;
		m_ConstBlocks[i]->bind(a_pBinder, i);
	}
	for( unsigned int i=0 ; i<m_UavBlocks.size() ; ++i )
	{
		if( nullptr == m_UavBlocks[i] ) continue;
		m_UavBlocks[i]->bind(a_pBinder, i);
	}
}

void MaterialAsset::bindAll(GraphicCommander *a_pBinder)
{
	bindBlocks(a_pBinder);
	bindTexture(a_pBinder);
}
#pragma endregion

}