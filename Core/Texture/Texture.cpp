// Texture.cpp
//
// 2014/06/21 Ian Wu/Real0000
//
#include "CommonUtil.h"
#include "RImporters.h"
#include "RGDeviceWrapper.h"
#include "Texture.h"

namespace R
{

#pragma region TextureUnit
//
// TextureUnit
//
TextureUnit::TextureUnit()
	: m_TextureID(0)
	, m_Name(wxT(""))
	, m_bReady(false)
{
}

TextureUnit::~TextureUnit()
{
}

void TextureUnit::release()
{
	TextureManager::singleton().recycle(m_Name);
}

PixelFormat::Key TextureUnit::getTextureFormat()
{
	return GDEVICE()->getTextureFormat(m_TextureID);
}

glm::ivec3 TextureUnit::getDimension()
{
	return GDEVICE()->getTextureSize(m_TextureID);
}

TextureType TextureUnit::getTextureType()
{
	return GDEVICE()->getTextureType(m_TextureID);
}
#pragma endregion

#pragma region TextureManager
//
// TextureManager
//
TextureManager& TextureManager::singleton()
{
	static TextureManager s_Inst;
	return s_Inst;
}

TextureManager::TextureManager()
{
}

TextureManager::~TextureManager()
{
	clearCache();
}

std::shared_ptr<TextureUnit> TextureManager::createTexture(wxString a_Filename, bool a_bAsync, std::function<void(std::shared_ptr<TextureUnit>)> a_Callback)
{
	std::shared_ptr<TextureUnit> l_pNewTexture = nullptr;
	{
		std::lock_guard<std::mutex> l_FileLoadGuard(m_FileLock);
		auto it = m_Textures.find(a_Filename);
		if( m_Textures.end() != it ) return it->second;
		
		l_pNewTexture = std::shared_ptr<TextureUnit>(new TextureUnit());
		m_Textures.insert(std::make_pair(a_Filename, l_pNewTexture));
	}
	if( a_bAsync ) std::thread l_AsyncLoader(&TextureManager::asyncFunc, this, l_pNewTexture, a_Callback);
	else
	{
		loadTextureFile(l_pNewTexture);
		l_pNewTexture->setReady();
	}
	
	return l_pNewTexture;
}

std::shared_ptr<TextureUnit> TextureManager::createTexture(wxString a_Name, unsigned int a_Width, PixelFormat::Key a_Format, void *a_pInitData)
{
	std::shared_ptr<TextureUnit> l_pNewTexture = std::shared_ptr<TextureUnit>(new TextureUnit());
	unsigned int l_TextureID = GDEVICE()->allocateTexture(a_Width, a_Format);

	l_pNewTexture->setName(a_Name);
	l_pNewTexture->setTextureID(l_TextureID);
	l_pNewTexture->setReady();
	if( nullptr != a_pInitData ) GDEVICE()->updateTexture(l_TextureID, 0, a_Width, 0, a_pInitData);

	return packNewTextureUnit(a_Name, l_pNewTexture);
}

std::shared_ptr<TextureUnit> TextureManager::createTexture(wxString a_Name, glm::ivec2 a_Size, PixelFormat::Key a_Format, unsigned int a_ArraySize, bool a_bCube, ...)
{
	std::shared_ptr<TextureUnit> l_pNewTexture = std::shared_ptr<TextureUnit>(new TextureUnit());
	unsigned int l_TextureID = GDEVICE()->allocateTexture(a_Size, a_Format, a_ArraySize, a_bCube);
	
	l_pNewTexture->setName(a_Name);
	l_pNewTexture->setTextureID(l_TextureID);
	l_pNewTexture->setReady();
	
	va_list l_Arglist;
	va_start(l_Arglist, a_bCube);
	for( unsigned int i=0 ; i<a_ArraySize ; ++i )
	{
		void *l_pInitData = va_arg(l_Arglist, void *);
		if( nullptr == l_pInitData ) break;

		GDEVICE()->updateTexture(l_TextureID, 0, a_Size, glm::zero<glm::ivec2>(), i, l_pInitData);
	}
	va_end(l_Arglist);
	return packNewTextureUnit(a_Name, l_pNewTexture);
}

std::shared_ptr<TextureUnit> TextureManager::createTexture(wxString a_Name, glm::ivec3 a_Size, PixelFormat::Key a_Format, void *a_pInitData)
{
	std::shared_ptr<TextureUnit> l_pNewTexture = std::shared_ptr<TextureUnit>(new TextureUnit());
	unsigned int l_TextureID = GDEVICE()->allocateTexture(a_Size, a_Format);

	l_pNewTexture->setName(a_Name);
	l_pNewTexture->setTextureID(l_TextureID);
	l_pNewTexture->setReady();
	if( nullptr != a_pInitData ) GDEVICE()->updateTexture(l_TextureID, 0, a_Size, glm::zero<glm::ivec3>(), a_pInitData);

	return packNewTextureUnit(a_Name, l_pNewTexture);
}

void TextureManager::recycle(wxString a_Name)
{
	auto it = m_Textures.find(a_Name);
	if( m_Textures.end() == it ) return ;
	if( 2 < it->second.use_count() ) return ;// 2 -> map 1, who called release one
	
	m_Textures.erase(it);
}

std::shared_ptr<TextureUnit> TextureManager::getTexture(wxString a_Name)
{
	auto it = m_Textures.find(a_Name);
	if( m_Textures.end() == it ) return nullptr;
	return it->second;
}

void TextureManager::clearCache()
{
	m_Textures.clear();
}

std::shared_ptr<TextureUnit> TextureManager::packNewTextureUnit(wxString a_PresetName, std::shared_ptr<TextureUnit> a_pNewUnit)
{
	if( m_Textures.end() != m_Textures.find(a_PresetName) )
	{
		unsigned int l_Serial = 0;
		wxString l_NewName(wxString::Format(wxT("%s%d"), a_PresetName.c_str(), l_Serial++));
		while( m_Textures.end() != m_Textures.find(l_NewName) )
		{
			l_NewName = wxString::Format(wxT("%s%d"), a_PresetName.c_str(), l_Serial++);
		}
		a_PresetName = l_NewName;
	}

	a_pNewUnit->setName(a_PresetName);
	m_Textures.insert(std::make_pair(a_PresetName, a_pNewUnit));
	return a_pNewUnit;
}

void TextureManager::asyncFunc(std::shared_ptr<TextureUnit> a_Texutre, std::function<void(std::shared_ptr<TextureUnit>)> a_Callback)
{
	loadTextureFile(a_Texutre);
	if( nullptr != a_Callback ) a_Callback(a_Texutre);
}

void TextureManager::loadTextureFile(std::shared_ptr<TextureUnit> a_pTarget)
{
	std::shared_ptr<ImageData> l_pImageData = ImageManager::singleton().getData(a_pTarget->getName()).second;
	int l_TextureID = 0;
	glm::ivec3 l_Dim(l_pImageData->getSize());
	switch( l_pImageData->getType() )
	{
		case TEXTYPE_SIMPLE_1D:{
			l_TextureID = GDEVICE()->allocateTexture(l_Dim.x, l_pImageData->getFormat());
			GDEVICE()->updateTexture(l_TextureID, 0, l_Dim.x, 0, l_pImageData->getPixels(0));
			}break;

		case TEXTYPE_SIMPLE_2D:{
			l_TextureID = GDEVICE()->allocateTexture(glm::ivec2(l_Dim.x, l_Dim.y), l_pImageData->getFormat());
			GDEVICE()->updateTexture(l_TextureID, 0, glm::ivec2(l_Dim.x, l_Dim.y), glm::zero<glm::ivec2>(), 0, l_pImageData->getPixels(0));
			}break;

		case TEXTYPE_SIMPLE_2D_ARRAY:
		case TEXTYPE_SIMPLE_CUBE:{
			l_TextureID = GDEVICE()->allocateTexture(glm::ivec2(l_Dim.x, l_Dim.y), l_pImageData->getFormat(), l_Dim.z, l_pImageData->getType() == TEXTYPE_SIMPLE_CUBE);
			for( int i=0 ; i<l_Dim.z ; ++i ) GDEVICE()->updateTexture(l_TextureID, 0, glm::ivec2(l_Dim.x, l_Dim.y), glm::zero<glm::ivec2>(), i, l_pImageData->getPixels(i));
			}break;

		case TEXTYPE_SIMPLE_3D:{
			l_TextureID = GDEVICE()->allocateTexture(l_pImageData->getSize(), l_pImageData->getFormat());
			GDEVICE()->updateTexture(l_TextureID, 0, l_Dim, glm::zero<glm::ivec3>(), 0, l_pImageData->getPixels(0));
			}break;

		default:break;
	}
	a_pTarget->setTextureID(l_TextureID);
	GDEVICE()->generateMipmap(l_TextureID);
	a_pTarget->setReady();
}
#pragma endregion

}