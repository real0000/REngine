//--------------------------------------------------------------------------------------
// File: DDSTextureLoader.cpp
//
// Functions for loading a DDS texture and creating a Direct3D runtime resource for it
//
// Note these functions are useful as a light-weight runtime loader for DDS files. For
// a full-featured DDS file reader, writer, and texture processing pipeline see
// the 'Texconv' sample and the 'DirectXTex' library.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// http://go.microsoft.com/fwlink/?LinkId=248926
// http://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------

#include "CommonUtil.h"

#include <memory>
#include "wx/file.h"
#include "ImageImporter.h"

namespace R
{

enum DDS_ALPHA_MODE
{
    DDS_ALPHA_MODE_UNKNOWN       = 0,
    DDS_ALPHA_MODE_STRAIGHT      = 1,
    DDS_ALPHA_MODE_PREMULTIPLIED = 2,
    DDS_ALPHA_MODE_OPAQUE        = 3,
    DDS_ALPHA_MODE_CUSTOM        = 4,
};

//--------------------------------------------------------------------------------------
// Macros
//--------------------------------------------------------------------------------------
#ifndef MAKEFOURCC
    #define MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
                ((uint32_t)(uint8_t)(ch0) | ((uint32_t)(uint8_t)(ch1) << 8) |       \
                ((uint32_t)(uint8_t)(ch2) << 16) | ((uint32_t)(uint8_t)(ch3) << 24 ))
#endif /* defined(MAKEFOURCC) */

//--------------------------------------------------------------------------------------
// DDS file structure definitions
//
// See DDS.h in the 'Texconv' sample and the 'DirectXTex' library
//--------------------------------------------------------------------------------------

const uint32_t DDS_MAGIC = 0x20534444; // "DDS "

struct DDS_PIXELFORMAT
{
    uint32_t    size;
    uint32_t    flags;
    uint32_t    fourCC;
    uint32_t    RGBBitCount;
    uint32_t    RBitMask;
    uint32_t    GBitMask;
    uint32_t    BBitMask;
    uint32_t    ABitMask;
};

#define DDS_FOURCC      0x00000004  // DDPF_FOURCC
#define DDS_RGB         0x00000040  // DDPF_RGB
#define DDS_LUMINANCE   0x00020000  // DDPF_LUMINANCE
#define DDS_ALPHA       0x00000002  // DDPF_ALPHA
#define DDS_BUMPDUDV    0x00080000  // DDPF_BUMPDUDV

#define DDS_HEADER_FLAGS_VOLUME         0x00800000  // DDSD_DEPTH

#define DDS_HEIGHT 0x00000002 // DDSD_HEIGHT
#define DDS_WIDTH  0x00000004 // DDSD_WIDTH

#define DDS_CUBEMAP_POSITIVEX 0x00000600 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEX
#define DDS_CUBEMAP_NEGATIVEX 0x00000a00 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEX
#define DDS_CUBEMAP_POSITIVEY 0x00001200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEY
#define DDS_CUBEMAP_NEGATIVEY 0x00002200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEY
#define DDS_CUBEMAP_POSITIVEZ 0x00004200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEZ
#define DDS_CUBEMAP_NEGATIVEZ 0x00008200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEZ

#define DDS_CUBEMAP_ALLFACES ( DDS_CUBEMAP_POSITIVEX | DDS_CUBEMAP_NEGATIVEX |\
                               DDS_CUBEMAP_POSITIVEY | DDS_CUBEMAP_NEGATIVEY |\
                               DDS_CUBEMAP_POSITIVEZ | DDS_CUBEMAP_NEGATIVEZ )

#define DDS_CUBEMAP 0x00000200 // DDSCAPS2_CUBEMAP

enum DDS_MISC_FLAGS2
{
    DDS_MISC_FLAGS2_ALPHA_MODE_MASK = 0x7L,
};

struct DDS_HEADER
{
    uint32_t        size;
    uint32_t        flags;
    uint32_t        height;
    uint32_t        width;
    uint32_t        pitchOrLinearSize;
    uint32_t        depth; // only if DDS_HEADER_FLAGS_VOLUME is set in flags
    uint32_t        mipMapCount;
    uint32_t        reserved1[11];
    DDS_PIXELFORMAT ddspf;
    uint32_t        caps;
    uint32_t        caps2;
    uint32_t        caps3;
    uint32_t        caps4;
    uint32_t        reserved2;
};

