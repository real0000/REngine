// DeviceManager.cpp
//
// 2015/08/05 Ian Wu/Real0000
//
#include "RGDeviceWrapper.h"

#include "SDL.h"

namespace R
{

STRING_ENUM_CLASS_INST(AddressMode)
STRING_ENUM_CLASS_INST(BlendKey)
STRING_ENUM_CLASS_INST(BlendOP)
STRING_ENUM_CLASS_INST(BlendLogic)
STRING_ENUM_CLASS_INST(CullMode)
STRING_ENUM_CLASS_INST(CompareFunc)
STRING_ENUM_CLASS_INST(DefaultPrograms)
STRING_ENUM_CLASS_INST(Filter)
STRING_ENUM_CLASS_INST(ShaderParamType)
STRING_ENUM_CLASS_INST(StencilOP)
STRING_ENUM_CLASS_INST(TopologyType)
STRING_ENUM_CLASS_INST(Topology)

static SDL_Keycode convertKeyCode(wxKeyEvent &a_Event)
{
	int l_Code = a_Event.GetKeyCode();
	if( l_Code <= 255 )
	{
		if( l_Code >= 'A' && l_Code <= 'Z' ) l_Code -= 'A' - 'a';
		return (SDL_Keycode)l_Code;
	}
	switch( a_Event.GetKeyCode() )
	{
		case WXK_CANCEL:	return SDLK_CANCEL;
		case WXK_CLEAR:		return SDLK_CLEAR;
		case WXK_SHIFT:		return SDLK_LSHIFT;
		case WXK_ALT:		return SDLK_LALT;
#ifdef __APPLE__
		case WXK_RAW_CONTROL:
#endif
		case WXK_CONTROL:	return SDLK_LCTRL;
		case WXK_MENU:		return SDLK_MENU;
		case WXK_PAUSE:		return SDLK_PAUSE;
		case WXK_END:		return SDLK_END;
		case WXK_HOME:		return SDLK_HOME;
		case WXK_LEFT:		return SDLK_LEFT;
		case WXK_UP:		return SDLK_UP;
		case WXK_RIGHT:		return SDLK_RIGHT;
		case WXK_DOWN:		return SDLK_DOWN;
		case WXK_SELECT:	return SDLK_SELECT;
		case WXK_EXECUTE:	return SDLK_EXECUTE;
		case WXK_SNAPSHOT:	return SDLK_PRINTSCREEN;
		case WXK_INSERT:	return SDLK_INSERT;
		case WXK_HELP:		return SDLK_HELP;
		case WXK_NUMPAD0:	return SDLK_KP_0;
		case WXK_NUMPAD1:	return SDLK_KP_1;
		case WXK_NUMPAD2:	return SDLK_KP_2;
		case WXK_NUMPAD3:	return SDLK_KP_3;
		case WXK_NUMPAD4:	return SDLK_KP_4;
		case WXK_NUMPAD5:	return SDLK_KP_5;
		case WXK_NUMPAD6:	return SDLK_KP_6;
		case WXK_NUMPAD7:	return SDLK_KP_7;
		case WXK_NUMPAD8:	return SDLK_KP_8;
		case WXK_NUMPAD9:	return SDLK_KP_9;
		case WXK_MULTIPLY:	return SDLK_KP_MULTIPLY;
		case WXK_ADD:		return SDLK_KP_PLUS;
		case WXK_SEPARATOR: return SDLK_SEPARATOR;
		case WXK_SUBTRACT:	return SDLK_KP_MINUS;
		case WXK_DECIMAL:	return SDLK_KP_DECIMAL;
		case WXK_DIVIDE:	return SDLK_KP_DIVIDE;
		case WXK_F1:		return SDLK_F1;
		case WXK_F2:		return SDLK_F2;
		case WXK_F3:		return SDLK_F3;
		case WXK_F4:		return SDLK_F4;
		case WXK_F5:		return SDLK_F5;
		case WXK_F6:		return SDLK_F6;
		case WXK_F7:		return SDLK_F7;
		case WXK_F8:		return SDLK_F8;
		case WXK_F9:		return SDLK_F9;
		case WXK_F10:		return SDLK_F10;
		case WXK_F11:		return SDLK_F11;
		case WXK_F12:		return SDLK_F12;
		case WXK_NUMLOCK:	return SDLK_NUMLOCKCLEAR;
		case WXK_SCROLL:	return SDLK_SCROLLLOCK;
		case WXK_PAGEUP:	return SDLK_PAGEUP;
		case WXK_PAGEDOWN:	return SDLK_PAGEDOWN;
		case WXK_NUMPAD_SPACE:	return SDLK_SPACE;
		case WXK_NUMPAD_TAB:	return SDLK_TAB;
		case WXK_NUMPAD_ENTER:	return SDLK_RETURN2;
		case WXK_NUMPAD_F1:	return SDLK_F1;
		case WXK_NUMPAD_F2:	return SDLK_F2;
		case WXK_NUMPAD_F3:	return SDLK_F3;
		case WXK_NUMPAD_F4:	return SDLK_F4;
		case WXK_NUMPAD_HOME:	return SDLK_HOME;
		case WXK_NUMPAD_LEFT:	return SDLK_LEFT;
		case WXK_NUMPAD_UP:		return SDLK_UP;
		case WXK_NUMPAD_RIGHT:	return SDLK_RIGHT;
		case WXK_NUMPAD_DOWN:	return SDLK_DOWN;
		case WXK_NUMPAD_PAGEUP:	return SDLK_PAGEUP;
		case WXK_NUMPAD_PAGEDOWN:return SDLK_PAGEDOWN;
		case WXK_NUMPAD_END:	return SDLK_END;
		case WXK_NUMPAD_INSERT:	return SDLK_INSERT;
		case WXK_NUMPAD_DELETE:	return SDLK_DELETE;
		case WXK_NUMPAD_EQUAL:	return SDLK_EQUALS;
		case WXK_NUMPAD_MULTIPLY:return SDLK_KP_MULTIPLY;
		case WXK_NUMPAD_ADD:	return SDLK_KP_PLUS;
		case WXK_NUMPAD_SEPARATOR:return SDLK_SEPARATOR;
		case WXK_NUMPAD_SUBTRACT:return SDLK_MINUS;
		case WXK_NUMPAD_DECIMAL:return SDLK_KP_DECIMAL;
		case WXK_NUMPAD_DIVIDE:	return SDLK_KP_DIVIDE;
		case WXK_WINDOWS_MENU:	return SDLK_MENU;
		case WXK_WINDOWS_RIGHT:	return SDLK_RGUI;
#ifdef __APPLE__
		case WXK_COMMAND:
#endif
		case WXK_WINDOWS_LEFT:	return SDLK_LGUI;
		default: return SDLK_UNKNOWN;
	}

	return SDLK_UNKNOWN;
}

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
	std::shared_ptr<ShaderProgram> l_pProgram = ProgramManager::singleton().getData(a_Key);
	useProgram(l_pProgram);
}

