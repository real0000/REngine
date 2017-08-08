// InputMediator.h
//
// 2017/07/30 Ian Wu/Real0000
//

#ifndef _INPUT_MEDIATOR_H_
#define _INPUT_MEDIATOR_H_

namespace R
{

class EngineCore;
class EngineComponent;
class InputMediator;

enum InputType
{
	INPUTTYPE_BUTTON = 0,
	INPUTTYPE_ANALOG,
	INPUTTYPE_ANALOG2,
	INPUTTYPE_TEXT,

	INPUTTYPE_UNDEFINED
};

struct InputData
{
	InputData() : m_Key(0), m_Type(INPUTTYPE_UNDEFINED){}
	~InputData(){}

	unsigned int m_Key;
	union 
	{
		bool m_bDown;
		float m_Val[2];// 0.0f ~ 1.0f
	} m_Data;
	wxString m_Text;// or key name
	InputType m_Type;
};

class InputDeviceInterface
{
public:
	struct DeviceKey
	{
		wxString m_KeyName;
		InputType m_Type;
		std::pair<float, float> m_Range;// if is analog
	};

public:
	InputDeviceInterface(InputMediator *a_pOwner, wxString a_ID);
	virtual ~InputDeviceInterface();
	
	wxString getIdentify(){ return m_ID; }
	const std::vector<DeviceKey>& getDeviceKey(){ return m_Key; }
	
	virtual void removeKey(int a_Define) = 0;
	virtual void mapKey(int a_Define, wxString a_Key) = 0;
	virtual bool processEvent(SDL_Event &a_Event, InputData &a_Output) = 0;

protected:
	virtual void initKeymap(std::vector<DeviceKey> &a_Output) = 0;
	InputMediator* getOwner(){ return m_pRefOwner; }

private:
	InputMediator *m_pRefOwner;
	wxString m_ID;
	std::vector<DeviceKey> m_Key;
};

class InputMediator
{
	friend class EngineCore;
public:
	enum
	{
		MOUSE_LEFT_BUTTON = 0,
		MOUSE_MIDDLE_BUTTON,
		MOUSE_RIGHT_BUTTON,
		MOUSE_MOVE,
		MOUSE_WHEEL,

		TOUCH_ID1,
		TOUCH_ID2,

		USER_DEFINED, // all user defined key must >= USER_DEFINED
	};
	enum
	{
		SIMPLE_MODE          = 0x00000001, // only receive important input

		ALLOW_KEYBOARD       = 0x00000002,

		ALLOW_MOUSE_BUTTON   = 0x00000004,
		ALLOW_MOUSE_MOVE     = 0x00000008,
		ALLOW_MOUSE_WHEEL    = 0x00000010,

		ALLOW_JOYSTICK       = 0x00000020,

		ALLOW_TOUCH          = 0x00000040,
		ALLOW_TOUCH_AS_MOUSE = 0x00000080,// set touch id 1 as mouse input
	};

public:
	template<typename T>
	void addDevice(wxString a_ID)
	{
		T *l_pNewInputDevice = new T(this, a_ID);
		m_InputMap.push_back(l_pNewInputDevice);
	}

	void addKey(int a_Define);
	void removeKey(int a_Define);
	void mapKey(int a_Define, wxString a_DeviceType, wxString a_Key);
	const std::set<int>& getDefinedKey(){ return m_DefinedKey; }

	void addListener(std::shared_ptr<EngineComponent> a_pComponent);
	void removeListener(std::shared_ptr<EngineComponent> a_pComponent);
	void clearListener();
	
	void setFlag(unsigned int a_Flag){ m_Flags = a_Flag; }
	void toggleFlag(unsigned int a_Flag){ m_Flags ^= a_Flag; }
	unsigned int getFlag(){ return m_Flags; }

	void pollEvent();

private:
	InputMediator(unsigned int a_Flag = ALLOW_KEYBOARD | ALLOW_MOUSE_BUTTON | ALLOW_MOUSE_MOVE | ALLOW_MOUSE_WHEEL | ALLOW_JOYSTICK | ALLOW_TOUCH | ALLOW_TOUCH_AS_MOUSE);
	virtual ~InputMediator();

	std::mutex m_Locker;
	std::list< std::shared_ptr<EngineComponent> > m_Listener, m_ReadyListener;
	std::set< std::shared_ptr<EngineComponent> > m_DroppedListener;
	std::set<int> m_DefinedKey;
		
	std::vector<InputDeviceInterface *> m_InputMap;
	std::list<InputData> m_Buffer;
	unsigned int m_Flags;
};

}

#endif