struct DDS_HEADER_DXT10
{
    PixelFormat::Key dxgiFormat;
    uint32_t        resourceDimension;
    uint32_t        miscFlag; // see D3D11_RESOURCE_MISC_FLAG
    uint32_t        arraySize;
    uint32_t        miscFlags2;
};

// D3D11_RESOURCE_DIMENSION
enum DDS_DIMENSION
{ 
  DDS_DIMENSION_UNKNOWN = 0,
  DDS_DIMENSION_BUFFER = 1,
  DDS_DIMENSION_TEXTURE1D = 2,
  DDS_DIMENSION_TEXTURE2D = 3,
  DDS_DIMENSION_TEXTURE3D = 4
};

// D3D11_RESOURCE_MISC_FLAG
enum MISC_FLAG
{ 
  MISC_FLAG_GENERATE_MIPS					= 0x1L,
  MISC_FLAG_SHARED							= 0x2L,
  MISC_FLAG_TEXTURECUBE						= 0x4L,
  MISC_FLAG_DRAWINDIRECT_ARGS				= 0x10L,
  MISC_FLAG_BUFFER_ALLOW_RAW_VIEWS			= 0x20L,
  MISC_FLAG_BUFFER_STRUCTURED				= 0x40L,
  MISC_FLAG_RESOURCE_CLAMP					= 0x80L,
  MISC_FLAG_SHARED_KEYEDMUTEX				= 0x100L,
  MISC_FLAG_GDI_COMPATIBLE					= 0x200L,
  MISC_FLAG_SHARED_NTHANDLE					= 0x800L,
  MISC_FLAG_RESTRICTED_CONTENT				= 0x1000L,
  MISC_FLAG_RESTRICT_SHARED_RESOURCE		= 0x2000L,
  MISC_FLAG_RESTRICT_SHARED_RESOURCE_DRIVER	= 0x4000L,
  MISC_FLAG_GUARDED							= 0x8000L,
  MISC_FLAG_TILE_POOL						= 0x20000L,
  MISC_FLAG_TILED							= 0x40000L,
  MISC_FLAG_HW_PROTECTED					= 0x80000L
};

static PixelFormat::Key GetDXGIFormat(const DDS_PIXELFORMAT& ddpf);
static void GetSurfaceInfo(size_t width, size_t height, PixelFormat::Key fmt, size_t* outNumBytes, size_t* outRowBytes, size_t* outNumRows);
static DDS_ALPHA_MODE GetAlphaMode(_In_ const DDS_HEADER* header);

