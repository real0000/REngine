// DeviceManager.cpp
//
// 2015/08/05 Ian Wu/Real0000
//
#include "RGDeviceWrapper.h"

namespace R
{

STRING_ENUM_CLASS_INST(ShaderParamType)
STRING_ENUM_CLASS_INST(BlendKey)
STRING_ENUM_CLASS_INST(BlendOP)
STRING_ENUM_CLASS_INST(BlendLogic)
STRING_ENUM_CLASS_INST(CullMode)
STRING_ENUM_CLASS_INST(CompareFunc)
STRING_ENUM_CLASS_INST(StencilOP)
STRING_ENUM_CLASS_INST(TopologyType)
STRING_ENUM_CLASS_INST(Topology)
STRING_ENUM_CLASS_INST(DefaultPrograms)

#pragma region GraphicCommander
//
// GraphicCommander
//
GraphicCommander::GraphicCommander()
{
}

GraphicCommander::~GraphicCommander()
{
}

void GraphicCommander::useProgram(unsigned int a_Key)
{
	ShaderProgram *l_pProgram = ProgramManager::singleton().getData(a_Key);
	useProgram(l_pProgram);
}

void GraphicCommander::setRenderTarget(int a_DSVHandle, unsigned int a_NumRT, ...)
{
	std::vector<int> l_RtvHandleList;
	l_RtvHandleList.resize(a_NumRT);
	{
		va_list l_Arglist;
		va_start(l_Arglist, a_NumRT);
		for( unsigned int i=0 ; i<a_NumRT ; ++i ) l_RtvHandleList[i] = va_arg(l_Arglist, int);
		va_end(l_Arglist);
	}
	setRenderTarget(a_DSVHandle, l_RtvHandleList);
}
#pragma endregion

#pragma region GraphicCanvas
//
// GraphicCanvas
//
GraphicCanvas::GraphicCanvas()
	: m_pRefCommander(nullptr)
	, m_RenderFunc(nullptr)
{
}

GraphicCanvas::~GraphicCanvas()
{
}

void GraphicCanvas::update(float a_Delta)
{
	if( m_RenderFunc ) m_RenderFunc(a_Delta);
}

void GraphicCanvas::init(WXWidget a_Wnd, glm::ivec2 a_Size, bool a_bFullScr)
{
	if( nullptr == m_pRefCommander ) return ;
	m_pRefCommander->init(a_Wnd, a_Size, a_bFullScr);
}

void GraphicCanvas::resize(glm::ivec2 a_Size, bool a_bFullScr)
{
	if( nullptr == m_pRefCommander ) return ;
	m_pRefCommander->resize(a_Size, a_bFullScr);
}

void GraphicCanvas::setCommander(GraphicCommander *a_pCommander)
{
	if( nullptr == m_pRefCommander ) m_pRefCommander = a_pCommander;
}

void GraphicCanvas::setRenderFunction(std::function<void(float)> a_Func)
{
	m_RenderFunc = a_Func;
}
#pragma endregion

#pragma region GraphicDevice
//
// GraphicDevice
//
unsigned int GraphicDevice::getPixelSize(PixelFormat::Key a_Key)
{
	assert(PixelFormat::unknown != a_Key);
	switch( a_Key )
	{
		case PixelFormat::rgba32_typeless:
		case PixelFormat::rgba32_float:
		case PixelFormat::rgba32_uint:
		case PixelFormat::rgba32_sint:				return 16;
		case PixelFormat::rgb32_typeless:
		case PixelFormat::rgb32_float:
		case PixelFormat::rgb32_uint:
		case PixelFormat::rgb32_sint:				return 12;
		case PixelFormat::rgba16_typeless:
		case PixelFormat::rgba16_float:
		case PixelFormat::rgba16_unorm:
		case PixelFormat::rgba16_uint:
		case PixelFormat::rgba16_snorm:
		case PixelFormat::rgba16_sint:
		case PixelFormat::rg32_typeless:
		case PixelFormat::rg32_float:
		case PixelFormat::rg32_uint:
		case PixelFormat::rg32_sint:
		case PixelFormat::r32g8x24_typeless: 
		case PixelFormat::d32_float_s8x24_uint:
		case PixelFormat::r32_float_x8x24_typeless:
		case PixelFormat::x32_typeless_g8x24_uint:	return 8;
		case PixelFormat::rgb10a2_typeless:
		case PixelFormat::rgb10a2_unorm:
		case PixelFormat::rgb10a2_uint:
		case PixelFormat::r11g11b10_float:
		case PixelFormat::rgba8_typeless:
		case PixelFormat::rgba8_unorm:
		case PixelFormat::rgba8_unorm_srgb:
		case PixelFormat::rgba8_uint:
		case PixelFormat::rgba8_snorm:
		case PixelFormat::rgba8_sint:
		case PixelFormat::rg16_typeless:
		case PixelFormat::rg16_float:
		case PixelFormat::rg16_unorm:
		case PixelFormat::rg16_uint:
		case PixelFormat::rg16_snorm:
		case PixelFormat::rg16_sint:
		case PixelFormat::r32_typeless:
		case PixelFormat::d32_float:
		case PixelFormat::r32_float:
		case PixelFormat::r32_uint:
		case PixelFormat::r32_sint:
		case PixelFormat::r24g8_typeless:
		case PixelFormat::d24_unorm_s8_uint:
		case PixelFormat::r24_unorm_x8_typeless:
		case PixelFormat::x24_typeless_g8_uint:		return 4;
		case PixelFormat::rg8_typeless:
		case PixelFormat::rg8_unorm:
		case PixelFormat::rg8_uint:
		case PixelFormat::rg8_snorm:
		case PixelFormat::rg8_sint:
		case PixelFormat::r16_typeless:
		case PixelFormat::r16_float:
		case PixelFormat::d16_unorm:
		case PixelFormat::r16_unorm:
		case PixelFormat::r16_uint:
		case PixelFormat::r16_snorm:
		case PixelFormat::r16_sint:					return 2;
		case PixelFormat::r8_typeless:
		case PixelFormat::r8_unorm:
		case PixelFormat::r8_uint:
		case PixelFormat::r8_snorm:
		case PixelFormat::r8_sint:
		case PixelFormat::a8_unorm:					return 1;
		case PixelFormat::r1_unorm:					return 1;//?
		case PixelFormat::rgb9e5:
		case PixelFormat::rgbg8_unorm:
		case PixelFormat::grgb8_unorm:				return 4;
		case PixelFormat::bc1_typeless:
		case PixelFormat::bc1_unorm:
		case PixelFormat::bc1_unorm_srgb:
		case PixelFormat::bc2_typeless:
		case PixelFormat::bc2_unorm:
		case PixelFormat::bc2_unorm_srgb:
		case PixelFormat::bc3_typeless:
		case PixelFormat::bc3_unorm:
		case PixelFormat::bc3_unorm_srgb:			return 2;
		case PixelFormat::bc4_typeless:
		case PixelFormat::bc4_unorm:
		case PixelFormat::bc4_snorm:				return 1;
		case PixelFormat::bc5_typeless:
		case PixelFormat::bc5_unorm:
		case PixelFormat::bc5_snorm:				return 2;
		case PixelFormat::b5g6r5_unorm:
		case PixelFormat::bgr5a1_unorm:				return 2;
		case PixelFormat::bgra8_unorm:
		case PixelFormat::bgrx8_unorm:
		case PixelFormat::rgb10_xr_bias_a2_unorm:
		case PixelFormat::bgra8_typeless:
		case PixelFormat::bgra8_unorm_srgb:
		case PixelFormat::bgrx8_typeless:
		case PixelFormat::bgrx8_unorm_srgb:			return 4;
		case PixelFormat::bc6h_typeless:
		case PixelFormat::bc6h_uf16:
		case PixelFormat::bc6h_sf16:				return 6;
		case PixelFormat::bc7_typeless:
		case PixelFormat::bc7_unorm:
		case PixelFormat::bc7_unorm_srgb:			return 4;//?
		case PixelFormat::ayuv:
		case PixelFormat::y410:						return 4;
		case PixelFormat::y416:						return 8;
		case PixelFormat::nv12:						return 1;
		case PixelFormat::p010:
		case PixelFormat::p016:						return 2;
		case PixelFormat::opaque420:				return 4;
		case PixelFormat::yuy2:						return 4;
		case PixelFormat::y210:						return 8;
		case PixelFormat::y216:						return 8;
		case PixelFormat::nv11:						return 1;
		case PixelFormat::ai44:
		case PixelFormat::ia44:						return 1;// 0.5?
		case PixelFormat::p8:						return 1;
		case PixelFormat::ap8:
		case PixelFormat::bgra4_unorm:				return 2;
		case PixelFormat::p208:
		case PixelFormat::v208:
		case PixelFormat::v408:						return 1;
		case PixelFormat::uint:						return 4;
		default:break;
	}

	assert(false && "invalid key");
	return 0;
}

unsigned int GraphicDevice::getVertexSlotStride(unsigned int a_Type)
{
	unsigned int l_Res = 0;
	switch( a_Type )
	{
		case VTXSLOT_POSITION:	l_Res = sizeof(glm::vec3); break;
		case VTXSLOT_TEXCOORD01:
		case VTXSLOT_TEXCOORD23:
		case VTXSLOT_TEXCOORD45:
		case VTXSLOT_TEXCOORD67:l_Res = sizeof(glm::vec4); break;
		case VTXSLOT_NORMAL:	l_Res = sizeof(glm::vec3); break;
		case VTXSLOT_TANGENT:	l_Res = sizeof(glm::vec3); break;
		case VTXSLOT_BINORMAL:	l_Res = sizeof(glm::vec3); break;
		case VTXSLOT_BONE:		l_Res = sizeof(glm::ivec4); break;
		case VTXSLOT_WEIGHT:	l_Res = sizeof(glm::vec4); break;
		case VTXSLOT_COLOR:		l_Res = sizeof(unsigned int); break;
		default:break;
	}
	assert(0 != l_Res);
	return l_Res;
}

unsigned int GraphicDevice::getParamAlignmentSize(ShaderParamType::Key a_Key)
{
	switch( a_Key )
	{
		case ShaderParamType::int1:		return sizeof(int);
		case ShaderParamType::int2:		return sizeof(int) * 2;
		case ShaderParamType::int3:		return sizeof(int) * 3;
		case ShaderParamType::int4:		return sizeof(int) * 4;
		case ShaderParamType::float1:	return sizeof(float);
		case ShaderParamType::float2:	return sizeof(float) * 2;
		case ShaderParamType::float3:	return sizeof(float) * 3;
		case ShaderParamType::float4:	return sizeof(float) * 4;
		case ShaderParamType::float3x3:	return sizeof(float) * 12;
		case ShaderParamType::float4x4:	return sizeof(float) * 16;
	}

	assert(false && "invalid argument type");
	return 0;
}
#pragma endregion

#pragma region GraphicDeviceManager
//
// GraphicDeviceManager
//
GraphicDeviceManager& GraphicDeviceManager::singleton()
{
	static GraphicDeviceManager s_Inst;
	return s_Inst;
}

GraphicDeviceManager::GraphicDeviceManager()
	: m_pDeviceInst(nullptr)
{
}

GraphicDeviceManager::~GraphicDeviceManager()
{
	SAFE_DELETE(m_pDeviceInst);
}
#pragma endregion

}