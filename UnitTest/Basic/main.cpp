#include "REngine.h"

class BasicApp: public wxApp
{
public:
	BasicApp() : wxApp(){}
	
private:
	virtual bool OnInit();
};

IMPLEMENT_APP(BasicApp)

bool BasicApp::OnInit()
{
	R::EngineCore::singleton().createCanvas();
	
	return true;
}