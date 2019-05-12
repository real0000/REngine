#include "REngine.h"

class BasicApp: public wxApp
{
public:
	BasicApp() : wxApp(){}
	
private:
	virtual bool OnInit();
	virtual int OnExit();

	std::shared_ptr<R::GraphicCanvas> m_pCanvas;
};

IMPLEMENT_APP(BasicApp)

bool BasicApp::OnInit()
{
	m_pCanvas = R::EngineCore::singleton().createCanvas();
	
	std::shared_ptr<R::Scene> l_pScene = R::SceneManager::singleton().create(wxT("Test"));
	l_pScene->initEmpty();
	
	R::SceneManager::singleton().setMainScene(m_pCanvas, l_pScene);

	return true;
}

int BasicApp::OnExit()
{
	m_pCanvas = nullptr;

	return 0;
}