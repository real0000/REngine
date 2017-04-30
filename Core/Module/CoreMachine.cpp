// CoreMachine.cpp
//
// 2014/03/12 Ian Wu/Real0000
//

#include "CommonUtil.h"

#include "CommonConst.h"
#include "Module/CoreMachine.h"
#include "Module/Model/ExternModel.h"
#include "wxCanvas.h"

namespace R
{

//
// CoreState
//
CoreState::CoreState(StateMachine *l_pOwner, int l_StateID)
	: StateItem(l_pOwner, l_StateID)
{

}

CoreState::~CoreState()
{

}

void CoreState::begin()
{

}

void CoreState::end()
{

}

void CoreState::update(int l_ThreadID, float l_Delta)
{
	switch( l_ThreadID )
	{
		case DEF_THREAD_GRAPHIC:{
			render(l_Delta);
			}break;

		case DEF_THREAD_INPUT:{
			}break;

		case DEF_THREAD_LOADER_0:{
			}break;
			 
		case DEF_THREAD_LOADER_1:{
			}break;

		case DEF_THREAD_LOADER_2:{
			}break;

		case DEF_THREAD_LOADER_3:{
			}break;
	}
}

void CoreState::eventHandler(int l_EventID, std::vector<wxString> &l_Params)
{
	switch( l_EventID )
	{
		case CoreMachine::EVENT_CREATE_SINGLE_CANVAS:{
			unsigned int l_CanvasID = atoi(l_Params[0].c_str());
			wxString l_Title = l_Params[1];
			unsigned int l_Width = atoi(l_Params[2].c_str());
			unsigned int l_Height = atoi(l_Params[3].c_str());
			bool l_bFullScreen = 0 != atoi(l_Params[4].c_str());
			bool l_bStereo = 0 != atoi(l_Params[5].c_str());

			DefaultCanvas *l_pWnd = DefaultCanvas::newCanvas(wxID_ANY, l_Title, wxSize(l_Width, l_Height), l_bFullScreen, l_bStereo);
			l_pWnd->Show();

			CoreMachine *l_pOwner = getOwner();
			assert(NULL != l_pOwner);
			l_pOwner->m_Canvas[l_CanvasID] = l_pWnd;
			}break;

		case CoreMachine::EVENT_LOAD_MODEL_FILE:{
			wxString l_File = l_Params[0];
			//ExternModelData *l_pModelData = ExternModelFileManager::singleton().getModelData(l_File, l_bImmediate);
			//if( !l_bImmediate && !l_pModelData->isValid() )
			{
				//addEvent(DEF_THREAD_LOADER_0
			}
			}break;

		case COMMON_EVENT_SHUTDOWN:{
			CoreMachine *l_pOwner = getOwner();
			l_pOwner->m_Canvas.clear();
			}break;

		default:break;
	}
}

void CoreState::render(float l_Delta)
{
	CoreMachine *l_pOwner = getOwner();
	for( unsigned int i=0 ; i<l_pOwner->m_Canvas.size() ; ++i )
	{
		if( l_pOwner->m_Canvas[i]->begin() )
		{
			l_pOwner->m_Canvas[i]->render(l_Delta);
			l_pOwner->m_Canvas[i]->end();
		}
	}
}

//
// CoreMachine
//
CoreMachine::CoreMachine()
{
}

CoreMachine::~CoreMachine()
{
}

wxString CoreMachine::getEventFormat(int l_EventID)
{
	switch(l_EventID)
	{
		case EVENT_CREATE_SINGLE_CANVAS:	return wxT("CanvasID(int) Title(string) Width(int) Height(int) FullScreen(bool) Stereo(bool)");
		case EVENT_LOAD_MODEL_FILE:			return wxT("Filename(string) IsImmediate(bool)");
		default:break;
	}

	return wxT("");
}
	
BaseStateItem* CoreMachine::stateItemFactory(int l_StateID)
{
	switch( l_StateID )
	{
		case CORE_STATE_DEFAULT: return new CoreState(this, CORE_STATE_DEFAULT);

		default:break;
	}

	return NULL;
}

}