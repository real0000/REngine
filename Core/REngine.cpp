// REngine.cpp
//
// 2014/03/12 Ian Wu/Real0000
//

#include "CommonUtil.h"
#include "REngine.h"
#include <time.h>

namespace R
{

//
// EngineThread
//
EngineThread::EngineThread(float l_Period, unsigned int l_ID)
	: m_ID(l_ID), m_Period(l_Period)
	, m_Prev(0)
	, m_Curr(0)
	, m_bExit(false)
{
}

EngineThread::~EngineThread()
{
}

void EngineThread::fakeEntry()
{
	m_Curr = timeGetTime();
	float l_Delta = (m_Curr - m_Prev) * 0.001f;
	if( l_Delta >= m_Period )
	{
		EngineCore::singleton().threadUpdate(m_ID, l_Delta);
		m_Prev = m_Curr;
	}
}

wxThread::ExitCode EngineThread::Entry()
{
	while( !EngineCore::singleton().isShutdown() && !m_bExit )
	{
		m_Curr = timeGetTime();
		float l_Delta = (m_Curr - m_Prev) * 0.001f;
		if( l_Delta >= m_Period )
		{
			EngineCore::singleton().threadUpdate(m_ID, l_Delta);
			m_Prev = m_Curr;
		}
		else
		{
			float l_Percent = (m_Period - l_Delta) / m_Period;
			unsigned int l_Priority = (wxPRIORITY_MAX - wxPRIORITY_DEFAULT) * l_Percent * 0.5f + wxPRIORITY_DEFAULT;
			SetPriority(l_Priority);
			Sleep((m_Curr - m_Prev) >> 1);
		}
	}
    
	EngineCore::singleton().threadJoin(m_ID);

	return NULL;
}

void EngineThread::OnExit()
{

}

//
// EngineCore
//
EngineCore& EngineCore::singleton()
{
	static EngineCore l_Inst;
	if( !l_Inst.m_bValid )
	{
		if( !l_Inst.init() ) wxMessageBox(wxT("Invalid to initilize engine"));
	}
	return l_Inst;
}

EngineCore::EngineCore()
	: m_bValid(false)
{
}

EngineCore::~EngineCore()
{
	for( unsigned int i=0 ; i<m_MutexLock.size() ; ++i ) delete m_MutexLock[i];
	m_MutexLock.clear();
}

bool EngineCore::init()
{
	m_bValid = true;

	for( unsigned int i=0 ; i<LOCKER_CUSTOM_ID_START ; ++i ) m_MutexLock.push_back(new wxMutex());

	m_pMainMachine = new CoreMachine();
	m_pMainMachine->switchState(CORE_STATE_DEFAULT);
	
	EngineThread *l_pNewEngineThread = new EngineThread(1.0f/60.0f, DEF_THREAD_GRAPHIC);// graphic thread shoud be main thread, so create as fake
	m_EngineThread[DEF_THREAD_GRAPHIC] = l_pNewEngineThread;

	addThread(DEF_THREAD_INPUT, 1.0f/60.0f);
	addThread(DEF_THREAD_LOADER_0, 1.0f/20.0f);
	addThread(DEF_THREAD_LOADER_1, 1.0f/20.0f);
	addThread(DEF_THREAD_LOADER_2, 1.0f/20.0f);
	addThread(DEF_THREAD_LOADER_3, 1.0f/20.0f);

	return true;
}

void EngineCore::threadUpdate(unsigned int l_ID, float l_Delta)
{
	m_pMainMachine->update(l_ID, l_Delta);
	for( unsigned int i=0 ; i<m_RegistedMachine.size() ; ++i ) m_RegistedMachine[i]->update(l_ID, l_Delta);
}

void EngineCore::threadJoin(unsigned int l_ID)
{
	lock(LOCKER_MAIN);

	std::map<unsigned int, EngineThread *>::iterator it = m_EngineThread.find(l_ID);
	assert(m_EngineThread.end() != it && "invalid thread id");

	m_pMainMachine->addInterruptEvent(l_ID, COMMON_EVENT_THREAD_END);
	for( unsigned int i=0 ; i<m_RegistedMachine.size() ; ++i ) m_RegistedMachine[i]->addInterruptEvent(l_ID, COMMON_EVENT_THREAD_END);

	if( DEF_THREAD_GRAPHIC == l_ID ) delete it->second;
	m_EngineThread.erase(it);

	if( m_EngineThread.empty() && m_bShutdown )
	{
		m_pMainMachine->addInterruptEvent(l_ID, COMMON_EVENT_SHUTDOWN);
		for( unsigned int i=0 ; i<m_RegistedMachine.size() ; i++ )
		{
			m_RegistedMachine[i]->addInterruptEvent(l_ID, COMMON_EVENT_SHUTDOWN);
			delete m_RegistedMachine[i];
		}
		m_RegistedMachine.clear();
		delete m_pMainMachine;
		m_pMainMachine = NULL;
	}
	
	unlock(LOCKER_MAIN);
}

void EngineCore::initGameWindow(wxString l_CfgFile)
{
	//
	// to do : read xml file for settings
	//
	m_pMainMachine->addEvent(DEF_THREAD_GRAPHIC, 0.0f, CoreMachine::EVENT_CREATE_SINGLE_CANVAS, wxT("0 Test 1280 720 0 0"));
}

//void initWnd(wxWindow *l_pParent);

bool EngineCore::isShutdown()
{
	return m_bShutdown;
}

void EngineCore::shutDown()
{
	m_bShutdown = true;
	while( 1 != m_EngineThread.size() ) Sleep(1000 / 60);
	threadJoin(DEF_THREAD_GRAPHIC);

	ExternModelFileManager::purge();
	GLSLProgramManager::purge();
	TextureManager::purge();
}

void EngineCore::mainLoop()
{
	m_EngineThread[DEF_THREAD_GRAPHIC]->fakeEntry();
}

void EngineCore::addThread(unsigned int l_ID, float l_Period)
{
	assert( l_Period > 0.0f );

	EngineThread *l_pNewEngineThread = new EngineThread(l_Period, l_ID);
	
	lock(LOCKER_MAIN);
		m_EngineThread[l_ID] = l_pNewEngineThread;
	unlock(LOCKER_MAIN);

	l_pNewEngineThread->Create();
	l_pNewEngineThread->Run();
}
	
void EngineCore::removeThread(unsigned int l_ID)
{
	assert(l_ID >= DEF_THREAD_RESERVE_END && "Invalid thread id");
	std::map<unsigned int, EngineThread *>::iterator it = m_EngineThread.find(l_ID);
	assert(it != m_EngineThread.end());
	it->second->setOffDuty();
}

inline void EngineCore::lock(unsigned int l_LockerID)
{
	m_MutexLock[l_LockerID]->Lock();
}

inline void EngineCore::unlock(unsigned int l_LockerID)
{
	m_MutexLock[l_LockerID]->Unlock();
}

void EngineCore::loadModel(wxString l_Filename, bool l_bImmediate)
{
	m_pMainMachine->addEvent(DEF_THREAD_GRAPHIC, 0.0f, CoreMachine::EVENT_LOAD_MODEL_FILE, wxString::Format(wxT("%s %d"), l_Filename, l_bImmediate ? 1 : 0));
}

}