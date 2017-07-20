// StateMachine.cpp
//
// 2014/03/12 Ian Wu/Real0000
//

#include "CommonUtil.h"
#include "StateMachine.h"

namespace R
{
//
// StateItem
//
BaseStateItem::BaseStateItem(StateMachine *a_pOwner, int a_StateID)
	: m_pOwner(a_pOwner)
	, m_StateID(a_StateID)
{
	assert(NULL != m_pOwner);
}

BaseStateItem::~BaseStateItem()
{
}

void BaseStateItem::switchState(unsigned int a_StateID)
{
	m_pOwner->switchState(a_StateID);
}

unsigned int BaseStateItem::addEvent(float a_DelayTime, int a_EventID, ...)
{
	va_list l_Args;

	va_start(l_Args, a_EventID);

	wxString l_Params;
	l_Params.FormatV(m_pOwner->getEventFormat(a_EventID), l_Args);

	va_end(l_Args);

	return m_pOwner->addEvent(a_DelayTime, a_EventID, l_Params);
}

unsigned int BaseStateItem::addEvent(float a_DelayTime, int a_EventID, wxString a_Params)
{
	return m_pOwner->addEvent(a_DelayTime, a_EventID, a_Params);
}

void BaseStateItem::removeEvent(unsigned int a_EventKey)
{
	m_pOwner->removeEvent(a_EventKey);
}

unsigned int BaseStateItem::getPrevStateID()
{
	if( NULL == m_pOwner->m_pPrevState ) return -1;
	return m_pOwner->m_pPrevState->getStateID();
}

unsigned int BaseStateItem::getNextStateID()
{
	if( NULL == m_pOwner->m_pNextState ) return -1;
	return m_pOwner->m_pNextState->getStateID();
}

//
// StateMachine
//
StateMachine::StateMachine()
	: m_EventSerial(0)
	, m_pPrevState(NULL), m_pCurrState(NULL), m_pNextState(NULL)
{
}

StateMachine::~StateMachine()
{
	for( auto it = m_StateStorage.begin() ; it != m_StateStorage.end() ; it++ ) delete it->second;
	m_StateStorage.clear();
}

void StateMachine::update(float a_Delta)
{
	std::vector< std::map<unsigned int, StateEvent>::iterator > l_EventList;
	for( auto l_StateIt = m_Events.begin() ; l_StateIt != m_Events.end() ; l_StateIt++ )
	{
		l_StateIt->second.m_Time -= a_Delta;
		if( 0.0f >= l_StateIt->second.m_Time ) l_EventList.push_back(l_StateIt);
	}

	if( NULL != m_pCurrState )
	{
		for( unsigned int i=0 ; i<l_EventList.size() ; ++i ) m_pCurrState->eventHandler(l_EventList[i]->second.m_EventID, l_EventList[i]->second.m_Params);
		m_pCurrState->update(a_Delta);
	}

	for( int i=int(l_EventList.size())-1 ; i>=0 ; --i ) m_Events.erase(l_EventList[i]);
}

void StateMachine::switchState(unsigned int a_StateID)
{
	BaseStateItem *l_pItem = m_StateStorage[a_StateID];
	if( NULL == l_pItem ) m_StateStorage[a_StateID] = l_pItem = stateItemFactory(a_StateID);
	assert(NULL != l_pItem);

	m_pNextState = l_pItem;
	if( NULL != m_pCurrState ) m_pCurrState->end();
	m_pPrevState = m_pCurrState;
	m_pCurrState = m_pNextState;
	m_pNextState = NULL;
	m_pCurrState->begin();
}

unsigned int StateMachine::addEvent(float a_DelayTime, unsigned int a_EventID, wxString a_Params)
{
	unsigned int l_ID = m_EventSerial;
	++m_EventSerial;
	
	StateEvent l_NewEvt;
	l_NewEvt.m_EventID = a_EventID;
	l_NewEvt.m_Time = a_DelayTime;
	splitString(wxT(' '), a_Params, l_NewEvt.m_Params);
	m_Events.insert(std::make_pair(m_EventSerial, l_NewEvt));

	return l_ID;
}

void StateMachine::addInterruptEvent(unsigned int a_EventID, wxString a_Params)
{
	unsigned int l_ID = m_EventSerial;
	
	std::vector<wxString> l_SplitParams;
	splitString(wxT(' '), a_Params, l_SplitParams);

	if( NULL != m_pCurrState ) m_pCurrState->eventHandler(a_EventID, l_SplitParams);
}

void StateMachine::removeEvent(unsigned int a_EventID)
{
	std::map<unsigned int, StateEvent>::iterator it = m_Events.find(a_EventID);
	if( m_Events.end() != it ) m_Events.erase(it);
}

}