// KeyboardInput.cpp
//
// 2017/08/08 Ian Wu/Real0000
//

#include "CommonUtil.h"

#include "Core.h"
#include "KeyboardInput.h"

namespace R
{

#pragma region KeyboardInput
//
// KeyboardInput
//
KeyboardInput::KeyboardInput(InputMediator *a_pOwner)
	: InputDeviceInterface(a_pOwner, KEYBOARD_DEVICE)
{
}

KeyboardInput::~KeyboardInput()
{
}
	
void KeyboardInput::removeKey(int a_Define)
{
	for( auto it = m_KeyMap.begin() ; it != m_KeyMap.end() ; ++it )
	{
		if( it->second == a_Define )
		{
			m_KeyMap.erase(it);
			return;
		}
	}
}

void KeyboardInput::mapKey(int a_Define, wxString a_Key)
{
	removeKey(a_Define);
	SDL_Keycode l_Code = SDL_GetKeyFromName(a_Key);
	if( SDLK_UNKNOWN == l_Code ) return;
	m_KeyMap.insert(std::make_pair(l_Code, a_Define));
}

bool KeyboardInput::processEvent(SDL_Event &a_Event, InputData &a_Output)
{
	a_Output.m_DeviceName = getIdentify();

	unsigned int l_Flag = getOwner()->getFlag();
	if( 0 == (l_Flag & InputMediator::ALLOW_KEYBOARD) ) return false;

	if( SDL_KEYDOWN != a_Event.type && SDL_KEYUP != a_Event.type ) return false;
	if( 0 != (InputMediator::SIMPLE_MODE & l_Flag) )
	{
		auto it = m_KeyMap.find(a_Event.key.keysym.sym);
		if( m_KeyMap.end() == it ) return false;

		a_Output.m_Key = it->second;
		a_Output.m_Data.m_bDown = SDL_KEYDOWN == a_Event.type;
		a_Output.m_Type = INPUTTYPE_BUTTON;
		return true;
	}

	//auto l_NameIt = m_NameMap.find(a_Event.key.keysym.sym);
	//if( m_NameMap.end() == l_NameIt ) return false;// or return true but don't do anything?
	//const InputDeviceInterface::DeviceKey &l_KeyData = getDeviceKey()[l_NameIt->second];

	a_Output.m_Key = a_Event.key.keysym.sym;
	a_Output.m_Text = SDL_GetKeyName(a_Event.key.keysym.sym);//l_KeyData.m_KeyName;
	a_Output.m_Data.m_bDown = SDL_KEYDOWN == a_Event.type;
	a_Output.m_Type = INPUTTYPE_BUTTON;
	return true;
}

void KeyboardInput::initKeymap(std::vector<InputDeviceInterface::DeviceKey> &a_Output)
{
	// list all sdl keycode
	const SDL_Keycode c_Keycodes[] = {
		SDLK_RETURN,
		SDLK_ESCAPE,
		SDLK_BACKSPACE,
		SDLK_TAB,
		SDLK_SPACE,
		SDLK_EXCLAIM,
		SDLK_QUOTEDBL,
		SDLK_HASH,
		SDLK_PERCENT,
		SDLK_DOLLAR,
		SDLK_AMPERSAND,
		SDLK_QUOTE,
		SDLK_LEFTPAREN,
		SDLK_RIGHTPAREN,
		SDLK_ASTERISK,
		SDLK_PLUS,
		SDLK_COMMA,
		SDLK_MINUS,
		SDLK_PERIOD,
		SDLK_SLASH,
		SDLK_0,
		SDLK_1,
		SDLK_2,
		SDLK_3,
		SDLK_4,
		SDLK_5,
		SDLK_6,
		SDLK_7,
		SDLK_8,
		SDLK_9,
		SDLK_COLON,
		SDLK_SEMICOLON,
		SDLK_LESS,
		SDLK_EQUALS,
		SDLK_GREATER,
		SDLK_QUESTION,
		SDLK_AT,

		SDLK_LEFTBRACKET,
		SDLK_BACKSLASH,
		SDLK_RIGHTBRACKET,
		SDLK_CARET,
		SDLK_UNDERSCORE,
		SDLK_BACKQUOTE,
		SDLK_a,
		SDLK_b,
		SDLK_c,
		SDLK_d,
		SDLK_e,
		SDLK_f,
		SDLK_g,
		SDLK_h,
		SDLK_i,
		SDLK_j,
		SDLK_k,
		SDLK_l,
		SDLK_m,
		SDLK_n,
		SDLK_o,
		SDLK_p,
		SDLK_q,
		SDLK_r,
		SDLK_s,
		SDLK_t,
		SDLK_u,
		SDLK_v,
		SDLK_w,
		SDLK_x,
		SDLK_y,
		SDLK_z,

		SDLK_CAPSLOCK,

		SDLK_F1,
		SDLK_F2,
		SDLK_F3,
		SDLK_F4,
		SDLK_F5,
		SDLK_F6,
		SDLK_F7,
		SDLK_F8,
		SDLK_F9,
		SDLK_F10,
		SDLK_F11,
		SDLK_F12,

		SDLK_PRINTSCREEN,
		SDLK_SCROLLLOCK,
		SDLK_PAUSE,
		SDLK_INSERT,
		SDLK_HOME,
		SDLK_PAGEUP,
		SDLK_DELETE,
		SDLK_END,
		SDLK_PAGEDOWN,
		SDLK_RIGHT,
		SDLK_LEFT,
		SDLK_DOWN,
		SDLK_UP,

		SDLK_NUMLOCKCLEAR,
		SDLK_KP_DIVIDE,
		SDLK_KP_MULTIPLY,
		SDLK_KP_MINUS,
		SDLK_KP_PLUS,
		SDLK_KP_ENTER,
		SDLK_KP_1,
		SDLK_KP_2,
		SDLK_KP_3,
		SDLK_KP_4,
		SDLK_KP_5,
		SDLK_KP_6,
		SDLK_KP_7,
		SDLK_KP_8,
		SDLK_KP_9,
		SDLK_KP_0,
		SDLK_KP_PERIOD,

		SDLK_APPLICATION,
		SDLK_POWER,
		SDLK_KP_EQUALS,
		SDLK_F13,
		SDLK_F14,
		SDLK_F15,
		SDLK_F16,
		SDLK_F17,
		SDLK_F18,
		SDLK_F19,
		SDLK_F20,
		SDLK_F21,
		SDLK_F22,
		SDLK_F23,
		SDLK_F24,
		SDLK_EXECUTE,
		SDLK_HELP,
		SDLK_MENU,
		SDLK_SELECT,
		SDLK_STOP,
		SDLK_AGAIN,
		SDLK_UNDO,
		SDLK_CUT,
		SDLK_COPY,
		SDLK_PASTE,
		SDLK_FIND,
		SDLK_MUTE,
		SDLK_VOLUMEUP,
		SDLK_VOLUMEDOWN,
		SDLK_KP_COMMA,
		SDLK_KP_EQUALSAS400,

		SDLK_ALTERASE,
		SDLK_SYSREQ,
		SDLK_CANCEL,
		SDLK_CLEAR,
		SDLK_PRIOR,
		SDLK_RETURN2,
		SDLK_SEPARATOR,
		SDLK_OUT,
		SDLK_OPER,
		SDLK_CLEARAGAIN,
		SDLK_CRSEL,
		SDLK_EXSEL,

		SDLK_KP_00,
		SDLK_KP_000,
		SDLK_THOUSANDSSEPARATOR,
		SDLK_DECIMALSEPARATOR,
		SDLK_CURRENCYUNIT,
		SDLK_CURRENCYSUBUNIT,
		SDLK_KP_LEFTPAREN,
		SDLK_KP_RIGHTPAREN,
		SDLK_KP_LEFTBRACE,
		SDLK_KP_RIGHTBRACE,
		SDLK_KP_TAB,
		SDLK_KP_BACKSPACE,
		SDLK_KP_A,
		SDLK_KP_B,
		SDLK_KP_C,
		SDLK_KP_D,
		SDLK_KP_E,
		SDLK_KP_F,
		SDLK_KP_XOR,
		SDLK_KP_POWER,
		SDLK_KP_PERCENT,
		SDLK_KP_LESS,
		SDLK_KP_GREATER,
		SDLK_KP_AMPERSAND,
		SDLK_KP_DBLAMPERSAND,
		SDLK_KP_VERTICALBAR,
		SDLK_KP_DBLVERTICALBAR,
		SDLK_KP_COLON,
		SDLK_KP_HASH,
		SDLK_KP_SPACE,
		SDLK_KP_AT,
		SDLK_KP_EXCLAM,
		SDLK_KP_MEMSTORE,
		SDLK_KP_MEMRECALL,
		SDLK_KP_MEMCLEAR,
		SDLK_KP_MEMADD,
		SDLK_KP_MEMSUBTRACT,
		SDLK_KP_MEMMULTIPLY,
		SDLK_KP_MEMDIVIDE,
		SDLK_KP_PLUSMINUS,
		SDLK_KP_CLEAR,
		SDLK_KP_CLEARENTRY,
		SDLK_KP_BINARY,
		SDLK_KP_OCTAL,
		SDLK_KP_DECIMAL,
		SDLK_KP_HEXADECIMAL,

		SDLK_LCTRL,
		SDLK_LSHIFT,
		SDLK_LALT,
		SDLK_LGUI,
		SDLK_RCTRL,
		SDLK_RSHIFT,
		SDLK_RALT,
		SDLK_RGUI,

		SDLK_MODE,

		SDLK_AUDIONEXT,
		SDLK_AUDIOPREV,
		SDLK_AUDIOSTOP,
		SDLK_AUDIOPLAY,
		SDLK_AUDIOMUTE,
		SDLK_MEDIASELECT,
		SDLK_WWW,
		SDLK_MAIL,
		SDLK_CALCULATOR,
		SDLK_COMPUTER,
		SDLK_AC_SEARCH,
		SDLK_AC_HOME,
		SDLK_AC_BACK,
		SDLK_AC_FORWARD,
		SDLK_AC_STOP,
		SDLK_AC_REFRESH,
		SDLK_AC_BOOKMARKS,

		SDLK_BRIGHTNESSDOWN,
		SDLK_BRIGHTNESSUP,
		SDLK_DISPLAYSWITCH,
		SDLK_KBDILLUMTOGGLE,
		SDLK_KBDILLUMDOWN,
		SDLK_KBDILLUMUP,
		SDLK_EJECT,
		SDLK_SLEEP
		};
	const unsigned int c_NumKeycode = sizeof(c_Keycodes) / sizeof(const SDL_Keycode);
	a_Output.resize(c_NumKeycode);
	for( unsigned int i=0 ; i<c_NumKeycode ; ++i )
	{
		InputDeviceInterface::DeviceKey &l_Target = a_Output[i];
		l_Target.m_KeyName = SDL_GetKeyName(c_Keycodes[i]);
		l_Target.m_Type = INPUTTYPE_BUTTON;
		m_NameMap.insert(std::make_pair(c_Keycodes[i], i));
	}
}

}