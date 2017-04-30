#pragma once

#include "CommonUtil.h"
#include "REngine.h"

class MainApp: public wxApp
{
public:
	MainApp() : wxApp(){}
	
private:
	virtual bool OnInit();
	virtual void OnIdle(wxIdleEvent &l_Evt);
	virtual int OnExit();
};

IMPLEMENT_APP(MainApp)

bool MainApp::OnInit()
{
	R::EngineCore::singleton().initGameWindow();
	
	Connect(wxID_ANY, wxEVT_IDLE, wxIdleEventHandler(MainApp::OnIdle));

	return true;
}

void MainApp::OnIdle(wxIdleEvent &l_Evt)
{
	R::EngineCore::singleton().mainLoop();
	l_Evt.RequestMore();
}

int MainApp::OnExit()
{
	R::EngineCore::singleton().shutDown();
	return wxApp::OnExit();
}