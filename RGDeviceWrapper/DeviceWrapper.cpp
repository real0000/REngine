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