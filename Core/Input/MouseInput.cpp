// MouseInput.cpp
//
// 2017/08/09 Ian Wu/Real0000
//


#include "CommonUtil.h"

#include "Core.h"
#include "InputMediator.h"
#include "MouseInput.h"

namespace R
{
	
MouseInput::MouseInput(InputMediator *a_pOwner)
	: InputDeviceInterface(a_pOwner, MOUSE_DEVICE)
{
	SDL_CaptureMouse(SDL_TRUE);
}

MouseInput::~MouseInput()
{
}
	
bool MouseInput::processEvent(SDL_Event &a_Event, InputData &a_Output)
{
	unsigned int l_Flag = getOwner()->getFlag();
	switch( a_Event.type )
	{
		case SDL_MOUSEMOTION:{
			if( 0 == (l_Flag & InputMediator::ALLOW_MOUSE_MOVE) ) return false;

			a_Output.m_Key = InputMediator::MOUSE_MOVE;
			a_Output.m_Type = INPUTTYPE_MOTION;
			a_Output.m_Data.m_Val[0] = a_Event.motion.x;
			a_Output.m_Data.m_Val[1] = a_Event.motion.y;
			
			if( 0 == (l_Flag & InputMediator::SIMPLE_MODE) ) a_Output.m_Text = wxT("Motion");
			}return true;

		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:{
			if( 0 == (l_Flag & InputMediator::ALLOW_MOUSE_BUTTON) ) return false;

			switch( a_Event.button.button )
			{
				case SDL_BUTTON_LEFT:  a_Output.m_Key = InputMediator::MOUSE_LEFT_BUTTON;  break;
				case SDL_BUTTON_MIDDLE:a_Output.m_Key = InputMediator::MOUSE_MIDDLE_BUTTON;break;
				case SDL_BUTTON_RIGHT: a_Output.m_Key = InputMediator::MOUSE_RIGHT_BUTTON; break;
				default:return false;
			}
			
			if( 0 == (l_Flag & InputMediator::SIMPLE_MODE) )
			{
				switch( a_Event.button.button )
				{
					case SDL_BUTTON_LEFT:  a_Output.m_Text = wxT("Left Button");  break;
					case SDL_BUTTON_MIDDLE:a_Output.m_Text = wxT("Middle Button");break;
					case SDL_BUTTON_RIGHT: a_Output.m_Text = wxT("Right Button"); break;
				}
			}

			a_Output.m_Type = INPUTTYPE_BUTTON;
			a_Output.m_Data.m_bDown = a_Event.type == SDL_MOUSEBUTTONDOWN;
			}return true;
		
		case SDL_MOUSEWHEEL:{
			if( 0 == (l_Flag & InputMediator::ALLOW_MOUSE_WHEEL) ) return false;

			a_Output.m_Key = InputMediator::MOUSE_WHEEL;
			a_Output.m_Data.m_Val[0] = a_Event.wheel.x;
			a_Output.m_Data.m_Val[1] = a_Event.wheel.y;
			if( 0 == (l_Flag & InputMediator::SIMPLE_MODE) ) a_Output.m_Text = wxT("Wheel");
			}return true;

		default: return false;
	}

	return false;
}

void MouseInput::initKeymap(std::vector<DeviceKey> &a_Output)
{
	a_Output.resize(InputMediator::MOUSE_WHEEL - InputMediator::MOUSE_LEFT_BUTTON + 1);

	a_Output[InputMediator::MOUSE_LEFT_BUTTON].m_KeyName = wxT("Left Button");
	a_Output[InputMediator::MOUSE_LEFT_BUTTON].m_Type = INPUTTYPE_BUTTON;
	
	a_Output[InputMediator::MOUSE_MIDDLE_BUTTON].m_KeyName = wxT("Middle Button");
	a_Output[InputMediator::MOUSE_MIDDLE_BUTTON].m_Type = INPUTTYPE_BUTTON;
	
	a_Output[InputMediator::MOUSE_RIGHT_BUTTON].m_KeyName = wxT("Right Button");
	a_Output[InputMediator::MOUSE_RIGHT_BUTTON].m_Type = INPUTTYPE_BUTTON;
	
	a_Output[InputMediator::MOUSE_MOVE].m_KeyName = wxT("Motion");
	a_Output[InputMediator::MOUSE_MOVE].m_Type = INPUTTYPE_MOTION;

	a_Output[InputMediator::MOUSE_WHEEL].m_KeyName = wxT("Wheel");
	a_Output[InputMediator::MOUSE_WHEEL].m_Type = INPUTTYPE_MOTION;
}

}