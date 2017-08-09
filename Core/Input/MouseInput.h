// MouseInput.h
//
// 2017/08/09 Ian Wu/Real0000
//

#ifndef _MOUSE_INPUT_H_
#define _MOUSE_INPUT_H_

#include "InputMediator.h"

namespace R
{
	
#define MOUSE_DEVICE wxT("Mouse")
class MouseInput : public InputDeviceInterface
{
public:
	MouseInput(InputMediator *a_pOwner);
	virtual ~MouseInput();
	
	virtual void removeKey(int a_Define){}
	virtual void mapKey(int a_Define, wxString a_Key){}
	virtual bool processEvent(SDL_Event &a_Event, InputData &a_Output);

protected:
	virtual void initKeymap(std::vector<DeviceKey> &a_Output);

private:
};

}

#endif