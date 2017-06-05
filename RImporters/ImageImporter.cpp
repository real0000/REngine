// ImageImporter.cpp
//
// 2017/06/02 Ian Wu/Real0000
//
#include "CommonUtil.h"
#include "ImageImporter.h"

#include "gli/gli.h"

namespace R
{
	
#pragma region ImageData
//
// ImageData
//
ImageData::ImageData()
	: m_bReady(false)
	, m_pPixelData(nullptr)
	, m_Format(PixelFormat::unknown)
	, m_Dim(0, 0, 0)
{
}

ImageData::~ImageData()
{
	SAFE_DELETE_ARRAY(m_pPixelData)
}

void ImageData::init(wxString a_Path)
{
	wxString l_FileExtension(getFileExt(a_Path).MakeLower());
	if( l_FileExtension == wxT("dds") )
	{
	
	}
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
}

ImageManager::~ImageManager()
{
}

void ImageManager::loadFile(ImageData *a_pInst, wxString a_Path)
{
	a_pInst->init(a_Path);
}
#pragma endregion

}