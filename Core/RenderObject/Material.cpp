// Material.cpp
//
// 2015/11/12 Ian Wu/Real0000
//
#include "CommonUtil.h"
#include "RGDeviceWrapper.h"
#include "Core.h"
#include "Material.h"
#include "Texture/Texture.h"

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

#pragma region Material
//
// Material
//
std::shared_ptr<Material> Material::create(std::shared_ptr<ShaderProgram> a_pRefProgram)
{
	return std::shared_ptr<Material>(new Material(a_pRefProgram));
}

Material::Material(std::shared_ptr<ShaderProgram> a_pRefProgram)
	: m_pRefProgram(a_pRefProgram)
	, m_bNeedRebatch(true), m_bNeedUavUpdate(true)
{
	auto &l_CbvBlock = m_pRefProgram->getBlockDesc(ShaderRegType::ConstBuffer);
	auto &l_ConstBlock = m_pRefProgram->getBlockDesc(ShaderRegType::Constant);
	auto &l_UavBlock = m_pRefProgram->getBlockDesc(ShaderRegType::UavBuffer);
	m_ConstBlocks.resize(l_ConstBlock.size() + l_CbvBlock.size(), nullptr);
	for( unsigned int i=0 ; i<l_CbvBlock.size() ; ++i )
	{
		ProgramBlockDesc *l_pDesc = l_CbvBlock[i];
		if( l_pDesc->m_bReserved ) continue;
		m_ConstBlocks[l_pDesc->m_pRegInfo->m_Slot] = MaterialBlock::create(ShaderRegType::ConstBuffer, l_pDesc);
	}
	for( unsigned int i=0 ; i<l_ConstBlock.size() ; ++i )
	{
		ProgramBlockDesc *l_pDesc = l_ConstBlock[i];
		if( l_pDesc->m_bReserved ) continue;
		m_ConstBlocks[l_pDesc->m_pRegInfo->m_Slot] = MaterialBlock::create(ShaderRegType::Constant, l_pDesc);
	}
	m_UavBlocks.resize(l_UavBlock.size(), nullptr);

	m_Textures.resize(a_pRefProgram->getTextureDesc().size(), nullptr);
}

Material::Material(Material *a_pSrc)
{
	m_ConstBlocks.resize(a_pSrc->m_ConstBlocks.size(), nullptr);
	std::copy(a_pSrc->m_ConstBlocks.begin(), a_pSrc->m_ConstBlocks.end(), m_ConstBlocks.begin());
	m_UavBlocks.resize(a_pSrc->m_UavBlocks.size(), nullptr);
	std::copy(a_pSrc->m_UavBlocks.begin(), a_pSrc->m_UavBlocks.end(), m_UavBlocks.begin());
	m_Textures.resize(a_pSrc->m_Textures.size());
	std::copy(a_pSrc->m_Textures.begin(), a_pSrc->m_Textures.end(), m_Textures.begin());
}

Material::~Material()
{
	m_pRefProgram = nullptr;
	m_ConstBlocks.clear();
	m_UavBlocks.clear();
	m_Textures.clear();
}

std::shared_ptr<Material> Material::clone()
{
	std::shared_ptr<Material> l_pNewMaterial = std::shared_ptr<Material>(new Material(this));
	return l_pNewMaterial;
}

std::shared_ptr<MaterialBlock> Material::createExternalBlock(ShaderRegType::Key a_Type, std::string a_Name, unsigned int a_NumSlot)
{
	std::vector<ProgramBlockDesc *> &l_DescList = m_pRefProgram->getBlockDesc(a_Type);
	auto it = std::find_if(l_DescList.begin(), l_DescList.end(), [=](ProgramBlockDesc *a_pDesc) -> bool{ return a_Name == a_pDesc->m_Name; });
	if( l_DescList.end() == it ) return nullptr;
	return MaterialBlock::create(a_Type, *it, a_NumSlot);
}