void GraphicCommander::setRenderTarget(int a_DSVHandle, unsigned int a_NumRT, ...)
{
	std::vector<int> l_RtvHandleList;
	l_RtvHandleList.resize(a_NumRT);
	if( 0 != a_NumRT )
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
GraphicCanvas::GraphicCanvas(wxWindow *a_pParent, wxWindowID a_ID)
	: wxWindow(a_pParent, a_ID)
	, m_bFullScreen(false)
	, m_bInitialed(false)
	, m_bNeedResize(false)
{
	if( 0 != SDL_WasInit(SDL_INIT_EVENTS) )
	{
		Connect(wxEVT_LEFT_DOWN, wxMouseEventHandler(GraphicCanvas::onMouseDown), nullptr, this);
		Connect(wxEVT_MIDDLE_DOWN, wxMouseEventHandler(GraphicCanvas::onMouseDown), nullptr, this);
		Connect(wxEVT_RIGHT_DOWN, wxMouseEventHandler(GraphicCanvas::onMouseDown), nullptr, this);
		Connect(wxEVT_AUX1_DOWN, wxMouseEventHandler(GraphicCanvas::onMouseDown), nullptr, this);
		Connect(wxEVT_AUX2_DOWN, wxMouseEventHandler(GraphicCanvas::onMouseDown), nullptr, this);
		Connect(wxEVT_MOTION, wxMouseEventHandler(GraphicCanvas::onMouseMove), nullptr, this);
		Connect(wxEVT_LEFT_UP, wxMouseEventHandler(GraphicCanvas::onMouseUp), nullptr, this);
		Connect(wxEVT_MIDDLE_UP, wxMouseEventHandler(GraphicCanvas::onMouseUp), nullptr, this);
		Connect(wxEVT_RIGHT_UP, wxMouseEventHandler(GraphicCanvas::onMouseUp), nullptr, this);
		Connect(wxEVT_AUX1_UP, wxMouseEventHandler(GraphicCanvas::onMouseUp), nullptr, this);
		Connect(wxEVT_AUX2_UP, wxMouseEventHandler(GraphicCanvas::onMouseUp), nullptr, this);
		Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(GraphicCanvas::onKeyDown), nullptr, this);
		Connect(wxEVT_KEY_UP , wxKeyEventHandler(GraphicCanvas::onKeyUp), nullptr, this);
	}
}

