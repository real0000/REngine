// InputMediator.cpp
//
// 2017/07/30 Ian Wu/Real0000
//

#include "CommonUtil.h"

#include "Core.h"
#include "InputMediator.h"
#include "KeyboardInput.h"
#include "MouseInput.h"

#include "Scene/Scene.h"

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

void InputMediator::pollEvent()
{
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

	if( 0 != (m_Flags & SIMPLE_MODE) )
	{
		std::sort(m_Buffer.begin(), m_Buffer.end(), [](const InputData &a_Left, const InputData &a_Right)
		{
		   return a_Left.m_Key < a_Right.m_Key;
		});
		auto l_ListIt = std::unique(m_Buffer.begin(), m_Buffer.end(), [](const InputData &a_Left, const InputData &a_Right)
		{
			return a_Left.m_Key == a_Right.m_Key;
		});
		m_Buffer.erase(l_ListIt, m_Buffer.end());
	}

	SceneManager::singleton().preprocessInput();
	for( auto l_InputIt = m_Buffer.begin() ; l_InputIt != m_Buffer.end() ; ++l_InputIt )
	{
		SceneManager::singleton().processInput(*l_InputIt);
	}
	m_Buffer.clear();
}
#pragma endregion

}