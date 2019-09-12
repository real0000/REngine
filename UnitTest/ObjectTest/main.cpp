#include "REngine.h"
#include "vld.h"

class BasicApp: public wxApp
{
public:
	BasicApp() : wxApp(), m_pCanvas(nullptr){}
	
	void onClose(wxCloseEvent &a_Event);

private:
	virtual bool OnInit();

	R::GraphicCanvas *m_pCanvas;
};

IMPLEMENT_APP(BasicApp)

bool BasicApp::OnInit()
{
	m_pCanvas = R::EngineCore::singleton().createCanvas();
	
	std::shared_ptr<R::Scene> l_pScene = R::SceneManager::singleton().create(wxT("Test"));
	l_pScene->initEmpty();
	
	R::SceneManager::singleton().setMainScene(m_pCanvas, l_pScene);

	std::shared_ptr<R::SceneNode> l_pMeshNode = l_pScene->getRootNode()->addChild();
	std::shared_ptr<R::RenderableMesh> l_pNewMesh = l_pMeshNode->addComponent<R::RenderableMesh>();
	l_pNewMesh->setMesh(wxT("Cube.FBX"), nullptr);

	glm::mat4x4 l_World(glm::identity<glm::mat4x4>());
	l_World = glm::translate(l_World, glm::vec3(0.0f, 0.0f, -100.0f));
	std::shared_ptr<R::SceneNode> l_pCameraNode = l_pScene->getRootNode()->find(wxT("Default Camera"));
	l_pCameraNode->setTransform(l_World);

	GetTopWindow()->Connect(wxEVT_CLOSE_WINDOW, wxCloseEventHandler(BasicApp::onClose), nullptr, this);

	return true;
}

void BasicApp::onClose(wxCloseEvent &a_Event)
{
	R::SceneManager::singleton().dropCanvas(m_pCanvas);
	m_pCanvas = nullptr;
	R::SceneManager::singleton().dropScene(wxT("Test"));

	R::EngineCore::singleton().shutDown();

	GetTopWindow()->Destroy();
}