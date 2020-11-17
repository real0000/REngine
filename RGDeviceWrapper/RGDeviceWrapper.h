// RGDeviceWrapper.h
//
// 2015/11/12 Ian Wu/Real0000
//

#ifndef _R_GDEVICE_WRAPPER_H_
#define _R_GDEVICE_WRAPPER_H_

#include "CommonUtil.h"

namespace R
{

#define NUM_BACKBUFFER 2

enum
{
	VTXSLOT_POSITION = 0,//in vec3 a_Position;
	VTXSLOT_TEXCOORD01,//in vec4 a_UVs;
	VTXSLOT_TEXCOORD23,
	VTXSLOT_TEXCOORD45,
	VTXSLOT_TEXCOORD67,
	VTXSLOT_NORMAL,//in vec3 a_Normal;
	VTXSLOT_TANGENT,//in vec3 a_Tangent;
	VTXSLOT_BINORMAL,//in vec3 a_Binormal;
	VTXSLOT_BONE,//in ivec4 a_BoneID;
	VTXSLOT_WEIGHT,//in vec4 a_BoneWeight; 
	VTXSLOT_COLOR,//in unsigned int a_Color;

	VTXSLOT_COUNT,
	VTXSLOT_INSTANCE = VTXSLOT_COUNT,
};

enum
{
	VTXFLAG_POSITION	= 1 << VTXSLOT_POSITION,
	VTXFLAG_TEXCOORD01	= 1 << VTXSLOT_TEXCOORD01,
	VTXFLAG_TEXCOORD23	= 1 << VTXSLOT_TEXCOORD23,
	VTXFLAG_TEXCOORD45	= 1 << VTXSLOT_TEXCOORD45,
	VTXFLAG_TEXCOORD67	= 1 << VTXSLOT_TEXCOORD67,
	VTXFLAG_NORMAL		= 1 << VTXSLOT_NORMAL,
	VTXFLAG_TANGENT		= 1 << VTXSLOT_TANGENT,
	VTXFLAG_BINORMAL	= 1 << VTXSLOT_BINORMAL,
	VTXFLAG_BONE		= 1 << VTXSLOT_BONE,
	VTXFLAG_WEIGHT		= 1 << VTXSLOT_WEIGHT,
	VTXFLAG_COLOR		= 1 << VTXSLOT_COLOR,

