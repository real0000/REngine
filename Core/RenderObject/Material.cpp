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
		l_pNewParam->m_Type = it->second->m_Type;
		l_pNewParam->m_Byte = GDEVICE()->getParamAlignmentSize(l_pNewParam->m_Type);
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
std::shared_ptr<Material> Material::create(ShaderProgram *a_pRefProgram)
{
	return std::shared_ptr<Material>(new Material(a_pRefProgram));
}

Material::Material(ShaderProgram *a_pRefProgram)
	: m_pRefProgram(a_pRefProgram)
	, m_bHide(false)
	, m_Stage(0)
{
	for( unsigned int i = ShaderRegType::ConstBuffer ; i <= ShaderRegType::UavBuffer ; ++i )
	{
		auto &l_Block = m_pRefProgram->getBlockDesc((ShaderRegType::Key)i);
		for( ProgramBlockDesc *l_pDesc : l_Block )
		{
			if( l_pDesc->m_bReserved )
			{
				m_ExternBlock.push_back(std::make_pair(nullptr, l_pDesc->m_pRegInfo->m_Slot));
				continue;
			}

			auto l_pNewBlock = MaterialBlock::create((ShaderRegType::Key)i, l_pDesc);
			m_OwnBlocks.push_back(std::make_pair(l_pNewBlock, l_pDesc->m_pRegInfo->m_Slot));
		}
	}

	m_Textures.resize(a_pRefProgram->getTextureDesc().size(), nullptr);
}

Material::~Material()
{
	m_OwnBlocks.clear();
	m_ExternBlock.clear();
	m_Textures.clear();
}

void Material::setTexture(std::string a_Name, std::shared_ptr<TextureUnit> a_pTexture)
{
	auto &l_TextureSlotMap = m_pRefProgram->getTextureDesc();
	auto it = l_TextureSlotMap.find(a_Name);
	if( l_TextureSlotMap.end() == it ) return;
	m_Textures[it->second->m_pRegInfo->m_Slot] = a_pTexture;
}

void Material::setBlock(unsigned int a_Idx, std::shared_ptr<MaterialBlock> a_pBlock)
{
	assert(a_Idx < m_ExternBlock.size());
	m_ExternBlock[a_Idx].first = a_pBlock;
}

void Material::bind(GraphicCommander *a_pBinder)
{
	for( unsigned int i=0 ; i<m_OwnBlocks.size() ; ++i ) m_OwnBlocks[i].first->bind(a_pBinder, m_OwnBlocks[i].second);
	for( unsigned int i=0 ; i<m_ExternBlock.size() ; ++i )
	{
		if( nullptr == m_ExternBlock[i].first ) continue;
		m_ExternBlock[i].first->bind(a_pBinder, m_ExternBlock[i].second);
	}
	for( unsigned int i=0 ; i<m_Textures.size() ; ++i )
	{
		if( nullptr != m_Textures[i] ) continue;

		TextureType l_Type = m_Textures[i]->getTextureType();
		a_pBinder->bindTexture(m_Textures[i]->getTextureID(), i, TextureType::TEXTYPE_DEPTH_STENCIL_VIEW != l_Type && TextureType::TEXTYPE_DEPTH_STENCIL_VIEW != l_Type);
	}
}
#pragma endregion

}