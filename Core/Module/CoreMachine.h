// CoreMachine.h
//
// 2014/03/12 Ian Wu/Real0000
//

#ifndef _CORE_MACHINE_H_
#define _CORE_MACHINE_H_

#include "StateMachine.h"

namespace R
{

class BaseCanvas;
class CoreMachine;

enum
{
	CORE_STATE_DEFAULT = 0,
};

class CoreState : public StateItem<CoreMachine *>
{
public:
	CoreState(StateMachine *l_pOwner, int l_StateID);
	virtual ~CoreState();

	virtual void begin();
	virtual void end();
	virtual void update(int l_ThreadID, float l_Delta);
	virtual void eventHandler(int l_EventID, std::vector<wxString> &l_Params);

private:
	void render(float l_Delta);
};

class CoreMachine : public StateMachine
{
	friend class CoreState;
public:
	enum
	{
		EVENT_CREATE_SINGLE_CANVAS = 0,
		EVENT_LOAD_MODEL_FILE,
	};

public:
	CoreMachine();
	virtual ~CoreMachine();

	virtual wxString getEventFormat(int l_EventID) override;

protected:
	virtual BaseStateItem* stateItemFactory(int l_StateID) override;

private:
	std::map<unsigned int, BaseCanvas *> m_Canvas;
};

}

#endif