// Texture.cpp
//
// 2014/06/21 Ian Wu/Real0000
//
#include "CommonUtil.h"
#include "DeviceManager.h"
#include "Module/Texture/Texture.h"
#include "wxCanvas.h"

namespace R
{

static FIBITMAP* loadImage(wxString l_Filename)
{
	FREE_IMAGE_FORMAT l_Format = FreeImage_GetFIFFromFilename(l_Filename.c_str());
	FIBITMAP *l_pImage = FreeImage_Load(l_Format, l_Filename.c_str());
	if( NULL == l_pImage ) return NULL;

	FIBITMAP *l_p32Image = FreeImage_ConvertTo32Bits(l_pImage);
	FreeImage_Unload(l_pImage);
	if( NULL == l_p32Image ) return NULL;

	return l_p32Image;
}

#pragma region TextureUnit
//
// TextureUnit
//
TextureUnit::TextureUnit()
	: m_TextureID(0)
	, m_Name(wxT(""))
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
	return GDEVICE()->getTexturePixelFormat(m_TextureID);
}

unsigned int TextureUnit::getPixelSize()
{
	return GDEVICE()->getTexturePixelSize(m_TextureID);
}

glm::ivec3 TextureUnit::getDimension()
{
	return GDEVICE()->getTextureDimension(m_TextureID);
}
#pragma endregion

#pragma region Texture1D
//
// Texture1D
//
Texture1D::Texture1D()
	: TextureUnit()
{
}

Texture1D::~Texture1D()
{
}

void Texture1D::init(unsigned int l_Width, PixelFormat::Key l_Format, void *l_pInitData)
{
	unsigned int l_TextureID = GDEVICE()->allocateTexture1D(l_Width, l_Format);
	setTextureID(l_TextureID);

	GDEVICE()->updateTexture1D(l_TextureID, l_Width, 0, l_pInitData);
}

void Texture1D::update(void *l_pData, unsigned int l_OffsetX, unsigned int l_Width)
{
	GDEVICE()->updateTexture1D(getTextureID(), l_Width, 0, l_pData);
}
#pragma endregion

#pragma region Texture2D
//
// Texture2D
//
Texture2D::Texture2D()
	: TextureUnit()
{
}

Texture2D::~Texture2D()
{
}

void Texture2D::init(wxString l_File)
{
	FIBITMAP *l_pBmp = loadImage(l_File);
	unsigned int l_Width = FreeImage_GetWidth(l_pBmp);
	unsigned int l_Height = FreeImage_GetHeight(l_pBmp);
	void *l_pPixelData = FreeImage_GetBits(l_pBmp);
	init(l_Width, l_Height, PixelFormat::rgba8_unorm, l_pPixelData);
	FreeImage_Unload(l_pBmp);
}
	
void Texture2D::init(unsigned int l_Width, unsigned int l_Height, PixelFormat::Key l_Format, void *l_pInitData)
{
	init(glm::ivec2(l_Width, l_Height), l_Format, l_pInitData);
}

void Texture2D::init(glm::ivec2 l_Size, PixelFormat::Key l_Format, void *l_pInitData)
{
	unsigned int l_TextureID = GDEVICE()->allocateTexture2D(l_Size, l_Format);
	setTextureID(l_TextureID);
	update(l_pInitData, glm::ivec2(0, 0), l_Size);
}

void Texture2D::update(void *l_pData, unsigned int l_OffsetX, unsigned int l_OffsetY, unsigned int l_Width, unsigned int l_Height)
{
	update(l_pData, glm::ivec2(l_OffsetX, l_OffsetY), glm::ivec2(l_Width, l_Height));
}

void Texture2D::update(void *l_pData, glm::ivec2 l_Offset, glm::ivec2 l_Size)
{
	GDEVICE()->updateTexture2D(getTextureID(), l_Size, l_Offset, 0, l_pData);
}
#pragma endregion

#pragma region Texture2DArray
//
// Texture2DArray
//
Texture2DArray::Texture2DArray()
	: TextureUnit()
{
}

Texture2DArray::~Texture2DArray()
{
}

