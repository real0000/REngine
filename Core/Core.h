// Core.h
//
// 2014/03/12 Ian Wu/Real0000
//

#ifndef _CORE_H_
#define _CORE_H_

namespace R
{

class StateMachine;

class EngineThread;
class EngineCore;

class EngineThread : public wxThread
{
public:
	EngineThread(float l_Period, unsigned int l_ID);
	virtual ~EngineThread();

	void fakeEntry();
	virtual wxThread::ExitCode Entry();
    virtual void OnExit();

	void setOffDuty(){ m_bExit = true; }

private:
	unsigned int m_ID;
	float m_Period;
	time_t m_Prev, m_Curr;
	bool m_bExit;
};

class EngineCore
{
	friend class EngineThread;
public:
	static EngineCore& singleton();
	
	// for normal game frame : 1 canvas with close button
	void initGameWindow(wxString l_CfgFile = wxT("Config.xml"));
	void initWnd(wxWindow *l_pParent);

	bool isShutdown();
	void shutDown();

	void mainLoop();
	void addThread(unsigned int l_ID, float l_Period);
	void removeThread(unsigned int l_ID);
	inline void lock(unsigned int l_LockerID);
	inline void unlock(unsigned int l_LockerID);

	// models
	void loadModel(wxString l_Filename, bool l_bImmediate);

private:
	EngineCore();
	virtual ~EngineCore();

	bool init();
	void threadUpdate(unsigned int l_ID, float l_Delta);
	void threadJoin(unsigned int l_ID);

	StateMachine *m_pMainMachine;
	std::vector<StateMachine *> m_RegistedMachine;
	std::map<unsigned int, EngineThread *> m_EngineThread;
	std::vector<wxMutex *> m_MutexLock;
	bool m_bValid;
	bool m_bShutdown;
};

}

#endif