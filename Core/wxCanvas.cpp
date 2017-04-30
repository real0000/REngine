// wxCanvas.cpp
//
// 2014/03/12 Ian Wu/Real0000
//
#include "CommonUtil.h"
#include "Module/Program/GLSLShader.h"
#include "wxCanvas.h"

namespace R
{

#pragma region BaseCanvas
//
// BaseCanvas 
//
wxGLContext *BaseCanvas::m_pSharedContext = nullptr;
BaseCanvas::BaseCanvas(wxWindow *l_pParent, wxWindowID l_ID, const wxSize& l_WndSize, const int *l_pAttrList, bool l_bFullScreen, bool l_bStereo)
	: wxGLCanvas(l_pParent, l_ID, l_pAttrList, wxDefaultPosition, l_WndSize)
	, m_pContext(NULL)
	, m_bFullScreen(l_bFullScreen)
	, m_bStereo(l_bStereo)
{
}

BaseCanvas::~BaseCanvas()
{
	if( NULL != m_pContext ) delete m_pContext;
	m_pContext = NULL;
}

bool BaseCanvas::Show(bool l_bShow)
{
	bool l_bRet = GetParent()->Show(l_bShow);
	wxGLCanvas::Show(l_bShow);
	initGLEW();
	if( m_bFullScreen )
	{
		wxWindow *l_pWnd = this;
		while( nullptr != l_pWnd )
		{
			l_pWnd = l_pWnd->GetParent();
			wxFrame *l_pTopWnd = dynamic_cast<wxFrame *>(l_pWnd);
			if( nullptr != l_pWnd )
			{
				l_pTopWnd->ShowFullScreen(l_bShow);
				break;
			}
		}
	}
	return l_bRet;
}

void BaseCanvas::initGLEW()
{
	m_pContext = new wxGLContext(this, m_pSharedContext);
	if( nullptr == m_pSharedContext ) m_pSharedContext = m_pContext;
	
	m_pContext->SetCurrent(*this);
	GLenum l_Err = glewInit();
	if( GLEW_OK != l_Err ) wxMessageBox(wxT("glew init error"));
		
	if( !m_bFullScreen )
	{
		glFlush();
		SwapBuffers();
		return;
	}


    glFlush();
    SwapBuffers();
}
#pragma endregion

#pragma region DefaultCanvas
//
// DefaultCanvas
//
DefaultCanvas* DefaultCanvas::newCanvas(wxWindowID l_ID, const wxString& l_Title, const wxSize& l_WndSize, bool l_bFullScreen, bool l_bStereo)
{
	wxFrame *l_pParent = new wxFrame(NULL, wxID_ANY, l_Title, wxDefaultPosition, l_WndSize, wxCAPTION | wxCLOSE_BOX );
	l_pParent->SetClientSize(l_WndSize);
	std::vector<int> l_Params;
	if( l_bStereo ) l_Params.push_back(WX_GL_STEREO);
	l_Params.push_back(WX_GL_RGBA);
	l_Params.push_back(WX_GL_DOUBLEBUFFER);
	l_Params.push_back(WX_GL_DEPTH_SIZE);
	l_Params.push_back(24);
	l_Params.push_back(WX_GL_STENCIL_SIZE);
	l_Params.push_back(8);
	l_Params.push_back(NULL);
	DefaultCanvas *l_pCanvas = new DefaultCanvas(l_pParent, l_ID, l_WndSize, l_bFullScreen, l_bStereo, &(l_Params.front()));

	return l_pCanvas; 
}

DefaultCanvas::DefaultCanvas(wxWindow *l_pParent, wxWindowID l_ID, const wxSize& l_WndSize, bool l_bFullScreen, bool l_bStereo, const int *l_pAttrList)
	: BaseCanvas(l_pParent, l_ID, l_WndSize, l_pAttrList, l_bFullScreen, l_bStereo)
	, m_ClearColor(0.0f, 0.0f, 1.0f)
{
}

DefaultCanvas::~DefaultCanvas()
{
}


void DefaultCanvas::setTitle(wxString &l_Title)
{
	wxFrame *l_pFrame = dynamic_cast<wxFrame *>(GetParent());
	l_pFrame->SetTitle(l_Title);
}

void DefaultCanvas::render(float l_Delta)
{
	//
	// to do : after render pipeline finished, convert this code to another module
	//
	/*if( isStereo() )
	{
        glDrawBuffer(GL_BACK_LEFT);
		glClearColor(m_ClearColor.r, m_ClearColor.g, m_ClearColor.b, 0.0f);
		glClearDepth(1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		///

        glDrawBuffer(GL_BACK_RIGHT);
		glClearColor(m_ClearColor.r, m_ClearColor.g, m_ClearColor.b, 0.0f);
		glClearDepth(1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		///
	}
	else*/
	{
		glClearColor(m_ClearColor.r, m_ClearColor.g, m_ClearColor.b, 0.0f);
		glClearDepth(1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}
}
#pragma endregion

}