	VTXFLAG_USE_WORLD_MAT	= 0x10000,
	VTXFLAG_USE_SKIN		= 0x20000,
	VTXFLAG_USE_NORMAL_TEXTURE = 0x40000,
	VTXFLAG_WITHOUT_VP_MAT	= 0x80000,
	VTXFLAG_USE_TESSELLATION= 0x100000
};
#define GENERAL_VTX_FLAG (VTXFLAG_POSITION | VTXFLAG_TEXCOORD01 | VTXFLAG_NORMAL | VTXFLAG_TANGENT | VTXFLAG_BINORMAL | VTXFLAG_USE_WORLD_MAT)

STRING_ENUM_CLASS(ShaderParamType,
	int1,
	int2,
	int3,
	int4,
	float1,
	float2,
	float3,
	float4,
	float3x3,
	float4x4,
	double1)

STRING_ENUM_CLASS(BlendKey,
	zero,
	one,
	src_color,
	inv_src_color,
	src_alpha,
	inv_src_alpha,
	dst_alpha,
	inv_dst_alpha,
	dst_color,
	inv_dst_color,
	src_alpha_saturate,
	blend_factor,
	inv_blend_factor,
	src1_color,
	inv_src1_color,
	src1_alpha,
	inv_src1_alpha) 
	
STRING_ENUM_CLASS(BlendOP, add, sub, rev_sub, min, max)

STRING_ENUM_CLASS(BlendLogic,//add '_' to avoid keyword
	clear_,
	set_,
	copy_,
	copy_inv_,
	none_,
	inv_,
	and_,
	nand_,
	or_,
	nor_,
	xor_,
	eq_,
	and_rev_,
	and_inv_,
	or_rev_,
	or_inv_)

STRING_ENUM_CLASS(CullMode,
	none,
	front,
	back)

STRING_ENUM_CLASS(CompareFunc,
	never,
	less,
	equal,
	less_equal,
	greater,
	not_equal,
	greater_equal,
	always)

STRING_ENUM_CLASS(StencilOP,
	keep,
	zero,
	replace,
	inc_saturate,
	dec_saturate,
	invert,
	inc,
	dec)

STRING_ENUM_CLASS(TopologyType, point, line, triangle, patch)

STRING_ENUM_CLASS(Topology,
	undefined,
	point_list,
	line_list,
	line_strip,
	triangle_list,
	triangle_strip,
	line_list_adj,
	line_strip_adj,
	triangle_list_adj,
	triangle_strip_adj,
	point_patch_list_1,
	point_patch_list_2,
	point_patch_list_3,
	point_patch_list_4,
	point_patch_list_5,
	point_patch_list_6,
	point_patch_list_7,
	point_patch_list_8,
	point_patch_list_9,
	point_patch_list_10,
	point_patch_list_11,
	point_patch_list_12,
	point_patch_list_13,
	point_patch_list_14,
	point_patch_list_15,
	point_patch_list_16,
	point_patch_list_17,
	point_patch_list_18,
	point_patch_list_19,
	point_patch_list_20,
	point_patch_list_21,
	point_patch_list_22,
	point_patch_list_23,
	point_patch_list_24,
	point_patch_list_25,
	point_patch_list_26,
	point_patch_list_27,
	point_patch_list_28,
	point_patch_list_29,
	point_patch_list_30,
	point_patch_list_31,
	point_patch_list_32)

STRING_ENUM_CLASS(Filter,
	min_mag_mip_point,
    min_mag_point_mip_linear,
    min_point_mag_linear_mip_point,
    min_point_mag_mip_linear,
    min_linear_mag_mip_point,
    min_linear_mag_point_mip_linear,
    min_mag_linear_mip_point,
    min_mag_mip_linear,
    anisotropic,
    comparison_min_mag_mip_point,
    comparison_min_mag_point_mip_linear,
    comparison_min_point_mag_linear_mip_point,
    comparison_min_point_mag_mip_linear,
    comparison_min_linear_mag_mip_point,
    comparison_min_linear_mag_point_mip_linear,
    comparison_min_mag_linear_mip_point,
    comparison_min_mag_mip_linear,
    comparison_anisotropic,
    minimum_min_mag_mip_point,
    minimum_min_mag_point_mip_linear,
    minimum_min_point_mag_linear_mip_point,
    minimum_min_point_mag_mip_linear,
    minimum_min_linear_mag_mip_point,
    minimum_min_linear_mag_point_mip_linear,
    minimum_min_mag_linear_mip_point,
    minimum_min_mag_mip_linear,
    minimum_anisotropic,
    maximum_min_mag_mip_point,
    maximum_min_mag_point_mip_linear,
    maximum_min_point_mag_linear_mip_point,
    maximum_min_point_mag_mip_linear,
    maximum_min_linear_mag_mip_point,
    maximum_min_linear_mag_point_mip_linear,
    maximum_min_mag_linear_mip_point,
    maximum_min_mag_mip_linear,
    maximum_anisotropic)

STRING_ENUM_CLASS(AddressMode,
	wrap,
    mirror,
    clamp,
    border,
    mirror_once)

const wxString c_ShaderPath[] = {
	wxT("Shaders/"), 
	wxT("Shaders/Lightmap/")
	wxT("Shaders/Shadowmap/")};
const unsigned int c_NumShaderPath = sizeof(c_ShaderPath) / sizeof(wxString);
#define SAHDER_BLOCK_DEFINE_FILE "BlockDefine.xml"
#define STANDARD_TEXTURE_BASECOLOR	"BaseColor"
#define STANDARD_TEXTURE_NORMAL		"Normal"
#define STANDARD_TEXTURE_SURFACE	"Surface"
#define STANDARD_TRANSFORM_NORMAL	"NormalTransition"
#define STANDARD_TRANSFORM_SKIN		"SkinTransition"
STRING_ENUM_CLASS(DefaultPrograms,

	// compute
	GenerateMipmap2D,
	GenerateMipmap3D,
	TiledLightIntersection,
	GenerateMinmaxDepth,

	// draw
	Copy,
	CopyDepth,
	CopyFrame,
	TextureOnly,
	TiledDeferredLighting,

	// shadow map
	DirShadowMap,
	OmniShadowMap,
	SpotShadowMap,

	// Lightmap
	RayIntersection,
	RayScatter,

	num_default_program)

struct IndirectDrawData
{
	unsigned int m_IndexCount;
	unsigned int m_StartIndex;
	unsigned int m_BaseVertex;
	unsigned int m_StartInstance;
	unsigned int m_InstanceCount;
};

std::string convertParamValue(ShaderParamType::Key a_Type, char *a_pSrc);
void parseShaderParamValue(ShaderParamType::Key a_Type, std::string a_Src, char *a_pDst);

}

#define TEXTURE_ARRAY_SIZE 16
#define DEFAULT_D3D_THREAD_COUNT 4
#define MIN_TEXTURE_SIZE 8

#include "DeviceWrapper.h"
#include "DrawBuffer.h"
#include "ShaderBase.h"

#ifdef WIN32
#	include "D3D/D3DDevice.h"
#	include "D3D/D3D11Device.h"
#	include "D3D/D3D12Device.h"
#	include "D3D/HLSLShader.h"
#endif

#if (defined MAC_OSX || defined IOS)
	// include metal lib
#endif

#endif