GraphicCanvas::~GraphicCanvas()
{
}

void GraphicCanvas::setFullScreen(bool a_bFullScreen)
{
	if( m_bFullScreen != a_bFullScreen )
	{
		m_bFullScreen = a_bFullScreen;
#ifdef WIN32
		DEVMODE l_DevMode;
		memset(&l_DevMode, 0, sizeof(DEVMODE));
		l_DevMode.dmSize = sizeof(DEVMODE);
		l_DevMode.dmPelsWidth = GetClientSize().x;
		l_DevMode.dmPelsHeight = GetClientSize().y;
		l_DevMode.dmBitsPerPel = 32;
		l_DevMode.dmDisplayFrequency = 60;
		l_DevMode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;

		bool l_bSuccess = ChangeDisplaySettings(&l_DevMode, CDS_FULLSCREEN) == DISP_CHANGE_SUCCESSFUL;
		assert( l_bSuccess && "invalid display settings");
#endif
	}
}

BEGIN_EVENT_TABLE(GraphicCanvas, wxWindow)
	EVT_SIZE(GraphicCanvas::onSize)
	EVT_ENTER_WINDOW(GraphicCanvas::onMouseEnter)
END_EVENT_TABLE()

void GraphicCanvas::onSize(wxSizeEvent &a_Event)
{
	if( !m_bInitialed ) return;
	if( a_Event.GetSize().x * a_Event.GetSize().y <= 1 ) return;
	m_bNeedResize = true;
}

void GraphicCanvas::onMouseDown(wxMouseEvent &a_Event)
{
	SDL_Event l_SDLEvent;
	switch( a_Event.GetButton() )
	{
		case wxMOUSE_BTN_LEFT:	l_SDLEvent.button.button = SDL_BUTTON_LEFT;		break;
		case wxMOUSE_BTN_MIDDLE:l_SDLEvent.button.button = SDL_BUTTON_MIDDLE;	break;
		case wxMOUSE_BTN_RIGHT:	l_SDLEvent.button.button = SDL_BUTTON_RIGHT;	break;
		case wxMOUSE_BTN_AUX1:	l_SDLEvent.button.button = SDL_BUTTON_X1;		break;
		case wxMOUSE_BTN_AUX2:	l_SDLEvent.button.button = SDL_BUTTON_X2;		break;
		default:return;
	}
	l_SDLEvent.type = SDL_MOUSEBUTTONDOWN;
	SDL_PushEvent(&l_SDLEvent);
}

void GraphicCanvas::onMouseMove(wxMouseEvent &a_Event)
{
	wxPoint l_Pos = a_Event.GetPosition();

	SDL_Event l_SDLEvent;
	l_SDLEvent.type = SDL_MOUSEMOTION;
	l_SDLEvent.motion.x = l_Pos.x;
	l_SDLEvent.motion.y = l_Pos.y;
	SDL_PushEvent(&l_SDLEvent);
}