//--------------------------------------------------------------------------------------
void ImageData::loadDDS(wxString a_Filepath)
{
	int64 l_FileLength = 0;
	{
		wxFile l_File;
		if( !l_File.Open(a_Filepath) ) return;

		l_FileLength = l_File.Length();
		// Need at least enough data to fill the header and magic number to be a valid DDS
		if(l_FileLength < (sizeof(DDS_HEADER) + sizeof(uint32_t))) return;

		// create enough space for the file data
		m_pImageData = new uint8_t[l_FileLength];
		if( nullptr == m_pImageData ) return;
		memset(m_pImageData, 0, sizeof(uint8_t) * l_FileLength);

		// read the data in
		l_File.Read(m_pImageData, l_FileLength);
		l_FileLength = l_FileLength;
		l_File.Close();
	}

    // DDS files always start with the same magic number ("DDS ")
    uint32_t dwMagicNumber = *reinterpret_cast<const uint32_t*>(m_pImageData);
    if (dwMagicNumber != DDS_MAGIC)
	{
		clear();
		return ;
	}

    auto hdr = reinterpret_cast<const DDS_HEADER*>(m_pImageData + sizeof(uint32_t));

    // Verify header to validate DDS file
    if (hdr->size != sizeof(DDS_HEADER) ||
        hdr->ddspf.size != sizeof(DDS_PIXELFORMAT))
    {
		clear();
        return ;
    }

    // Check for DX10 extension
    bool bDXT10Header = false;
    if ((hdr->ddspf.flags & DDS_FOURCC) &&
        (MAKEFOURCC('D', 'X', '1', '0') == hdr->ddspf.fourCC))
    {
        // Must be long enough for both headers and magic value
        if (l_FileLength < (sizeof(DDS_HEADER) + sizeof(uint32_t) + sizeof(DDS_HEADER_DXT10)))
		{
			clear();
			return;
		}

        bDXT10Header = true;
    }

    // setup the pointers in the process request
    const DDS_HEADER *header = hdr;
    ptrdiff_t offset = sizeof(uint32_t) + sizeof(DDS_HEADER)
        + (bDXT10Header ? sizeof(DDS_HEADER_DXT10) : 0);
    m_RefSurfacePtr.push_back(m_pImageData + offset);
    l_FileLength = l_FileLength - offset;

	m_Dim = glm::ivec3(header->width, header->height, header->depth);

	uint32_t resDim = DDS_DIMENSION_UNKNOWN;
	UINT arraySize = 1;
	PixelFormat::Key l_Format = PixelFormat::unknown;
	bool isCubeMap = false;

    size_t mipCount = std::max<size_t>(header->mipMapCount, 1);

    if ((header->ddspf.flags & DDS_FOURCC) &&
        (MAKEFOURCC('D', 'X', '1', '0') == header->ddspf.fourCC))
    {
        auto d3d10ext = (DDS_HEADER_DXT10 *)((const char*)header + sizeof(DDS_HEADER));

        arraySize = d3d10ext->arraySize;
        if (arraySize == 0)
        {
			clear();
            return ;
        }

		switch( (unsigned int)d3d10ext->dxgiFormat )
		{
			case 130: d3d10ext->dxgiFormat = PixelFormat::p208; break;
			case 131: d3d10ext->dxgiFormat = PixelFormat::v208; break;
			case 132: d3d10ext->dxgiFormat = PixelFormat::v408; break;
			case 0xffffffff: d3d10ext->dxgiFormat = PixelFormat::uint; break;
			default:break;
		}

        switch (d3d10ext->dxgiFormat)
        {
		case PixelFormat::ai44:
        case PixelFormat::ia44:
        case PixelFormat::p8:
        case PixelFormat::ap8:
			clear();
            return ;

        default:{
            if (getPixelSize(d3d10ext->dxgiFormat) == 0)
            {
				clear();
                return ;
			}
			}break;
        }

        l_Format = d3d10ext->dxgiFormat;

        switch (d3d10ext->resourceDimension)
        {
        case DDS_DIMENSION_TEXTURE2D:
            if (d3d10ext->miscFlag & MISC_FLAG_TEXTURECUBE)
            {
                arraySize *= 6;
                isCubeMap = true;
				m_Type = TEXTYPE_SIMPLE_CUBE;
            }
			else m_Type = TEXTYPE_SIMPLE_2D;
            m_Dim.z = 1;
            break;

        case DDS_DIMENSION_TEXTURE3D:
            if (!(header->flags & DDS_HEADER_FLAGS_VOLUME))
            {
				clear();
                return ;
            }

            if (arraySize > 1)
            {
				clear();
                return ;
            }
			m_Type = TEXTYPE_SIMPLE_3D;
            break;

        default:
			clear();
            return ;
        }

        resDim = d3d10ext->resourceDimension;
    }
    else
    {
        l_Format = GetDXGIFormat(header->ddspf);

        if (l_Format == PixelFormat::unknown)
        {
			clear();
            return ;
        }

        if (header->flags & DDS_HEADER_FLAGS_VOLUME)
        {
            resDim = DDS_DIMENSION_TEXTURE3D;
			m_Type = TEXTYPE_SIMPLE_3D;
        }
        else
        {
            if (header->caps2 & DDS_CUBEMAP)
            {
                // We require all six faces to be defined
                if ((header->caps2 & DDS_CUBEMAP_ALLFACES) != DDS_CUBEMAP_ALLFACES)
                {
					clear();
                    return ;
                }

                arraySize = 6;
                isCubeMap = true;
				m_Type = TEXTYPE_SIMPLE_CUBE;
            }

            m_Dim.z = 1;
            resDim = DDS_DIMENSION_TEXTURE2D;

            // Note there's no way for a legacy Direct3D 9 DDS to express a '1D' texture
        }

        assert(getPixelSize(l_Format) != 0);
    }
	
	size_t numBytes = 0;
    size_t rowBytes = 0;
    GetSurfaceInfo(m_Dim.x, m_Dim.y, l_Format, &numBytes, &rowBytes, nullptr);

    if (numBytes > (size_t)l_FileLength)
    {
        clear();
        return ;
    }
	
    size_t NumBytes = 0;
    size_t RowBytes = 0;
    const uint8_t* pSrcBits = m_RefSurfacePtr.front();
    const uint8_t* pEndBits = pSrcBits + l_FileLength;
	m_RefSurfacePtr.clear();
    for (size_t j = 0; j < arraySize; j++)
    {
        size_t w = m_Dim.x;
        size_t h = m_Dim.y;
        size_t d = m_Dim.z;
        for (size_t i = 0; i < mipCount; i++)
        {
            GetSurfaceInfo(w,
                h,
                l_Format,
                &NumBytes,
                &RowBytes,
                nullptr
            );

			m_RefSurfacePtr.push_back(const_cast<unsigned char *>(pSrcBits));
            if (pSrcBits + (NumBytes*d) > pEndBits)
            {
				clear();
                return ;
            }

            pSrcBits += NumBytes * d;

            w = std::max<unsigned int>(w >> 1, 1u);
            h = std::max<unsigned int>(h >> 1, 1u);
            d = std::max<unsigned int>(d >> 1, 1u);
        }
    }
	if( TEXTYPE_SIMPLE_2D == m_Type && 1 != arraySize )
	{
		m_Dim.z = arraySize;
		m_Type = TEXTYPE_SIMPLE_2D_ARRAY;
	}
	else if( TEXTYPE_SIMPLE_CUBE == m_Type  )m_Dim.z = arraySize;// 6
	//*alphaMode = GetAlphaMode(header);
	m_Formats.resize(m_Dim.z, l_Format);
	m_bReady = true;
}



