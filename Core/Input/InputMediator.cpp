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

	while( !m_Pool.empty() )
	{
		delete m_Pool.front();
		m_Pool.pop_front();
	}
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
	while( SDL_PollEvent(&l_Event) )
	{
		InputData *l_pData = requestData();
		l_pData->m_Type = INPUTTYPE_UNDEFINED;
		for( unsigned int i=0 ; i<m_InputMap.size() ; ++i )
		{
			if( m_InputMap[i]->processEvent(l_Event, *l_pData) ) break;
		}
		if( INPUTTYPE_UNDEFINED != l_pData->m_Type ) m_Buffer.push_back(l_pData);
		else m_Pool.push_back(l_pData);
	}

	if( 0 != (m_Flags & SIMPLE_MODE) )
	{
		std::sort(m_Buffer.begin(), m_Buffer.end(), [](const InputData *a_pLeft, const InputData *a_pRight)
		{
		   return a_pLeft->m_Key < a_pRight->m_Key;
		});
		auto l_ListIt = std::unique(m_Buffer.begin(), m_Buffer.end(), [](const InputData *a_pLeft, const InputData *a_pRight)
		{
			return a_pLeft->m_Key == a_pRight->m_Key;
		});
		for( auto it = l_ListIt ; it != m_Buffer.end() ; ++it ) m_Pool.push_back(*it);
		m_Buffer.erase(l_ListIt, m_Buffer.end());
	}

	SceneManager::singleton().processInput(m_Buffer);
	
	for( unsigned int i=0 ; i<m_Buffer.size() ; ++i ) m_Pool.push_back(m_Buffer[i]);
	m_Buffer.clear();
}

InputData* InputMediator::requestData()
{
	if( m_Pool.empty() ) return new InputData();

	InputData *l_pRes = m_Pool.front();
	m_Pool.pop_front();
	return l_pRes;
}
#pragma endregion

}