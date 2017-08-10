// D3DDevice.cpp
//
// 2015/11/12 Ian Wu/Real0000
//

#include "RGDeviceWrapper.h"
#include <thread>

namespace R
{

#pragma region Dircet3DDevice
//
// Dircet3DDevice
//
Dircet3DDevice::Dircet3DDevice()
{
}

Dircet3DDevice::~Dircet3DDevice()
{
}
	
unsigned int Dircet3DDevice::getPixelFormat(PixelFormat::Key a_Key)
{
	switch( a_Key )
	{
		case PixelFormat::unknown:					return DXGI_FORMAT_UNKNOWN;
		case PixelFormat::rgba32_typeless:			return DXGI_FORMAT_R32G32B32A32_TYPELESS;
		case PixelFormat::rgba32_float:				return DXGI_FORMAT_R32G32B32A32_FLOAT;
		case PixelFormat::rgba32_uint:				return DXGI_FORMAT_R32G32B32A32_UINT;
		case PixelFormat::rgba32_sint:				return DXGI_FORMAT_R32G32B32A32_SINT;
		case PixelFormat::rgb32_typeless:			return DXGI_FORMAT_R32G32B32_TYPELESS;
		case PixelFormat::rgb32_float:				return DXGI_FORMAT_R32G32B32_FLOAT;
		case PixelFormat::rgb32_uint:				return DXGI_FORMAT_R32G32B32_UINT;
		case PixelFormat::rgb32_sint:				return DXGI_FORMAT_R32G32B32_SINT;
		case PixelFormat::rgba16_typeless:			return DXGI_FORMAT_R16G16B16A16_TYPELESS;
		case PixelFormat::rgba16_float:				return DXGI_FORMAT_R16G16B16A16_FLOAT;
		case PixelFormat::rgba16_unorm:				return DXGI_FORMAT_R16G16B16A16_UNORM;
		case PixelFormat::rgba16_uint:				return DXGI_FORMAT_R16G16B16A16_UINT;
		case PixelFormat::rgba16_snorm:				return DXGI_FORMAT_R16G16B16A16_SNORM;
		case PixelFormat::rgba16_sint:				return DXGI_FORMAT_R16G16B16A16_SINT;
		case PixelFormat::rg32_typeless:			return DXGI_FORMAT_R32G32_TYPELESS;
		case PixelFormat::rg32_float:				return DXGI_FORMAT_R32G32_FLOAT;
		case PixelFormat::rg32_uint:				return DXGI_FORMAT_R32G32_UINT;
		case PixelFormat::rg32_sint:				return DXGI_FORMAT_R32G32_SINT;
		case PixelFormat::r32g8x24_typeless:		return DXGI_FORMAT_R32G8X24_TYPELESS;
		case PixelFormat::d32_float_s8x24_uint:		return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
		case PixelFormat::r32_float_x8x24_typeless:	return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
		case PixelFormat::x32_typeless_g8x24_uint:	return DXGI_FORMAT_X32_TYPELESS_G8X24_UINT;
		case PixelFormat::rgb10a2_typeless:			return DXGI_FORMAT_R10G10B10A2_TYPELESS;
		case PixelFormat::rgb10a2_unorm:			return DXGI_FORMAT_R10G10B10A2_UNORM;
		case PixelFormat::rgb10a2_uint:				return DXGI_FORMAT_R10G10B10A2_UINT;
		case PixelFormat::r11g11b10_float:			return DXGI_FORMAT_R11G11B10_FLOAT;
		case PixelFormat::rgba8_typeless:			return DXGI_FORMAT_R8G8B8A8_TYPELESS;
		case PixelFormat::rgba8_unorm:				return DXGI_FORMAT_R8G8B8A8_UNORM;
		case PixelFormat::rgba8_unorm_srgb:			return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		case PixelFormat::rgba8_uint:				return DXGI_FORMAT_R8G8B8A8_UINT;
		case PixelFormat::rgba8_snorm:				return DXGI_FORMAT_R8G8B8A8_SNORM;
		case PixelFormat::rgba8_sint:				return DXGI_FORMAT_R8G8B8A8_SINT;
		case PixelFormat::rg16_typeless:			return DXGI_FORMAT_R16G16_TYPELESS;
		case PixelFormat::rg16_float:				return DXGI_FORMAT_R16G16_FLOAT;
		case PixelFormat::rg16_unorm:				return DXGI_FORMAT_R16G16_UNORM;
		case PixelFormat::rg16_uint:				return DXGI_FORMAT_R16G16_UINT;
		case PixelFormat::rg16_snorm:				return DXGI_FORMAT_R16G16_SNORM;
		case PixelFormat::rg16_sint:				return DXGI_FORMAT_R16G16_SINT;
		case PixelFormat::r32_typeless:				return DXGI_FORMAT_R32_TYPELESS;
		case PixelFormat::d32_float:				return DXGI_FORMAT_D32_FLOAT;
		case PixelFormat::r32_float:				return DXGI_FORMAT_R32_FLOAT;
		case PixelFormat::r32_uint:					return DXGI_FORMAT_R32_UINT;
		case PixelFormat::r32_sint:					return DXGI_FORMAT_R32_SINT;
		case PixelFormat::r24g8_typeless:			return DXGI_FORMAT_R24G8_TYPELESS;
		case PixelFormat::d24_unorm_s8_uint:		return DXGI_FORMAT_D24_UNORM_S8_UINT;
		case PixelFormat::r24_unorm_x8_typeless:	return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		case PixelFormat::x24_typeless_g8_uint:		return DXGI_FORMAT_X24_TYPELESS_G8_UINT;
		case PixelFormat::rg8_typeless:				return DXGI_FORMAT_R8G8_TYPELESS;
		case PixelFormat::rg8_unorm:				return DXGI_FORMAT_R8G8_UNORM;
		case PixelFormat::rg8_uint:					return DXGI_FORMAT_R8G8_UINT;
		case PixelFormat::rg8_snorm:				return DXGI_FORMAT_R8G8_SNORM;
		case PixelFormat::rg8_sint:					return DXGI_FORMAT_R8G8_SINT;
		case PixelFormat::r16_typeless:				return DXGI_FORMAT_R16_TYPELESS;
		case PixelFormat::r16_float:				return DXGI_FORMAT_R16_FLOAT;
		case PixelFormat::d16_unorm:				return DXGI_FORMAT_D16_UNORM;
		case PixelFormat::r16_unorm:				return DXGI_FORMAT_R16_UNORM;
		case PixelFormat::r16_uint:					return DXGI_FORMAT_R16_UINT;
		case PixelFormat::r16_snorm:				return DXGI_FORMAT_R16_SNORM;
		case PixelFormat::r16_sint:					return DXGI_FORMAT_R16_SINT;
		case PixelFormat::r8_typeless:				return DXGI_FORMAT_R8_TYPELESS;
		case PixelFormat::r8_unorm:					return DXGI_FORMAT_R8_UNORM;
		case PixelFormat::r8_uint:					return DXGI_FORMAT_R8_UINT;
		case PixelFormat::r8_snorm:					return DXGI_FORMAT_R8_SNORM;
		case PixelFormat::r8_sint:					return DXGI_FORMAT_R8_SINT;
		case PixelFormat::a8_unorm:					return DXGI_FORMAT_A8_UNORM;
		case PixelFormat::r1_unorm:					return DXGI_FORMAT_R1_UNORM;
		case PixelFormat::rgb9e5:					return DXGI_FORMAT_R9G9B9E5_SHAREDEXP;
		case PixelFormat::rgbg8_unorm:				return DXGI_FORMAT_R8G8_B8G8_UNORM;
		case PixelFormat::grgb8_unorm:				return DXGI_FORMAT_G8R8_G8B8_UNORM;
		case PixelFormat::bc1_typeless:				return DXGI_FORMAT_BC1_TYPELESS;
		case PixelFormat::bc1_unorm:				return DXGI_FORMAT_BC1_UNORM;
		case PixelFormat::bc1_unorm_srgb:			return DXGI_FORMAT_BC1_UNORM_SRGB;
		case PixelFormat::bc2_typeless:				return DXGI_FORMAT_BC2_TYPELESS;
		case PixelFormat::bc2_unorm:				return DXGI_FORMAT_BC2_UNORM;
		case PixelFormat::bc2_unorm_srgb:			return DXGI_FORMAT_BC2_UNORM_SRGB;
		case PixelFormat::bc3_typeless:				return DXGI_FORMAT_BC3_TYPELESS;
		case PixelFormat::bc3_unorm:				return DXGI_FORMAT_BC3_UNORM;
		case PixelFormat::bc3_unorm_srgb:			return DXGI_FORMAT_BC3_UNORM_SRGB;
		case PixelFormat::bc4_typeless:				return DXGI_FORMAT_BC4_TYPELESS;
		case PixelFormat::bc4_unorm:				return DXGI_FORMAT_BC4_UNORM;
		case PixelFormat::bc4_snorm:				return DXGI_FORMAT_BC4_SNORM;
		case PixelFormat::bc5_typeless:				return DXGI_FORMAT_BC5_TYPELESS;
		case PixelFormat::bc5_unorm:				return DXGI_FORMAT_BC5_UNORM;
		case PixelFormat::bc5_snorm:				return DXGI_FORMAT_BC5_SNORM;
		case PixelFormat::b5g6r5_unorm:				return DXGI_FORMAT_B5G6R5_UNORM;
		case PixelFormat::bgr5a1_unorm:				return DXGI_FORMAT_B5G5R5A1_UNORM;
		case PixelFormat::bgra8_unorm:				return DXGI_FORMAT_B8G8R8A8_UNORM;
		case PixelFormat::bgrx8_unorm:				return DXGI_FORMAT_B8G8R8X8_UNORM;
		case PixelFormat::rgb10_xr_bias_a2_unorm:	return DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM;
		case PixelFormat::bgra8_typeless:			return DXGI_FORMAT_B8G8R8A8_TYPELESS;
		case PixelFormat::bgra8_unorm_srgb:			return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
		case PixelFormat::bgrx8_typeless:			return DXGI_FORMAT_B8G8R8X8_TYPELESS;
		case PixelFormat::bgrx8_unorm_srgb:			return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;
		case PixelFormat::bc6h_typeless:			return DXGI_FORMAT_BC6H_TYPELESS;
		case PixelFormat::bc6h_uf16:				return DXGI_FORMAT_BC6H_UF16;
		case PixelFormat::bc6h_sf16:				return DXGI_FORMAT_BC6H_SF16;
		case PixelFormat::bc7_typeless:				return DXGI_FORMAT_BC7_TYPELESS;
		case PixelFormat::bc7_unorm:				return DXGI_FORMAT_BC7_UNORM;
		case PixelFormat::bc7_unorm_srgb:			return DXGI_FORMAT_BC7_UNORM_SRGB;
		case PixelFormat::ayuv:						return DXGI_FORMAT_AYUV;
		case PixelFormat::y410:						return DXGI_FORMAT_Y410;
		case PixelFormat::y416:						return DXGI_FORMAT_Y416;
		case PixelFormat::nv12:						return DXGI_FORMAT_NV12;
		case PixelFormat::p010:						return DXGI_FORMAT_P010;
		case PixelFormat::p016:						return DXGI_FORMAT_P016;
		case PixelFormat::opaque420:				return DXGI_FORMAT_420_OPAQUE;
		case PixelFormat::yuy2:						return DXGI_FORMAT_YUY2;
		case PixelFormat::y210:						return DXGI_FORMAT_Y210;
		case PixelFormat::y216:						return DXGI_FORMAT_Y216;
		case PixelFormat::nv11:						return DXGI_FORMAT_NV11;
		case PixelFormat::ai44:						return DXGI_FORMAT_AI44;
		case PixelFormat::ia44:						return DXGI_FORMAT_IA44;
		case PixelFormat::p8:						return DXGI_FORMAT_P8;
		case PixelFormat::ap8:						return DXGI_FORMAT_A8P8;
		case PixelFormat::bgra4_unorm:				return DXGI_FORMAT_B4G4R4A4_UNORM;
		case PixelFormat::p208:						return DXGI_FORMAT_P208;
		case PixelFormat::v208:						return DXGI_FORMAT_V208;
		case PixelFormat::v408:						return DXGI_FORMAT_V408;
		case PixelFormat::uint:						return DXGI_FORMAT_FORCE_UINT;
	}

	assert(false && "invalid type");
	return 0xffffffff;
}

unsigned int Dircet3DDevice::getParamAlignment(ShaderParamType::Key a_Key)
{
	switch( a_Key )
	{
		case ShaderParamType::int1:		return sizeof(int);
		case ShaderParamType::int2:		return sizeof(int) * 2;
		case ShaderParamType::int3:		return sizeof(int) * 4;
		case ShaderParamType::int4:		return sizeof(int) * 4;
		case ShaderParamType::float1:	return sizeof(float);
		case ShaderParamType::float2:	return sizeof(float) * 2;
		case ShaderParamType::float3:	return sizeof(float) * 4;
		case ShaderParamType::float4:	return sizeof(float) * 4;
		case ShaderParamType::float3x3:	return sizeof(float) * 4;
		case ShaderParamType::float4x4:	return sizeof(float) * 4;
		default:break;
	}

	assert(false && "invalid param type");
	return 0;
}
#pragma endregion

}