//--------------------------------------------------------------------------------------
// Get surface information for a particular format
//--------------------------------------------------------------------------------------
void GetSurfaceInfo(
    _In_ size_t width,
    _In_ size_t height,
    _In_ PixelFormat::Key fmt,
    size_t* outNumBytes,
    _Out_opt_ size_t* outRowBytes,
    _Out_opt_ size_t* outNumRows)
{
    size_t numBytes = 0;
    size_t rowBytes = 0;
    size_t numRows = 0;

    bool bc = false;
    bool packed = false;
    bool planar = false;
    size_t bpe = 0;
    switch (fmt)
    {
	case PixelFormat::bc1_typeless:
	case PixelFormat::bc1_unorm:
	case PixelFormat::bc1_unorm_srgb:
	case PixelFormat::bc4_typeless:
    case PixelFormat::bc4_unorm:
    case PixelFormat::bc4_snorm:
        bc = true;
        bpe = 8;
        break;

	case PixelFormat::bc2_typeless:
	case PixelFormat::bc2_unorm:
	case PixelFormat::bc2_unorm_srgb:
	case PixelFormat::bc3_typeless:
    case PixelFormat::bc3_unorm:
	case PixelFormat::bc3_unorm_srgb:
    case PixelFormat::bc5_typeless:
    case PixelFormat::bc5_unorm:
    case PixelFormat::bc5_snorm:
    case PixelFormat::bc6h_typeless:
    case PixelFormat::bc6h_uf16:
    case PixelFormat::bc6h_sf16:
    case PixelFormat::bc7_typeless:
    case PixelFormat::bc7_unorm:
    case PixelFormat::bc7_unorm_srgb:
        bc = true;
        bpe = 16;
        break;

    case PixelFormat::rgbg8_unorm:
    case PixelFormat::grgb8_unorm:
    case PixelFormat::yuy2:
        packed = true;
        bpe = 4;
        break;

    case PixelFormat::y210:
    case PixelFormat::y216:
        packed = true;
        bpe = 8;
        break;

    case PixelFormat::nv12:
    case PixelFormat::opaque420:
        planar = true;
        bpe = 2;
        break;

    case PixelFormat::p010:
    case PixelFormat::p016:
        planar = true;
        bpe = 4;
        break;
    }

    if (bc)
    {
        size_t numBlocksWide = 0;
        if (width > 0)
        {
            numBlocksWide = std::max<size_t>(1, (width + 3) / 4);
        }
        size_t numBlocksHigh = 0;
        if (height > 0)
        {
            numBlocksHigh = std::max<size_t>(1, (height + 3) / 4);
        }
        rowBytes = numBlocksWide * bpe;
        numRows = numBlocksHigh;
        numBytes = rowBytes * numBlocksHigh;
    }
    else if (packed)
    {
        rowBytes = ((width + 1) >> 1) * bpe;
        numRows = height;
        numBytes = rowBytes * height;
    }
    else if (fmt == DXGI_FORMAT_NV11)
    {
        rowBytes = ((width + 3) >> 2) * 4;
        numRows = height * 2; // Direct3D makes this simplifying assumption, although it is larger than the 4:1:1 data
        numBytes = rowBytes * numRows;
    }
    else if (planar)
    {
        rowBytes = ((width + 1) >> 1) * bpe;
        numBytes = (rowBytes * height) + ((rowBytes * height + 1) >> 1);
        numRows = height + ((height + 1) >> 1);
    }
    else
    {
        size_t bpp = getPixelSize(fmt);
        rowBytes = (width * bpp + 7) / 8; // round up to nearest byte
        numRows = height;
        numBytes = rowBytes * height;
    }

    if (outNumBytes)
    {
        *outNumBytes = numBytes;
    }
    if (outRowBytes)
    {
        *outRowBytes = rowBytes;
    }
    if (outNumRows)
    {
        *outNumRows = numRows;
    }
}


