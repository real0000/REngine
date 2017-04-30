// StateMachine.h
//
// 2014/03/12 Ian Wu/Real0000
//

#ifndef _STATE_MACHINE_H_
#define _STATE_MACHINE_H_

namespace R
{

class BaseStateItem;
class StateMachine;

class BaseStateItem
{
public:
	BaseStateItem(StateMachine *l_pOwner, int l_StateID);
	virtual ~BaseStateItem();

	virtual void begin() = 0;
	virtual void end() = 0;
	virtual void update(int l_ThreadID, float l_Delta) = 0;
	virtual void eventHandler(int l_EventID, std::vector<wxString> &l_Params) = 0;

	unsigned int getStateID(){ return m_StateID; }
	StateMachine* getStateMachine(){ return m_pOwner; }

protected:
	void switchState(unsigned int l_StateID);
	unsigned int addEvent(unsigned int l_ThreadID, float l_DelayTime, int l_EventID, ...);
	unsigned int addEvent(unsigned int l_ThreadID, float l_DelayTime, int l_EventID, wxString l_Params);
	void removeEvent(unsigned int l_EventKey);

	unsigned int getPrevStateID();
	unsigned int getNextStateID();

private:
	unsigned int m_StateID;
	StateMachine *m_pOwner;
};

template<class T>
class StateItem : public BaseStateItem
{
public:
	StateItem(StateMachine *l_pOwner, int l_StateID) : BaseStateItem(l_pOwner, l_StateID){}
	virtual ~StateItem(){}

	T getOwner(){ return dynamic_cast<T>(getStateMachine()); }
};

class StateMachine
{
	friend class BaseStateItem;
public:
	StateMachine();
	virtual ~StateMachine();

	virtual wxString getEventFormat(int l_EventID) = 0;

	virtual void update(unsigned int l_ThreadID, float l_Delta);
	void switchState(unsigned int StateID);

	unsigned int addEvent(unsigned int l_ThreadID, float l_DelayTime, unsigned int l_EventID, wxString l_Params = wxT(""));
	void addInterruptEvent(unsigned int l_ThreadID, unsigned int l_EventID, wxString l_Params = wxT(""));
	void removeEvent(unsigned int l_EventKey);

protected:
	virtual BaseStateItem* stateItemFactory(int l_StateID) = 0;

private:
	struct StateEvent
	{
		float m_Time;
		unsigned int m_EventID;
		std::vector<wxString> m_Params;
	};

	BaseStateItem *m_pPrevState, *m_pCurrState, *m_pNextState;
	std::map<unsigned int, std::map<unsigned int, StateEvent> > m_Events;// thread : [event serial : event, ...], ...
	std::map<unsigned int, BaseStateItem *> m_StateStorage;
	int m_EventSerial;
	
};

}

#endif