// KeyboardInput.h
//
// 2017/08/08 Ian Wu/Real0000
//

#ifndef _KEYBOARD_INPUT_H_
#define _KEYBOARD_INPUT_H_

#include "InputMediator.h"

namespace R
{

#define KEYBOARD_DEVICE wxT("Keyboard")
class KeyboardInput : public InputDeviceInterface
{
public:
	KeyboardInput(InputMediator *a_pOwner);
	virtual ~KeyboardInput();
	
	virtual void removeKey(int a_Define);
	virtual void mapKey(int a_Define, wxString a_Key);
	virtual bool processEvent(SDL_Event &a_Event, InputData &a_Output);

protected:
	virtual void initKeymap(std::vector<DeviceKey> &a_Output);

private:
	std::map<SDL_Keycode, unsigned int> m_NameMap, m_KeyMap;
};

}

#endif