//--------------------------------------------------------------------------------------
#define ISBITMASK( r,g,b,a ) ( ddpf.RBitMask == r && ddpf.GBitMask == g && ddpf.BBitMask == b && ddpf.ABitMask == a )

PixelFormat::Key GetDXGIFormat(const DDS_PIXELFORMAT& ddpf)
{
    if (ddpf.flags & DDS_RGB)
    {
        // Note that sRGB formats are written using the "DX10" extended header

        switch (ddpf.RGBBitCount)
        {
        case 32:
            if (ISBITMASK(0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000))
            {
                return PixelFormat::rgba8_unorm;
            }

            if (ISBITMASK(0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000))
            {
                return PixelFormat::bgra8_unorm;
            }

            if (ISBITMASK(0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000))
            {
                return PixelFormat::bgrx8_unorm;
            }

            // No DXGI format maps to ISBITMASK(0x000000ff,0x0000ff00,0x00ff0000,0x00000000) aka D3DFMT_X8B8G8R8

            // Note that many common DDS reader/writers (including D3DX) swap the
            // the RED/BLUE masks for 10:10:10:2 formats. We assume
            // below that the 'backwards' header mask is being used since it is most
            // likely written by D3DX. The more robust solution is to use the 'DX10'
            // header extension and specify the DXGI_FORMAT_R10G10B10A2_UNORM format directly

            // For 'correct' writers, this should be 0x000003ff,0x000ffc00,0x3ff00000 for RGB data
            if (ISBITMASK(0x3ff00000, 0x000ffc00, 0x000003ff, 0xc0000000))
            {
                return PixelFormat::rgb10a2_unorm;
            }

            // No DXGI format maps to ISBITMASK(0x000003ff,0x000ffc00,0x3ff00000,0xc0000000) aka D3DFMT_A2R10G10B10

            if (ISBITMASK(0x0000ffff, 0xffff0000, 0x00000000, 0x00000000))
            {
                return PixelFormat::rg16_unorm;
            }

            if (ISBITMASK(0xffffffff, 0x00000000, 0x00000000, 0x00000000))
            {
                // Only 32-bit color channel format in D3D9 was R32F
                return PixelFormat::r32_float; // D3DX writes this out as a FourCC of 114
            }
            break;

        case 24:
            // No 24bpp DXGI formats aka D3DFMT_R8G8B8
            break;

        case 16:
            if (ISBITMASK(0x7c00, 0x03e0, 0x001f, 0x8000))
            {
                return PixelFormat::bgr5a1_unorm;
            }
            if (ISBITMASK(0xf800, 0x07e0, 0x001f, 0x0000))
            {
                return PixelFormat::b5g6r5_unorm;
            }

            // No DXGI format maps to ISBITMASK(0x7c00,0x03e0,0x001f,0x0000) aka D3DFMT_X1R5G5B5

            if (ISBITMASK(0x0f00, 0x00f0, 0x000f, 0xf000))
            {
                return PixelFormat::bgra4_unorm;
            }

            // No DXGI format maps to ISBITMASK(0x0f00,0x00f0,0x000f,0x0000) aka D3DFMT_X4R4G4B4

            // No 3:3:2, 3:3:2:8, or paletted DXGI formats aka D3DFMT_A8R3G3B2, D3DFMT_R3G3B2, D3DFMT_P8, D3DFMT_A8P8, etc.
            break;
        }
    }
    else if (ddpf.flags & DDS_LUMINANCE)
    {
        if (8 == ddpf.RGBBitCount)
        {
            if (ISBITMASK(0x000000ff, 0x00000000, 0x00000000, 0x00000000))
            {
                return PixelFormat::r8_unorm; // D3DX10/11 writes this out as DX10 extension
            }

            // No DXGI format maps to ISBITMASK(0x0f,0x00,0x00,0xf0) aka D3DFMT_A4L4

            if (ISBITMASK(0x000000ff, 0x00000000, 0x00000000, 0x0000ff00))
            {
                return PixelFormat::rg8_unorm; // Some DDS writers assume the bitcount should be 8 instead of 16
            }
        }

        if (16 == ddpf.RGBBitCount)
        {
            if (ISBITMASK(0x0000ffff, 0x00000000, 0x00000000, 0x00000000))
            {
                return PixelFormat::r16_unorm; // D3DX10/11 writes this out as DX10 extension
            }
            if (ISBITMASK(0x000000ff, 0x00000000, 0x00000000, 0x0000ff00))
            {
                return PixelFormat::rg8_unorm; // D3DX10/11 writes this out as DX10 extension
            }
        }
    }
    else if (ddpf.flags & DDS_ALPHA)
    {
        if (8 == ddpf.RGBBitCount)
        {
            return PixelFormat::a8_unorm;
        }
    }
    else if (ddpf.flags & DDS_BUMPDUDV)
    {
        if (16 == ddpf.RGBBitCount)
        {
            if (ISBITMASK(0x00ff, 0xff00, 0x0000, 0x0000))
            {
                return PixelFormat::rg8_snorm; // D3DX10/11 writes this out as DX10 extension
            }
        }

        if (32 == ddpf.RGBBitCount)
        {
            if (ISBITMASK(0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000))
            {
                return PixelFormat::rgba8_snorm; // D3DX10/11 writes this out as DX10 extension
            }
            if (ISBITMASK(0x0000ffff, 0xffff0000, 0x00000000, 0x00000000))
            {
                return PixelFormat::rg16_snorm; // D3DX10/11 writes this out as DX10 extension
            }

            // No DXGI format maps to ISBITMASK(0x3ff00000, 0x000ffc00, 0x000003ff, 0xc0000000) aka D3DFMT_A2W10V10U10
        }
    }
    else if (ddpf.flags & DDS_FOURCC)
    {
        if (MAKEFOURCC('D', 'X', 'T', '1') == ddpf.fourCC)
        {
            return PixelFormat::bc1_unorm;
        }
        if (MAKEFOURCC('D', 'X', 'T', '3') == ddpf.fourCC)
        {
            return PixelFormat::bc2_unorm;
        }
        if (MAKEFOURCC('D', 'X', 'T', '5') == ddpf.fourCC)
        {
            return PixelFormat::bc3_unorm;
        }

        // While pre-multiplied alpha isn't directly supported by the DXGI formats,
        // they are basically the same as these BC formats so they can be mapped
        if (MAKEFOURCC('D', 'X', 'T', '2') == ddpf.fourCC)
        {
            return PixelFormat::bc2_unorm;
        }
        if (MAKEFOURCC('D', 'X', 'T', '4') == ddpf.fourCC)
        {
            return PixelFormat::bc3_unorm;
        }

        if (MAKEFOURCC('A', 'T', 'I', '1') == ddpf.fourCC)
        {
            return PixelFormat::bc4_unorm;
        }
        if (MAKEFOURCC('B', 'C', '4', 'U') == ddpf.fourCC)
        {
            return PixelFormat::bc4_unorm;
        }
        if (MAKEFOURCC('B', 'C', '4', 'S') == ddpf.fourCC)
        {
            return PixelFormat::bc4_snorm;
        }

        if (MAKEFOURCC('A', 'T', 'I', '2') == ddpf.fourCC)
        {
            return PixelFormat::bc5_unorm;
        }
        if (MAKEFOURCC('B', 'C', '5', 'U') == ddpf.fourCC)
        {
            return PixelFormat::bc5_unorm;
        }
        if (MAKEFOURCC('B', 'C', '5', 'S') == ddpf.fourCC)
        {
            return PixelFormat::bc5_snorm;
        }

        // BC6H and BC7 are written using the "DX10" extended header

        if (MAKEFOURCC('R', 'G', 'B', 'G') == ddpf.fourCC)
        {
            return PixelFormat::rgbg8_unorm;
        }
        if (MAKEFOURCC('G', 'R', 'G', 'B') == ddpf.fourCC)
        {
            return PixelFormat::grgb8_unorm;
        }

        if (MAKEFOURCC('Y', 'U', 'Y', '2') == ddpf.fourCC)
        {
            return PixelFormat::yuy2;
        }

        // Check for D3DFORMAT enums being set here
        switch (ddpf.fourCC)
        {
        case 36: // D3DFMT_A16B16G16R16
            return PixelFormat::rgba16_unorm;

        case 110: // D3DFMT_Q16W16V16U16
            return PixelFormat::rgba16_snorm;

        case 111: // D3DFMT_R16F
            return PixelFormat::r16_float;

        case 112: // D3DFMT_G16R16F
            return PixelFormat::rg16_float;

        case 113: // D3DFMT_A16B16G16R16F
            return PixelFormat::rgba16_float;

        case 114: // D3DFMT_R32F
            return PixelFormat::r32_float;

        case 115: // D3DFMT_G32R32F
            return PixelFormat::rg32_float;

        case 116: // D3DFMT_A32B32G32R32F
            return PixelFormat::rgba32_float;
        }
    }

    return PixelFormat::unknown;
}