void Material::setTexture(std::string a_Name, std::shared_ptr<TextureUnit> a_pTexture)
{
	auto &l_TextureSlotMap = m_pRefProgram->getTextureDesc();
	auto it = l_TextureSlotMap.find(a_Name);
	if( l_TextureSlotMap.end() == it ) return;
	m_Textures[it->second->m_pRegInfo->m_Slot] = a_pTexture;
	m_bNeedRebatch = true;
}

void Material::setBlock(std::string a_Name, std::shared_ptr<MaterialBlock> a_pBlock)
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
			m_bNeedUavUpdate = true;
			break;

		default:return;
	}
}

bool Material::canBatch(std::shared_ptr<Material> a_Other)
{
	if( m_pRefProgram != a_Other->m_pRefProgram ) return false;

	if( m_Textures.size() != a_Other->m_Textures.size() ) return false;
	for( unsigned int i=0 ; i<m_Textures.size() ; ++i )
	{
		if( m_Textures[i] != a_Other->m_Textures[i] ) return false;
	}
	return true;
}

void Material::assignIndirectCommand(char *a_pOutput, std::shared_ptr<VertexBuffer> a_pVtxBuffer, std::shared_ptr<IndexBuffer> a_pIndexBuffer, IndirectDrawData a_DrawInfo)
{
	unsigned int l_Offset = 0;

	if( GDEVICE()->supportExtraIndirectCommand() )
	{
		m_pRefProgram->assignIndirectVertex(l_Offset, a_pOutput, a_pVtxBuffer.get());
		m_pRefProgram->assignIndirectIndex(l_Offset, a_pOutput, a_pIndexBuffer.get());
		/*for( unsigned int i=0 ; i<m_OwnBlocks.size() ; ++i )
		{
			if( m_OwnBlocks[i].first->getType() != ShaderRegType::Constant ) continue;
			memcpy(a_pOutput + l_Offset, m_OwnBlocks[i].first->getBlockPtr(0), m_OwnBlocks[i].first->getBlockSize());
			l_Offset += m_OwnBlocks[i].first->getBlockSize();
		}
	
		std::vector<int> l_IDList;
		for( ShaderRegType::Key l_Type : {ShaderRegType::ConstBuffer, ShaderRegType::UavBuffer} )
		{
			auto &l_BlockNameMap = m_pRefProgram->getBlockIndexMap(l_Type);
		
			l_IDList.resize(l_BlockNameMap.size());
			memset(l_IDList.data(), -1, sizeof(int) * l_IDList.size());
			for( unsigned int i=0 ; i<m_OwnBlocks.size() ; ++i )
			{
				if( l_Type != m_OwnBlocks[i].first->getType() ) continue;
				auto it = l_BlockNameMap.find(m_OwnBlocks[i].first->getName());
				l_IDList[it->second] = m_OwnBlocks[i].first->getID();
			}
			for( unsigned int i=0 ; i<m_ExternBlock.size() ; ++i )
			{
				if( l_Type != m_ExternBlock[i].first->getType() ) continue;
				auto it = l_BlockNameMap.find(m_ExternBlock[i].first->getName());
				l_IDList[it->second] = m_ExternBlock[i].first->getID();
			}
			m_pRefProgram->assignIndirectBlock(l_Offset, a_pOutput, l_Type, l_IDList);
		}*/
	}
	
	m_pRefProgram->assignIndirectDrawCommand(l_Offset, a_pOutput, a_DrawInfo);
}

void Material::bindTexture(GraphicCommander *a_pBinder)
{
	for( unsigned int i=0 ; i<m_Textures.size() ; ++i )
	{
		if( nullptr == m_Textures[i] ) continue;

		TextureType l_Type = m_Textures[i]->getTextureType();
		a_pBinder->bindTexture(m_Textures[i]->getTextureID(), i, TextureType::TEXTYPE_RENDER_TARGET_VIEW == l_Type || TextureType::TEXTYPE_DEPTH_STENCIL_VIEW == l_Type);
	}
}

void Material::bindBlocks(GraphicCommander *a_pBinder)
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

void Material::bindAll(GraphicCommander *a_pBinder)
{
	bindBlocks(a_pBinder);
	bindTexture(a_pBinder);
}
#pragma endregion

}