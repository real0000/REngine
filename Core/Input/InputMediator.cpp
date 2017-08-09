// InputMediator.cpp
//
// 2017/07/30 Ian Wu/Real0000
//

#include "CommonUtil.h"

#include "Core.h"
#include "InputMediator.h"
#include "KeyboardInput.h"
#include "MouseInput.h"

namespace R
{

#pragma region InputDeviceInterface
//
// InputDeviceInterface
//
InputDeviceInterface::InputDeviceInterface(InputMediator *a_pOwner, wxString a_ID)
	: m_pRefOwner(a_pOwner)
	, m_ID(a_ID)
{
}

InputDeviceInterface::~InputDeviceInterface()
{
}
#pragma endregion

#pragma region InputMediator
//
// InputMediator
//
InputMediator::InputMediator(unsigned int a_Flag)
	: m_Flags(a_Flag)
{
	m_InputMap.push_back(new KeyboardInput(this));
	m_InputMap.push_back(new MouseInput(this));

	// to do : add game controller and vive controller
}

InputMediator::~InputMediator()
{
	m_Listener.clear();
	m_ReadyListener.clear();
	m_DroppedListener.clear();

	for( unsigned int i=0 ; i<m_InputMap.size() ; ++i ) delete m_InputMap[i];
	m_InputMap.clear();
}

void InputMediator::addKey(int a_Define)
{
	if( m_DefinedKey.end() != m_DefinedKey.find(a_Define) ) return;
	m_DefinedKey.insert(a_Define);
}

void InputMediator::removeKey(int a_Define)
{
	if( m_DefinedKey.end() == m_DefinedKey.find(a_Define) ) return;
	m_DefinedKey.erase(a_Define);
	for( unsigned int i=0 ; i<m_InputMap.size() ; ++i ) m_InputMap[i]->removeKey(a_Define);
}

void InputMediator::mapKey(int a_Define, wxString a_DeviceType, wxString a_Key)
{
	assert(a_Define >= USER_DEFINED);
	for( unsigned int i=0 ; i<m_InputMap.size() ; ++i )
	{
		if( m_InputMap[i]->getIdentify() == a_DeviceType )
		{
			m_InputMap[i]->mapKey(a_Define, a_Key);
			break;
		}
	}
}

void InputMediator::addListener(std::shared_ptr<EngineComponent> a_pComponent)
{
	std::lock_guard<std::mutex> l_Locker(m_Locker);

	auto it = std::find(m_Listener.begin(), m_Listener.end(), a_pComponent);
	if( it != m_Listener.end() ) return;
	
	auto it2 = std::find(m_ReadyListener.begin(), m_ReadyListener.end(), a_pComponent);
	if( it2 != m_ReadyListener.end() ) return;

	m_DroppedListener.erase(a_pComponent);
	m_ReadyListener.push_back(a_pComponent);
}

void InputMediator::removeListener(std::shared_ptr<EngineComponent> a_pComponent)
{
	std::lock_guard<std::mutex> l_Locker(m_Locker);
	
	auto it = std::find(m_Listener.begin(), m_Listener.end(), a_pComponent);
	if( it == m_Listener.end() ) return;
	
	auto it2 = m_DroppedListener.find(a_pComponent);
	if( it2 == m_DroppedListener.end() ) return;

	it = std::find(m_ReadyListener.begin(), m_ReadyListener.end(), a_pComponent);
	if( m_ReadyListener.end() == it ) m_ReadyListener.erase(it);
	m_DroppedListener.insert(a_pComponent);
}

void InputMediator::clearListener()
{
	std::lock_guard<std::mutex> l_Locker(m_Locker);
	
	m_Listener.clear();
	m_ReadyListener.clear();
	m_DroppedListener.clear();
}

void InputMediator::pollEvent()
{
	{// do add/remove listener
		std::lock_guard<std::mutex> l_Locker(m_Locker);
		if( !m_DroppedListener.empty() )
		{
			for( auto it = m_DroppedListener.begin() ; it != m_DroppedListener.end() ; ++it )
			{
				auto l_ListenerIt = std::find(m_Listener.begin(), m_Listener.end(), *it);
				m_Listener.erase(l_ListenerIt);
			}
			m_DroppedListener.clear();
		}

		if( !m_ReadyListener.empty() )
		{
			for( auto it = m_ReadyListener.begin() ; it != m_ReadyListener.end() ; ++it ) m_Listener.push_back(*it);
			m_ReadyListener.clear();
		}
	}

	SDL_Event l_Event;
	InputData l_Data;
	while( SDL_PollEvent(&l_Event) )
	{
		l_Data.m_Type = INPUTTYPE_UNDEFINED;
		for( unsigned int i=0 ; i<m_InputMap.size() ; ++i )
		{
			if( m_InputMap[i]->processEvent(l_Event, l_Data) ) break;
		}
		if( INPUTTYPE_UNDEFINED != l_Data.m_Type ) m_Buffer.push_back(l_Data);
	}

	for( auto l_InputIt = m_Buffer.begin() ; l_InputIt != m_Buffer.end() ; ++l_InputIt )
	{
		for( auto it = m_Listener.begin() ; it != m_Listener.end() ; ++it )
		{
			if( (*it)->inputListener(*l_InputIt) ) break;
		}
	}
	m_Buffer.clear();
}
#pragma endregion

}