void Texture2DArray::init(std::vector<wxString> l_Filelist)
{
	assert(!l_Filelist.empty());

	std::vector<FIBITMAP *> l_BmpList;
	for( unsigned int i=0 ; i<l_Filelist.size() ; ++i )
	{
		l_BmpList.push_back(loadImage(l_Filelist[i]));
	}

	unsigned int l_Width = FreeImage_GetWidth(l_BmpList.front());
	unsigned int l_Height = FreeImage_GetHeight(l_BmpList.front());
#ifdef _DEBUG
	for( unsigned int i=1 ; i<l_BmpList.size() ; ++i )
	{
		unsigned int l_WidthForCheck = FreeImage_GetWidth(l_BmpList[i]);
		unsigned int l_HeightForCheck = FreeImage_GetHeight(l_BmpList[i]);
		assert(l_WidthForCheck == l_Width && l_HeightForCheck == l_Height);
	}
#endif
	unsigned int l_TextureID = GDEVICE()->allocateTexture2D(glm::ivec2(l_Width, l_Height), PixelFormat::rgba8_unorm, l_BmpList.size());
	setTextureID(l_TextureID);

	for( unsigned int i=0 ; i<l_BmpList.size() ; ++i )
	{
		void *l_pPixelData = FreeImage_GetBits(l_BmpList[i]);
		update(l_pPixelData, 0, 0, i, l_Width, l_Height);
		FreeImage_Unload(l_BmpList[i]);
	}
}

void Texture2DArray::init(unsigned int l_Width, unsigned int l_Height, unsigned int l_NumTexture, PixelFormat::Key l_Format, void *l_pInitData)
{
	init(glm::ivec2(l_Width, l_Height), l_NumTexture, l_Format, l_pInitData);
}

void Texture2DArray::init(glm::ivec2 l_Size, unsigned int l_NumTexture, PixelFormat::Key l_Format, void *l_pInitData)
{
	unsigned int l_TextureID = GDEVICE()->allocateTexture2D(l_Size, l_Format, l_NumTexture);
	setTextureID(l_TextureID);

	if( nullptr == l_pInitData ) for( unsigned int i=0 ; i<l_NumTexture ; ++i ) update(nullptr, glm::ivec2(0, 0), i, l_Size);
	else
	{
		char *l_pSrcBuff = (char *)l_pInitData;
		unsigned int l_PixelSize = GDEVICE()->getPixelSize(l_Format);
		for( unsigned int i=0 ; i<l_NumTexture ; ++i )
		{
			update(l_pSrcBuff, glm::ivec2(0, 0), i, l_Size);
			l_pSrcBuff += l_Size.x * l_Size.y * l_PixelSize;
		}
	}
}

void Texture2DArray::update(void *l_pData, unsigned int l_OffsetX, unsigned int l_OffsetY, unsigned int l_Idx, unsigned int l_Width, unsigned int l_Height)
{
	update(l_pData, glm::ivec2(l_OffsetX, l_OffsetY), l_Idx, glm::ivec2(l_Width, l_Height));
}

void Texture2DArray::update(void *l_pData, glm::ivec2 l_Offset, unsigned int l_Idx, glm::ivec2 l_Size)
{
	GDEVICE()->updateTexture2D(getTextureID(), l_Size, l_Offset, l_Idx, l_pData);	
}
#pragma endregion

#pragma region Texture3D
//
// Texture3D
//
Texture3D::Texture3D()
	: TextureUnit()
{
}

Texture3D::~Texture3D()
{
}

/*void Texture3D::init(wxString l_File)
{

}*/

void Texture3D::init(unsigned int l_Width, unsigned int l_Height, unsigned int l_Depth, PixelFormat::Key l_Format, void *l_pInitData)
{
	init(glm::ivec3(l_Width, l_Height, l_Depth), l_Format, l_pInitData);
}

void Texture3D::init(glm::ivec3 l_Size, PixelFormat::Key l_Format, void *l_pInitData)
{
	unsigned int l_TextureID = GDEVICE()->allocateTexture3D(l_Size, l_Format);
	setTextureID(l_TextureID);
	update(l_pInitData, glm::ivec3(0, 0, 0), l_Size);
}

void Texture3D::update(void *l_pData, unsigned int l_OffsetX, unsigned int l_OffsetY, unsigned int l_OffsetZ, unsigned int l_Width, unsigned int l_Height, unsigned int l_Depth)
{
	update(l_pData, glm::ivec3(l_OffsetX, l_OffsetY, l_OffsetZ), glm::ivec3(l_Width, l_Height, l_Depth));
}

