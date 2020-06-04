// TextureAsset.cpp
//
// 2019/11/10 Ian Wu/Real0000
//

#include "CommonUtil.h"
#include "RGDeviceWrapper.h"
#include "RImporters.h"

#include "Core.h"
#include "TextureAsset.h"

namespace R
{

#pragma region TextureAsset
//
// TextureAsset
//
TextureAsset::TextureAsset()
	: m_TextureID(-1)
	, m_bRenderTarget(false)
	, m_bReady(false)
	, m_SamplerID(-1)
	, m_Filter(Filter::comparison_min_mag_mip_linear)
	, m_AddressMode{AddressMode::wrap, AddressMode::wrap, AddressMode::wrap}
	, m_MipLodBias(0)
	, m_MaxAnisotropy(16)
	, m_Func(CompareFunc::never)
	, m_MinLod(0)
	, m_MaxLod(FLT_MAX)
	, m_Border{0.0f, 0.0f, 0.0f, 0.0f}
	, m_bSamplerDirty(true)
{
}

TextureAsset::~TextureAsset()
{
	GraphicDevice *l_pDev = GDEVICE();
	if( -1 != m_SamplerID ) l_pDev->freeSampler(m_SamplerID);
	if( -1 != m_TextureID )
	{
		if( m_bRenderTarget ) l_pDev->freeRenderTarget(m_TextureID);
		else l_pDev->freeTexture(m_TextureID);
	}
}

void TextureAsset::validImportExt(std::vector<wxString> &a_Output)
{
	const wxString c_ExtList[] = {
		wxT("dds"),
		wxT("png"),
		wxT("bmp"),
		wxT("jpg"),
		wxT("jpeg"),
		wxT("tga"),
	};
	const unsigned int c_NumExt = sizeof(c_ExtList) / sizeof(wxString);
	for( unsigned int i=0 ; i<c_NumExt ; ++i ) a_Output.push_back(c_ExtList[i]);
}

wxString TextureAsset::validAssetKey()
{
	return wxT("Image");
}

void TextureAsset::initTexture(glm::ivec2 a_Size, PixelFormat::Key a_Format, unsigned int a_ArraySize, bool a_bCube, ...)
{
	assert(-1 == m_TextureID);

	m_TextureID = GDEVICE()->allocateTexture(a_Size, a_Format, a_ArraySize, a_bCube);
	m_bReady = true;
	
	va_list l_Arglist;
	va_start(l_Arglist, a_bCube);
	for( unsigned int i=0 ; i<a_ArraySize ; ++i )
	{
		void *l_pInitData = va_arg(l_Arglist, void *);
		if( nullptr == l_pInitData ) break;

		GDEVICE()->updateTexture(m_TextureID, 0, a_Size, glm::zero<glm::ivec2>(), i, l_pInitData);
	}
	va_end(l_Arglist);
	updateSampler();
}

void TextureAsset::initTexture(glm::ivec3 a_Size, PixelFormat::Key a_Format, void *a_pInitData)
{
	assert(-1 == m_TextureID);

	m_TextureID = GDEVICE()->allocateTexture(a_Size, a_Format);
	m_bReady = true;

	if( nullptr != a_pInitData ) GDEVICE()->updateTexture(m_TextureID, 0, a_Size, glm::zero<glm::ivec3>(), a_pInitData);
	updateSampler();
}

void TextureAsset::initRenderTarget(glm::ivec2 a_Size, PixelFormat::Key a_Format, unsigned int a_ArraySize, bool a_bCube)
{
	assert(-1 == m_TextureID);

	m_TextureID = GDEVICE()->createRenderTarget(a_Size, a_Format, a_ArraySize, a_bCube);
	m_bRenderTarget = true;
	m_bReady = true;
	updateSampler();
}

void TextureAsset::initRenderTarget(glm::ivec3 a_Size, PixelFormat::Key a_Format)
{
	assert(-1 == m_TextureID);

	m_TextureID = GDEVICE()->createRenderTarget(a_Size, a_Format);
	m_bRenderTarget = true;
	m_bReady = true;
	updateSampler();
}

void TextureAsset::importFile(wxString a_File)
{
	assert(-1 == m_TextureID);

	m_TextureID = EngineCore::singleton().getWhiteTexture()->getComponent<TextureAsset>()->getTextureID();
	std::thread l_AsyncLoader(&TextureAsset::loadThread, this, a_File);
	updateSampler();
}

void TextureAsset::loadFile(boost::property_tree::ptree &a_Src)
{
	/*m_Texture = TextureManager::singleton().getTexture(a_File);
	if( nullptr == m_Texture ) return;

	m_Dimention = m_Texture->getDimension();
	m_SamplerID = GDEVICE()->createSampler(m_Filter, m_AddressMode[0], m_AddressMode[1], m_AddressMode[2], m_MipLodBias, m_MaxAnisotropy, m_Func, m_MinLod, m_MaxLod
		, m_Border[0], m_Border[1], m_Border[2], m_Border[3]);*/
}

void TextureAsset::saveFile(boost::property_tree::ptree &a_Dst)
{
	glm::ivec3 l_Dim(getDimension());
	a_Dst.put("root.<xmlattr>.x", l_Dim.x);
	a_Dst.put("root.<xmlattr>.y", l_Dim.y);
	a_Dst.put("root.<xmlattr>.z", l_Dim.z);
	a_Dst.put("root.<xmlattr>.type", getTextureType());
	a_Dst.put("root.<xmlattr>.isRenderTarget", m_bRenderTarget);
	if( !m_bRenderTarget )
	{
		for( unsigned int i=0 ; i<m_RawFile.size() ; ++i )
		{
			char l_Buff[64];
			snprintf(l_Buff, 64, "root.RawData.Layer%d", i);
			a_Dst.put("RawData", m_RawFile[i]);
		}
	}
	
	a_Dst.put("root.Sampler.<xmlattr>.filter", Filter::toString(m_Filter).c_str());
	a_Dst.put("root.Sampler.<xmlattr>.u", AddressMode::toString(m_AddressMode[0]).c_str());
	a_Dst.put("root.Sampler.<xmlattr>.v", AddressMode::toString(m_AddressMode[1]).c_str());
	a_Dst.put("root.Sampler.<xmlattr>.w", AddressMode::toString(m_AddressMode[2]).c_str());
	a_Dst.put("root.Sampler.<xmlattr>.lodbias", m_MipLodBias);
	a_Dst.put("root.Sampler.<xmlattr>.anisotropy", m_MaxAnisotropy);
	a_Dst.put("root.Sampler.<xmlattr>.func", CompareFunc::toString(m_Func));
	a_Dst.put("root.Sampler.<xmlattr>.minlod", m_MinLod);
	a_Dst.put("root.Sampler.<xmlattr>.minlod", m_MaxLod);
	a_Dst.put("root.Sampler.<xmlattr>.r", m_Border[0]);
	a_Dst.put("root.Sampler.<xmlattr>.g", m_Border[1]);
	a_Dst.put("root.Sampler.<xmlattr>.b", m_Border[2]);
	a_Dst.put("root.Sampler.<xmlattr>.a", m_Border[3]);
}

int TextureAsset::getSampler()
{
	updateSampler();
	return m_SamplerID;
}

PixelFormat::Key TextureAsset::getTextureFormat()
{
	int l_ID = m_TextureID;
	if( m_bRenderTarget ) l_ID = GDEVICE()->getRenderTargetTexture(l_ID);
	return GDEVICE()->getTextureFormat(l_ID);
}

glm::ivec3 TextureAsset::getDimension()
{
	int l_ID = m_TextureID;
	if( m_bRenderTarget ) l_ID = GDEVICE()->getRenderTargetTexture(l_ID);
	return GDEVICE()->getTextureSize(l_ID);
}

TextureType TextureAsset::getTextureType()
{
	int l_ID = m_TextureID;
	if( m_bRenderTarget ) l_ID = GDEVICE()->getRenderTargetTexture(l_ID);
	return GDEVICE()->getTextureType(l_ID);
}

void TextureAsset::generateMipmap(unsigned int a_Level, std::shared_ptr<ShaderProgram> a_pProgram)
{
	int l_ID = m_TextureID;
	if( m_bRenderTarget ) l_ID = GDEVICE()->getRenderTargetTexture(l_ID);
	GDEVICE()->generateMipmap(l_ID, a_Level, a_pProgram);
}

void TextureAsset::loadThread(wxString a_Path)
{
	std::shared_ptr<ImageData> l_pImageData = ImageManager::singleton().getData(a_Path).second;
	m_SrcType = l_pImageData->getType();
	
	int l_TextureID = 0;
	glm::ivec3 l_Dim(l_pImageData->getSize());
	std::shared_ptr<ShaderProgram> l_pProgram = nullptr;
	switch( m_SrcType )
	{
		case TEXTYPE_SIMPLE_2D:{
			l_pProgram = ProgramManager::singleton().getData(DefaultPrograms::GenerateMipmap2D);
			l_TextureID = GDEVICE()->allocateTexture(glm::ivec2(l_Dim.x, l_Dim.y), l_pImageData->getFormat());
			GDEVICE()->updateTexture(l_TextureID, 0, glm::ivec2(l_Dim.x, l_Dim.y), glm::zero<glm::ivec2>(), 0, l_pImageData->getPixels(0));

			std::string l_RawData;
			binary2Base64(l_pImageData->getPixels(0), l_Dim.x * l_Dim.y * 4, l_RawData);
			m_RawFile.push_back(l_RawData);
			}break;

		case TEXTYPE_SIMPLE_2D_ARRAY:
		case TEXTYPE_SIMPLE_CUBE:{
			l_pProgram = ProgramManager::singleton().getData(DefaultPrograms::GenerateMipmap2D);
			l_TextureID = GDEVICE()->allocateTexture(glm::ivec2(l_Dim.x, l_Dim.y), l_pImageData->getFormat(), l_Dim.z, l_pImageData->getType() == TEXTYPE_SIMPLE_CUBE);
			for( int i=0 ; i<l_Dim.z ; ++i )
			{
				GDEVICE()->updateTexture(l_TextureID, 0, glm::ivec2(l_Dim.x, l_Dim.y), glm::zero<glm::ivec2>(), i, l_pImageData->getPixels(i));

				std::string l_RawData;
				l_RawData.clear();
				binary2Base64(l_pImageData->getPixels(i), l_Dim.x * l_Dim.y * 4, l_RawData);
				m_RawFile.push_back(l_RawData);
			}
			}break;

		case TEXTYPE_SIMPLE_3D:{
			l_pProgram = ProgramManager::singleton().getData(DefaultPrograms::GenerateMipmap3D);
			l_TextureID = GDEVICE()->allocateTexture(l_pImageData->getSize(), l_pImageData->getFormat());
			GDEVICE()->updateTexture(l_TextureID, 0, l_Dim, glm::zero<glm::ivec3>(), 0, l_pImageData->getPixels(0));
			
			std::string l_RawData;
			binary2Base64(l_pImageData->getPixels(0), l_Dim.x * l_Dim.y * l_Dim.z * 4, l_RawData);
			m_RawFile.push_back(l_RawData);
			}break;

		default:break;
	}
	m_TextureID = l_TextureID;
	GDEVICE()->generateMipmap(l_TextureID, 0, l_pProgram);
	m_bReady = true;
}

void TextureAsset::updateSampler()
{
	if( m_bSamplerDirty )
	{
		if( -1 != m_SamplerID ) GDEVICE()->freeSampler(m_SamplerID);
		m_SamplerID = GDEVICE()->createSampler(m_Filter, m_AddressMode[0], m_AddressMode[1], m_AddressMode[2], m_MipLodBias, m_MaxAnisotropy, m_Func, m_MinLod, m_MaxLod
			, m_Border[0], m_Border[1], m_Border[2], m_Border[3]);
		m_bSamplerDirty = false;
	}
}
#pragma endregion

}