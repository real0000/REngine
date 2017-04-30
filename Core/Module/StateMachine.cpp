// StateMachine.cpp
//
// 2014/03/12 Ian Wu/Real0000
//

#include "CommonUtil.h"
#include "Module/StateMachine.h"

namespace R
{
//
// StateItem
//
BaseStateItem::BaseStateItem(StateMachine *l_pOwner, int l_StateID)
	: m_pOwner(l_pOwner)
	, m_StateID(m_StateID)
{
	assert(NULL != m_pOwner);
}

BaseStateItem::~BaseStateItem()
{
}

void BaseStateItem::switchState(unsigned int l_StateID)
{
	m_pOwner->switchState(l_StateID);
}

unsigned int BaseStateItem::addEvent(unsigned int l_ThreadID, float l_DelayTime, int l_EventID, ...)
{
	va_list l_Args;

	va_start(l_Args, l_EventID);

	wxString l_Params;
	l_Params.FormatV(m_pOwner->getEventFormat(l_EventID), l_Args);

	va_end(l_Args);

	return m_pOwner->addEvent(l_ThreadID, l_DelayTime, l_EventID, l_Params);
}

unsigned int BaseStateItem::addEvent(unsigned int l_ThreadID, float l_DelayTime, int l_EventID, wxString l_Params)
{
	return m_pOwner->addEvent(l_ThreadID, l_DelayTime, l_EventID, l_Params);
}

void BaseStateItem::removeEvent(unsigned int l_EventKey)
{
	m_pOwner->removeEvent(l_EventKey);
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

void StateMachine::update(unsigned int l_ThreadID, float l_Delta)
{
	std::map<unsigned int, std::map<unsigned int, StateEvent> >::iterator it = m_Events.find(l_ThreadID);
	if( m_Events.end() == m_Events.find(l_ThreadID) )
	{
		m_Events[l_ThreadID] = std::map<unsigned int, StateEvent>();
		it = m_Events.find(l_ThreadID);
	}

	std::vector< std::map<unsigned int, StateEvent>::iterator > l_EventList;
	for( std::map<unsigned int, StateEvent>::iterator l_StateIt = it->second.begin() ; l_StateIt != it->second.end() ; l_StateIt++ )
	{
		l_StateIt->second.m_Time -= l_Delta;
		if( 0.0f >= l_StateIt->second.m_Time ) l_EventList.push_back(l_StateIt);
	}

	if( NULL != m_pCurrState )
	{
		for( unsigned int i=0 ; i<l_EventList.size() ; ++i ) m_pCurrState->eventHandler(l_EventList[i]->second.m_EventID, l_EventList[i]->second.m_Params);
		m_pCurrState->update(l_ThreadID, l_Delta);
	}

	for( int i=int(l_EventList.size())-1 ; i>=0 ; --i ) it->second.erase(l_EventList[i]);
}

void StateMachine::switchState(unsigned int StateID)
{
	BaseStateItem *l_pItem = m_StateStorage[StateID];
	if( NULL == l_pItem ) m_StateStorage[StateID] = l_pItem = stateItemFactory(StateID);
	assert(NULL != l_pItem);

	m_pNextState = l_pItem;
	if( NULL != m_pCurrState ) m_pCurrState->end();
	m_pPrevState = m_pCurrState;
	m_pCurrState = m_pNextState;
	m_pNextState = NULL;
	m_pCurrState->begin();
}

unsigned int StateMachine::addEvent(unsigned int l_ThreadID, float l_DelayTime, unsigned int l_EventID, wxString l_Params)
{
	unsigned int l_ID = m_EventSerial | (l_ThreadID << 24);
	
	StateEvent l_NewEvt;
	l_NewEvt.m_EventID = l_EventID;
	l_NewEvt.m_Time = l_DelayTime;
	splitString(wxT(' '), l_Params, l_NewEvt.m_Params);

	std::map<unsigned int, std::map<unsigned int, StateEvent> >::iterator it = m_Events.find(l_ThreadID);
	if( m_Events.end() == m_Events.find(l_ThreadID) )
	{
		m_Events[l_ThreadID] = std::map<unsigned int, StateEvent>();
		it = m_Events.find(l_ThreadID);
	}

	it->second[m_EventSerial] = l_NewEvt;

	m_EventSerial = (++m_EventSerial) % 0xffffff;
	return l_ID;
}

void StateMachine::addInterruptEvent(unsigned int l_ThreadID, unsigned int l_EventID, wxString l_Params)
{
	unsigned int l_ID = m_EventSerial | (l_ThreadID << 24);
	
	std::vector<wxString> l_SplitParams;
	splitString(wxT(' '), l_Params, l_SplitParams);

	if( NULL != m_pCurrState ) m_pCurrState->eventHandler(l_EventID, l_SplitParams);
}

void StateMachine::removeEvent(unsigned int l_EventID)
{
	int l_ThreadID = (l_EventID & 0xff000000) >> 24;
	int l_EventSerial = l_EventID & 0x00ffffff;
	std::map<unsigned int, StateEvent> &l_EvtList = m_Events[l_ThreadID];
	std::map<unsigned int, StateEvent>::iterator it = l_EvtList.find(l_EventSerial);
	if( l_EvtList.end() != it ) l_EvtList.erase(it);
}

}