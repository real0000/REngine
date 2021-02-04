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
	, m_Filter(Filter::min_mag_mip_linear)
	, m_AddressMode{AddressMode::wrap, AddressMode::wrap, AddressMode::wrap}
	, m_MipLodBias(0.0f)
	, m_MaxAnisotropy(16)
	, m_Func(CompareFunc::never)
	, m_MinLod(0.0f)
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
		wxT("exr")
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
	m_bReady = true;
}

void TextureAsset::initTexture(glm::ivec3 a_Size, PixelFormat::Key a_Format, void *a_pInitData)
{
	assert(-1 == m_TextureID);

	m_TextureID = GDEVICE()->allocateTexture(a_Size, a_Format);

	if( nullptr != a_pInitData ) GDEVICE()->updateTexture(m_TextureID, 0, a_Size, glm::zero<glm::ivec3>(), a_pInitData);
	updateSampler();
	m_bReady = true;
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

void TextureAsset::updateTexture(unsigned int a_MipmapLevel, glm::ivec2 a_Size, glm::ivec2 a_Offset, unsigned int a_Idx, void *a_pSrcData)
{
	assert(-1 == m_TextureID);
	GDEVICE()->updateTexture(m_TextureID, a_MipmapLevel, a_Size, a_Offset, a_Idx, a_pSrcData);
}

void TextureAsset::updateTexture(unsigned int a_MipmapLevel, glm::ivec3 a_Size, glm::ivec3 a_Offset, void *a_pSrcData)
{
	assert(-1 == m_TextureID);
	GDEVICE()->updateTexture(m_TextureID, a_MipmapLevel, a_Size, a_Offset, a_pSrcData);
}

void TextureAsset::importFile(wxString a_File)
{
	assert(-1 == m_TextureID);

	m_TextureID = AssetManager::singleton().getAsset(WHITE_TEXTURE_ASSET_NAME)->getComponent<TextureAsset>()->getTextureID();
	EngineCore::singleton().addJob([=]() -> void{ importThread(a_File);});
	updateSampler();
}

void TextureAsset::loadFile(boost::property_tree::ptree &a_Src)
{
	assert(-1 == m_TextureID);
	
	m_TextureID = AssetManager::singleton().getAsset(WHITE_TEXTURE_ASSET_NAME)->getComponent<TextureAsset>()->getTextureID();

	boost::property_tree::ptree &l_Root = a_Src.get_child("root");
	boost::property_tree::ptree &l_RootAttr = l_Root.get_child("<xmlattr>");

	glm::ivec3 l_Dim(l_RootAttr.get<int>("x"), l_RootAttr.get<int>("y"), l_RootAttr.get<int>("z"));
	TextureType l_Type = (TextureType)l_RootAttr.get<int>("type");
	PixelFormat::Key l_Fmt = PixelFormat::fromString(l_RootAttr.get("format", "rgba8_unorm"));
	m_bRenderTarget = l_RootAttr.get<bool>("isRenderTarget");

	boost::property_tree::ptree l_SamplerAttr = l_Root.get_child("Sampler.<xmlattr>");
	m_Filter = Filter::fromString(l_SamplerAttr.get("filter", "comparison_min_mag_mip_linear"));
	m_AddressMode[0] = AddressMode::fromString(l_SamplerAttr.get("u", "wrap"));
	m_AddressMode[1] = AddressMode::fromString(l_SamplerAttr.get("v", "wrap"));
	m_AddressMode[2] = AddressMode::fromString(l_SamplerAttr.get("w", "wrap"));
	m_MipLodBias = l_SamplerAttr.get("lodbias", 0);
	m_MaxAnisotropy = l_SamplerAttr.get("anisotropy", 16);
	m_Func = CompareFunc::fromString(l_SamplerAttr.get("func", "never"));
	m_MinLod = l_SamplerAttr.get("minlod", 0.0f);
	m_MaxLod = l_SamplerAttr.get("maxlod", FLT_MAX);
	m_Border[0] = l_SamplerAttr.get("r", 0.0f);
	m_Border[1] = l_SamplerAttr.get("g", 0.0f);
	m_Border[2] = l_SamplerAttr.get("b", 0.0f);
	m_Border[3] = l_SamplerAttr.get("a", 0.0f);

	if( m_bRenderTarget )
	{
		switch( l_Type )
		{
			case TEXTYPE_SIMPLE_2D:{
				m_TextureID = GDEVICE()->createRenderTarget(glm::ivec2(l_Dim.x, l_Dim.y), l_Fmt);
				}break;

			case TEXTYPE_SIMPLE_2D_ARRAY:
			case TEXTYPE_SIMPLE_CUBE:{
				m_TextureID = GDEVICE()->createRenderTarget(glm::ivec2(l_Dim.x, l_Dim.y), l_Fmt, l_Dim.z, l_Type == TEXTYPE_SIMPLE_CUBE);
				}break;

			case TEXTYPE_SIMPLE_3D:{
				m_TextureID = GDEVICE()->createRenderTarget(l_Dim, l_Fmt);
				}break;

			default:break;
		}
		m_bReady = true;
	}
	else
	{
		boost::property_tree::ptree l_Layers = l_Root.get_child("Layers");
		for( auto it=l_Layers.begin() ; it!=l_Layers.end() ; ++it )
		{
			m_RawFile.push_back(it->second.get_child("RawData").data());
		}

		EngineCore::singleton().addJob([=]() -> void{ loadThread(l_Dim, l_Type, l_Fmt); });
	}
	
	updateSampler();
}

void TextureAsset::saveFile(boost::property_tree::ptree &a_Dst)
{
	while( !m_bReady ) std::this_thread::yield();

	glm::ivec3 l_Dim(getDimension());

	boost::property_tree::ptree l_Root;

	boost::property_tree::ptree l_RootAttr;
	l_RootAttr.add("x", l_Dim.x);
	l_RootAttr.add("y", l_Dim.y);
	l_RootAttr.add("z", l_Dim.z);
	l_RootAttr.add("type", getTextureType());
	l_RootAttr.add("format", PixelFormat::toString(getTextureFormat()));
	l_RootAttr.add("isRenderTarget", m_bRenderTarget);
	l_Root.add_child("<xmlattr>", l_RootAttr);

	if( !m_bRenderTarget )
	{
		boost::property_tree::ptree l_Layers;
		for( unsigned int i=0 ; i<m_RawFile.size() ; ++i )
		{
			boost::property_tree::ptree l_Layer;
			l_Layer.put("RawData", m_RawFile[i]);
		
			char l_Buff[64];	
			snprintf(l_Buff, 64, "Layer%d", i);
			l_Layers.add_child(l_Buff, l_Layer);
		}
		l_Root.add_child("Layers", l_Layers);
	}
	
	boost::property_tree::ptree l_Sampler;
	boost::property_tree::ptree l_SamplerAttr;
	l_SamplerAttr.add("filter", Filter::toString(m_Filter).c_str());
	l_SamplerAttr.add("u", AddressMode::toString(m_AddressMode[0]).c_str());
	l_SamplerAttr.add("v", AddressMode::toString(m_AddressMode[1]).c_str());
	l_SamplerAttr.add("w", AddressMode::toString(m_AddressMode[2]).c_str());
	l_SamplerAttr.add("lodbias", m_MipLodBias);
	l_SamplerAttr.add("anisotropy", m_MaxAnisotropy);
	l_SamplerAttr.add("func", CompareFunc::toString(m_Func));
	l_SamplerAttr.add("minlod", m_MinLod);
	l_SamplerAttr.add("maxlod", m_MaxLod);
	l_SamplerAttr.add("r", m_Border[0]);
	l_SamplerAttr.add("g", m_Border[1]);
	l_SamplerAttr.add("b", m_Border[2]);
	l_SamplerAttr.add("a", m_Border[3]);
	l_Sampler.add_child("<xmlattr>", l_SamplerAttr);
	l_Root.add_child("Sampler", l_Sampler);

	a_Dst.add_child("root", l_Root);
}

int TextureAsset::getSamplerID()
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

void TextureAsset::importThread(wxString a_Path)
{
	std::shared_ptr<ImageData> l_pImageData = ImageManager::singleton().getData(a_Path).second;
	
	int l_TextureID = 0;
	glm::ivec3 l_Dim(l_pImageData->getSize());
	std::shared_ptr<ShaderProgram> l_pProgram = nullptr;
	switch( l_pImageData->getType() )
	{
		case TEXTYPE_SIMPLE_2D:{
			l_pProgram = ProgramManager::singleton().getData(DefaultPrograms::GenerateMipmap2D);
			l_TextureID = GDEVICE()->allocateTexture(glm::ivec2(l_Dim.x, l_Dim.y), l_pImageData->getFormat());
			GDEVICE()->updateTexture(l_TextureID, 0, glm::ivec2(l_Dim.x, l_Dim.y), glm::zero<glm::ivec2>(), 0, l_pImageData->getPixels(0));

			std::string l_RawData;
			binary2Base64(l_pImageData->getPixels(0), l_Dim.x * l_Dim.y * getPixelSize(l_pImageData->getFormat()) / 8, l_RawData);
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
				binary2Base64(l_pImageData->getPixels(i), l_Dim.x * l_Dim.y * getPixelSize(l_pImageData->getFormat()) / 8, l_RawData);
				m_RawFile.push_back(l_RawData);
			}
			}break;

		case TEXTYPE_SIMPLE_3D:{
			l_pProgram = ProgramManager::singleton().getData(DefaultPrograms::GenerateMipmap3D);
			l_TextureID = GDEVICE()->allocateTexture(l_pImageData->getSize(), l_pImageData->getFormat());
			GDEVICE()->updateTexture(l_TextureID, 0, l_Dim, glm::zero<glm::ivec3>(), 0, l_pImageData->getPixels(0));
			
			std::string l_RawData;
			binary2Base64(l_pImageData->getPixels(0), l_Dim.x * l_Dim.y * l_Dim.z * getPixelSize(l_pImageData->getFormat()) / 8, l_RawData);
			m_RawFile.push_back(l_RawData);
			}break;

		default:break;
	}
	GDEVICE()->generateMipmap(l_TextureID, 0, l_pProgram);
	m_TextureID = l_TextureID;
	m_bReady = true;
}