void GraphicCanvas::onMouseUp(wxMouseEvent &a_Event)
{
	SDL_Event l_SDLEvent;
	switch( a_Event.GetButton() )
	{
		case wxMOUSE_BTN_LEFT:	l_SDLEvent.button.button = SDL_BUTTON_LEFT;		break;
		case wxMOUSE_BTN_MIDDLE:l_SDLEvent.button.button = SDL_BUTTON_MIDDLE;	break;
		case wxMOUSE_BTN_RIGHT:	l_SDLEvent.button.button = SDL_BUTTON_RIGHT;	break;
		case wxMOUSE_BTN_AUX1:	l_SDLEvent.button.button = SDL_BUTTON_X1;		break;
		case wxMOUSE_BTN_AUX2:	l_SDLEvent.button.button = SDL_BUTTON_X2;		break;
		default:return;
	}
	l_SDLEvent.type = SDL_MOUSEBUTTONUP;
	SDL_PushEvent(&l_SDLEvent);
}

void GraphicCanvas::onMouseWheel(wxMouseEvent &a_Event)
{
	SDL_Event l_SDLEvent;
	l_SDLEvent.type = SDL_MOUSEWHEEL;
	switch( a_Event.GetWheelAxis() )
	{
		case wxMOUSE_WHEEL_VERTICAL:
			l_SDLEvent.wheel.x = 0;
			l_SDLEvent.wheel.y = a_Event.GetWheelDelta();
			break;

		case wxMOUSE_WHEEL_HORIZONTAL:
			l_SDLEvent.wheel.x = a_Event.GetWheelDelta();
			l_SDLEvent.wheel.y = 0;
			break;
	}
	SDL_PushEvent(&l_SDLEvent);
}

void GraphicCanvas::onKeyDown(wxKeyEvent &a_Event)
{
	SDL_Event l_SDLEvent;
	l_SDLEvent.type = SDL_KEYDOWN;
	l_SDLEvent.key.keysym.sym = convertKeyCode(a_Event);
	SDL_PushEvent(&l_SDLEvent);
}

void GraphicCanvas::onKeyUp(wxKeyEvent &a_Event)
{
	SDL_Event l_SDLEvent;
	l_SDLEvent.type = SDL_KEYUP;
	l_SDLEvent.key.keysym.sym = convertKeyCode(a_Event);
	SDL_PushEvent(&l_SDLEvent);
}

void GraphicCanvas::onMouseEnter(wxMouseEvent &a_Event)
{
	SetFocus();
}

void GraphicCanvas::setInitialed()
{
	m_bInitialed = true;
}

bool GraphicCanvas::getNeedResize()
{
	bool l_Res = m_bNeedResize;
	m_bNeedResize = false;
	return l_Res;
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
		case VTXSLOT_NORMAL:
		case VTXSLOT_TANGENT:
		case VTXSLOT_BINORMAL:	l_Res = sizeof(glm::vec3); break;
		case VTXSLOT_BONE:		l_Res = sizeof(glm::ivec4); break;
		case VTXSLOT_WEIGHT:	l_Res = sizeof(glm::vec4); break;
		case VTXSLOT_COLOR:
		case VTXSLOT_INSTANCE:	l_Res = sizeof(glm::ivec4); break;
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
		case ShaderParamType::double1:	return sizeof(double);
	}

	assert(false && "invalid argument type");
	return 0;
}

void GraphicDevice::syncUavBuffer(bool a_bToGpu, unsigned int a_NumBuff, ...)
{
	std::vector<unsigned int> l_UavHandleList;
	l_UavHandleList.resize(a_NumBuff);
	{
		va_list l_Arglist;
		va_start(l_Arglist, a_NumBuff);
		for( unsigned int i=0 ; i<a_NumBuff ; ++i ) l_UavHandleList[i] = va_arg(l_Arglist, int);
		va_end(l_Arglist);
	}
	syncUavBuffer(a_bToGpu, l_UavHandleList);
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