//--------------------------------------------------------------------------------------
DDS_ALPHA_MODE GetAlphaMode(_In_ const DDS_HEADER* header)
{
    if (header->ddspf.flags & DDS_FOURCC)
    {
        if (MAKEFOURCC('D', 'X', '1', '0') == header->ddspf.fourCC)
        {
            auto d3d10ext = reinterpret_cast<const DDS_HEADER_DXT10*>((const char*)header + sizeof(DDS_HEADER));
            auto mode = static_cast<DDS_ALPHA_MODE>(d3d10ext->miscFlags2 & DDS_MISC_FLAGS2_ALPHA_MODE_MASK);
            switch (mode)
            {
            case DDS_ALPHA_MODE_STRAIGHT:
            case DDS_ALPHA_MODE_PREMULTIPLIED:
            case DDS_ALPHA_MODE_OPAQUE:
            case DDS_ALPHA_MODE_CUSTOM:
                return mode;
            }
        }
        else if ((MAKEFOURCC('D', 'X', 'T', '2') == header->ddspf.fourCC)
            || (MAKEFOURCC('D', 'X', 'T', '4') == header->ddspf.fourCC))
        {
            return DDS_ALPHA_MODE_PREMULTIPLIED;
        }
    }

    return DDS_ALPHA_MODE_UNKNOWN;
}

}