void Texture3D::update(void *l_pData, glm::ivec3 l_Offset, glm::ivec3 l_Size)
{
	GDEVICE()->updateTexture3D(getTextureID(), l_Size, l_Offset, l_pData);
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

TextureUnitPtr TextureManager::newTexture1D(wxString l_Name, unsigned int l_Width, PixelFormat::Key l_Format, void *l_pInitData)
{
	Texture1D *l_pNewTexture = new Texture1D();
	l_pNewTexture->init(l_Width, l_Format, l_pInitData);
	return packNewTextureUnit(l_Name, l_pNewTexture);
}

TextureUnitPtr TextureManager::newTexture2D(wxString l_Filename)
{
	auto it = m_Textures.find(l_Filename);
	if( m_Textures.end() != it ) return it->second;

	Texture2D *l_pNewTexture = new Texture2D();
	l_pNewTexture->init(l_Filename);
	return packNewTextureUnit(l_Filename, l_pNewTexture);
}

TextureUnitPtr TextureManager::newTexture2D(wxString l_Name, unsigned int l_Width, unsigned int l_Height, PixelFormat::Key l_Format, void *l_pInitData)
{
	return newTexture2D(l_Name, glm::ivec2(l_Width, l_Height), l_Format, l_pInitData);
}

TextureUnitPtr TextureManager::newTexture2D(wxString l_Name, glm::ivec2 l_Size, PixelFormat::Key l_Format, void *l_pInitData)
{
	Texture2D *l_pNewTexture = new Texture2D();
	l_pNewTexture->init(l_Size, l_Format, l_pInitData);
	return packNewTextureUnit(l_Name, l_pNewTexture);
}

TextureUnitPtr TextureManager::newTexture2DArray(std::vector<wxString> l_FileList)
{
	Texture2DArray *l_pNewTextureArray = new Texture2DArray();
	l_pNewTextureArray->init(l_FileList);
	return packNewTextureUnit(wxT("TextureSet"), l_pNewTextureArray);
}

TextureUnitPtr TextureManager::newTexture2DArray(wxString l_Name, unsigned int l_Width, unsigned int l_Height, unsigned int l_ArraySize, PixelFormat::Key l_Format, void *l_pInitData)
{
	return newTexture2DArray(l_Name, glm::ivec2(l_Width, l_Height), l_ArraySize, l_Format, l_pInitData);
}

TextureUnitPtr TextureManager::newTexture2DArray(wxString l_Name, glm::ivec2 l_Size, unsigned int l_ArraySize, PixelFormat::Key l_Format, void *l_pInitData)
{
	Texture2DArray *l_pNewTextureArray = new Texture2DArray();
	l_pNewTextureArray->init(l_Size, l_ArraySize, l_Format, l_pInitData);
	return packNewTextureUnit(l_Name, l_pNewTextureArray);
}

TextureUnitPtr TextureManager::newTexture3D(wxString l_Name, unsigned int l_Width, unsigned int l_Height, unsigned int l_Depth, PixelFormat::Key l_Format, void *l_pInitData)
{
	return newTexture3D(l_Name, glm::ivec3(l_Width, l_Height, l_Depth), l_Format, l_pInitData);
}

TextureUnitPtr TextureManager::newTexture3D(wxString l_Name, glm::ivec3 l_Size, PixelFormat::Key l_Format, void *l_pInitData)
{
	Texture3D *l_pNewTexture = new Texture3D();
	l_pNewTexture->init(l_Size, l_Format, l_pInitData);
	return packNewTextureUnit(l_Name, l_pNewTexture);
}

void TextureManager::recycle(wxString l_Name)
{
	auto it = m_Textures.find(l_Name);
	if( m_Textures.end() == it ) return ;
	if( 2 < it->second.use_count() ) return ;// 2 -> map 1, who called release one
	
	m_Textures.erase(it);
}

TextureUnitPtr TextureManager::getTexture(wxString l_Name)
{
	auto it = m_Textures.find(l_Name);
	if( m_Textures.end() == it ) return TextureUnitPtr(nullptr);
	return it->second;
}

void TextureManager::clearCache()
{
	m_Textures.clear();
}

TextureUnitPtr TextureManager::packNewTextureUnit(wxString l_PresetName, TextureUnit *l_pNewUnit)
{
	if( m_Textures.end() != m_Textures.find(l_PresetName) )
	{
		unsigned int l_Serial = 0;
		wxString l_NewName(wxString::Format(wxT("%s%d"), l_PresetName.c_str(), l_Serial++));
		while( m_Textures.end() != m_Textures.find(l_NewName) )
		{
			l_NewName = wxString::Format(wxT("%s%d"), l_PresetName.c_str(), l_Serial++);
		}
		l_PresetName = l_NewName;
	}

	l_pNewUnit->setName(l_PresetName);
	return (m_Textures[l_PresetName] = TextureUnitPtr(l_pNewUnit));
}
#pragma endregion

}