void TextureAsset::loadThread(glm::ivec3 a_Dim, TextureType a_Type, PixelFormat::Key a_Fmt)
{
	int l_TextureID = 0;
	std::shared_ptr<ShaderProgram> l_pProgram = nullptr;
	switch( a_Type )
	{
		case TEXTYPE_SIMPLE_2D:{
			std::vector<char> l_Buff;
			base642Binary(m_RawFile[0], l_Buff);

			l_pProgram = ProgramManager::singleton().getData(DefaultPrograms::GenerateMipmap2D);
			l_TextureID = GDEVICE()->allocateTexture(glm::ivec2(a_Dim.x, a_Dim.y), a_Fmt);
			GDEVICE()->updateTexture(l_TextureID, 0, glm::ivec2(a_Dim.x, a_Dim.y), glm::zero<glm::ivec2>(), 0, l_Buff.data());
			}break;

		case TEXTYPE_SIMPLE_2D_ARRAY:
		case TEXTYPE_SIMPLE_CUBE:{
			l_pProgram = ProgramManager::singleton().getData(DefaultPrograms::GenerateMipmap2D);
			l_TextureID = GDEVICE()->allocateTexture(glm::ivec2(a_Dim.x, a_Dim.y), a_Fmt, a_Dim.z, a_Type == TEXTYPE_SIMPLE_CUBE);
			for( int i=0 ; i<a_Dim.z ; ++i )
			{
				std::vector<char> l_Buff;
				base642Binary(m_RawFile[i], l_Buff);
				GDEVICE()->updateTexture(l_TextureID, 0, glm::ivec2(a_Dim.x, a_Dim.y), glm::zero<glm::ivec2>(), i, l_Buff.data());
			}
			}break;

		case TEXTYPE_SIMPLE_3D:{
			std::vector<char> l_Buff;
			base642Binary(m_RawFile[0], l_Buff);

			l_pProgram = ProgramManager::singleton().getData(DefaultPrograms::GenerateMipmap3D);
			l_TextureID = GDEVICE()->allocateTexture(a_Dim, a_Fmt);
			GDEVICE()->updateTexture(l_TextureID, 0, a_Dim, glm::zero<glm::ivec3>(), 0, l_Buff.data());
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