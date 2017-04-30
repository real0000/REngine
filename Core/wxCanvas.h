// wxCanvas.h
//
// 2014/03/12 Ian Wu/Real0000
//

#ifndef _WX_CANVAS_H_
#define _WX_CANVAS_H_

#include "wx/glcanvas.h"

namespace R
{

class GLSLProgramManager;
class BaseCanvas;

class BaseD3DCanvas : public wxFrame
{
public:
	BaseD3DCanvas(wxWindow *l_pParent, wxWindowID l_ID, const wxSize& l_WndSize, const int *l_pAttrList, bool l_bFullScreen, bool l_bStereo);
	virtual ~BaseD3DCanvas();

private:
	ID3D12Device *m_pDevice;
};

class BaseGLCanvas : public wxGLCanvas
{
public:
	BaseGLCanvas(wxWindow *l_pParent, wxWindowID l_ID, const wxSize& l_WndSize, const int *l_pAttrList, bool l_bFullScreen, bool l_bStereo);
	virtual ~BaseGLCanvas();
	
	bool isFullScreen(){ return m_bFullScreen; }
	bool isStereo(){ return m_bStereo; }
	virtual bool Show(bool l_bShow = true);

	bool begin(){ if( NULL != m_pContext ) return m_pContext->SetCurrent(*this); return false; }
	virtual void render(float l_Delta);
	void end(){ glFlush(); SwapBuffers(); }
	
protected:
	void initGLEW();
	
	wxGLContext *m_pContext;

private:
	static wxGLContext *m_pSharedContext;
	bool m_bFullScreen, m_bStereo;
};

class DefaultCanvas
{
public:
	static DefaultCanvas* newCanvas(wxWindowID l_ID, const wxString& l_Title, const wxSize& l_WndSize = wxDefaultSize, bool l_bFullScreen = false, bool l_bStereo = false);

	void setTitle(wxString &l_Title);
	virtual void render(float l_Delta);

	void setClearColor(glm::vec3 l_Color){ m_ClearColor = l_Color; }

private:
	DefaultCanvas(wxWindow *l_pParent, wxWindowID l_ID, const wxSize& l_WndSize, bool l_bFullScreen, bool l_bStereo, const int *l_pAttrList);
	virtual ~DefaultCanvas();

	glm::vec3 m_ClearColor;
};

}

#endif