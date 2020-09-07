// ImageImporter.cpp
//
// 2017/06/02 Ian Wu/Real0000
//
#include "CommonUtil.h"
#include "ImageImporter.h"

#include "wx/file.h"

#define TINYEXR_IMPLEMENTATION
#include "tinyexr.h"

namespace R
{
	
#pragma region ImageData
//
// ImageData
//
ImageData::ImageData()
	: m_bReady(false)
	, m_pImageData(nullptr)
	, m_Formats(0)
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
	if( wxT("dds") == l_FileExtension ) loadDDS(a_Filepath);
	else if( wxT("exr") == l_FileExtension ) loadExr(a_Filepath);
	else loadGeneral(a_Filepath);
}

void ImageData::loadExr(wxString a_Filepath)
{
	wxFileOffset l_FileLength = 0;
	unsigned char *l_pFileBuffer = nullptr;
	{
		wxFile l_File;
		if( !l_File.Open(a_Filepath) ) return;

		l_FileLength = l_File.Length();

		// create enough space for the file data
		l_pFileBuffer = new unsigned char[l_FileLength];
		if( nullptr == l_pFileBuffer ) return;
		memset(l_pFileBuffer, 0, sizeof(unsigned char) * l_FileLength);

		// read the data in
		l_File.Read(l_pFileBuffer, l_FileLength);
		l_File.Close();
	}

	EXRVersion l_Version;
	int l_Res = ParseEXRVersionFromMemory(&l_Version, l_pFileBuffer, l_FileLength);
	if( TINYEXR_SUCCESS != l_Res )
	{
		printf("file load failed : %s", static_cast<const char *>(a_Filepath.c_str()));
		delete[] l_pFileBuffer;
		return;
	}

	if( l_Version.non_image )
	{
		printf("NOT YET IMPLEMENTED : %s", a_Filepath.c_str());
		delete[] l_pFileBuffer;
		return;
	}
		
	const char *l_pError = nullptr;
	
	EXRHeader l_Header;
	InitEXRHeader(&l_Header);
		
	EXRImage l_Image;
	InitEXRImage(&l_Image);

	if( l_Version.multipart )
	{
		EXRHeader **l_ppHeaders = nullptr;
		int l_NumPart = 0;

		l_Res = ParseEXRMultipartHeaderFromMemory(&l_ppHeaders, &l_NumPart, &l_Version, l_pFileBuffer, l_FileLength, &l_pError);
		if( TINYEXR_SUCCESS != l_Res || l_NumPart < 1 )
		{
			printf(l_pError);
			delete[] l_pFileBuffer;
			FreeEXRErrorMessage(l_pError);
			return;
		}

		l_Header = *l_ppHeaders[0];
		for( unsigned int i=0 ; i<l_NumPart ; ++i )
		{
			if( 0 != i ) FreeEXRHeader(l_ppHeaders[i]);
			delete[] l_pFileBuffer;
			free(l_ppHeaders[i]);
		}
		free(l_ppHeaders);
	}
	else
	{
		l_Res = ParseEXRHeaderFromMemory(&l_Header, &l_Version, l_pFileBuffer, l_FileLength, &l_pError);
		if( TINYEXR_SUCCESS != l_Res )
		{
			printf(l_pError);
			delete[] l_pFileBuffer;
			FreeEXRErrorMessage(l_pError);
			return;
		}
	}

	l_Res = LoadEXRImageFromMemory(&l_Image, &l_Header, l_pFileBuffer, l_FileLength, &l_pError);
	if( TINYEXR_SUCCESS != l_Res )
	{
		printf(l_pError);
		delete[] l_pFileBuffer;
		FreeEXRHeader(&l_Header);
		FreeEXRErrorMessage(l_pError);
		return ;
	}

	m_Dim.x = l_Image.width;
	m_Dim.y = l_Image.height;
	m_Dim.z = 1;//l_Header.num_channels;
	auto l_pLoadFunc = nullptr != l_Image.tiles ?
		(std::function<void(unsigned int)>)[&](unsigned int a_Layer) -> void
		{
			unsigned char *l_pDst = m_RefSurfacePtr[a_Layer];
			unsigned int l_PixelSize = getPixelSize(m_Formats[a_Layer]);

			for( unsigned int i=0 ; i<l_Image.num_tiles ; ++i )
			{
				EXRTile &l_Tile = l_Image.tiles[i];
				for( unsigned int y=0 ; y<l_Tile.height ; ++y )
				{
					for( unsigned int x=0 ; x<l_Tile.width ; ++x )
					{
						unsigned char *l_pSrc = l_Tile.images[a_Layer] + l_PixelSize * (y * l_Tile.width + x);
						unsigned char *l_pPixel = l_pDst + l_PixelSize * (l_Tile.offset_x * l_Header.tile_size_x + x + (l_Tile.offset_y * l_Header.tile_size_y + y) * m_Dim.x);
						memcpy(l_pPixel, l_pSrc, l_PixelSize);
					}
				}
			}
		} :
		(std::function<void(unsigned int)>)[&](unsigned int a_Layer) -> void
	{
		unsigned char *l_pDst = m_RefSurfacePtr[a_Layer];
		unsigned int l_PixelSize = getPixelSize(m_Formats[a_Layer]);
		memcpy(l_pDst, l_Image.images[a_Layer], m_Dim.x * m_Dim.y * l_PixelSize);
	};
	
	unsigned int l_TotalSize = 0;
	std::vector<unsigned int> l_Offsets;
	for( unsigned int i=0 ; i<m_Dim.z ; ++i )
	{
		l_Offsets.push_back(l_TotalSize);
		switch( l_Header.pixel_types[i] )
		{	
			case TINYEXR_PIXELTYPE_HALF:
				m_Formats.push_back(PixelFormat::rgba16_float);
				l_TotalSize += m_Dim.x * m_Dim.y * getPixelSize(PixelFormat::rgba16_float);
				break;

			case TINYEXR_PIXELTYPE_FLOAT:
				m_Formats.push_back(PixelFormat::rgba32_float);
				l_TotalSize += m_Dim.x * m_Dim.y * getPixelSize(PixelFormat::rgba32_float);
				break;
			
			default://TINYEXR_PIXELTYPE_UINT
				m_Formats.push_back(PixelFormat::rgba8_unorm);
				l_TotalSize += m_Dim.x * m_Dim.y * getPixelSize(PixelFormat::rgba8_unorm);
				break;
		}
	}

	m_pImageData = new unsigned char[l_TotalSize];
	for( unsigned int i=0 ; i<m_Dim.z ; ++i )
	{
		m_RefSurfacePtr.push_back(m_pImageData + l_Offsets[i]);
		l_pLoadFunc(i);
	}

	FreeEXRImage(&l_Image);
	FreeEXRHeader(&l_Header);
	
	m_bReady = true;
}

void ImageData::loadGeneral(wxString a_Filepath)
{
	wxImage l_TempImage(a_Filepath);
	if( !l_TempImage.IsOk() ) return;

	unsigned char *l_pAlphaBuff = l_TempImage.GetAlpha();
	unsigned char *l_pRGBBuff = l_TempImage.GetData();
	m_Dim = glm::ivec3(l_TempImage.GetWidth(), l_TempImage.GetHeight(), 1);
	m_Formats.push_back(PixelFormat::rgba8_unorm);
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