// ImageImporter.cpp
//
// 2017/06/02 Ian Wu/Real0000
//
#include "CommonUtil.h"
#include "ImageImporter.h"

namespace R
{
	
#pragma region ImageData
//
// ImageData
//
ImageData::ImageData()
	: m_bReady(false)
	, m_pImageData(nullptr)
	, m_Format(PixelFormat::unknown)
	, m_Dim(0, 0, 0)
	, m_Type(TEXTYPE_SIMPLE_2D)
{
}

ImageData::~ImageData()
{
	clear();
}

void ImageData::init(wxString a_Filepath)
{
	wxString l_FileExtension(getFileExt(a_Filepath).MakeLower());
	if( l_FileExtension == wxT("dds") ) loadDDS(a_Filepath);
	else loadGeneral(a_Filepath);
}

void ImageData::loadGeneral(wxString a_Filepath)
{
	wxImage l_TempImage(a_Filepath);
	if( !l_TempImage.IsOk() ) return;

	unsigned char *l_pAlphaBuff = l_TempImage.GetAlpha();
	unsigned char *l_pRGBBuff = l_TempImage.GetData();
	m_Dim = glm::ivec3(l_TempImage.GetWidth(), l_TempImage.GetHeight(), 1);
	m_Format = PixelFormat::rgba8_unorm;
	m_pImageData = new unsigned char[m_Dim.x * m_Dim.y * 4];//rgba
	std::function<unsigned char(unsigned char *, unsigned int)> l_GetAlphaFunc = nullptr;
	if( nullptr == l_pAlphaBuff ) l_GetAlphaFunc = [](unsigned char *a_pSrc, unsigned int a_Offset)->unsigned char{ return 0xff; };
	else l_GetAlphaFunc = [](unsigned char *a_pSrc, unsigned int a_Offset)->unsigned char{ return a_pSrc[a_Offset]; };
		
	for( int y=0 ; y<m_Dim.y ; ++y )
	{
		for( int x=0 ; x<m_Dim.x ; ++x )
		{
			unsigned int l_Offset = m_Dim.x * y + x;
			m_pImageData[l_Offset * 4] = l_pRGBBuff[l_Offset * 3];
			m_pImageData[l_Offset * 4 + 1] = l_pRGBBuff[l_Offset * 3 + 1];
			m_pImageData[l_Offset * 4 + 2] = l_pRGBBuff[l_Offset * 3 + 2];
			m_pImageData[l_Offset * 4 + 3] = l_GetAlphaFunc(l_pAlphaBuff, l_Offset);
		}
	}
	m_RefSurfacePtr.push_back(m_pImageData);
	m_bReady = true;
}

void ImageData::clear()
{
	SAFE_DELETE_ARRAY(m_pImageData)
	m_RefSurfacePtr.clear();
}
#pragma endregion

#pragma region ImageManager
//
// ImageManager
//
ImageManager& ImageManager::singleton()
{
	static ImageManager s_Inst;
	return s_Inst;
}

ImageManager::ImageManager()
	: SearchPathSystem(std::bind(&ImageManager::loadFile, this, std::placeholders::_1, std::placeholders::_2), std::bind(&defaultNewFunc<ImageData>))
{
	wxInitAllImageHandlers();
}

ImageManager::~ImageManager()
{
}

void ImageManager::loadFile(std::shared_ptr<ImageData> a_pInst, wxString a_Path)
{
	a_pInst->init(a_Path);
}
#pragma endregion

}