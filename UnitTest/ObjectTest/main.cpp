#include "REngine.h"
#include "REditor.h"
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
	l_pScene->getRenderPipeline()->setDrawFlag(DRAW_DEBUG_TEXTURE);
	
	R::SceneManager::singleton().setMainScene(m_pCanvas, l_pScene);

	R::ModelManager::singleton().setFlipYZ(true);
	std::shared_ptr<R::Asset> l_pMeshAsset = R::AssetManager::singleton().getAsset("sponza/sponza.Mesh");
	auto l_pMeshNode = l_pScene->getRootNode()->addChild(l_pMeshAsset);
	
	std::shared_ptr<R::SceneNode> l_pDirLightNode = l_pScene->getRootNode()->addChild(); 
	std::shared_ptr<R::DirLight> l_pDirLight = l_pDirLightNode->addComponent<R::DirLight>();
	l_pDirLight->setColor(glm::vec3(1.0f, 1.0f, 0.7f));
	l_pDirLight->setIntensity(5.0f);
	l_pDirLight->setShadowed(true);
	l_pDirLightNode->setRotate(glm::eulerAngleYZ(-0.25f * glm::pi<float>(), -0.25f * glm::pi<float>()));

	std::shared_ptr<R::SceneNode> l_pCameraNode = l_pScene->getRootNode()->find(wxT("Default Camera"));
	std::shared_ptr<R::CameraController> l_pCameraCtrl = l_pCameraNode->addComponent<R::CameraController>();
	l_pCameraCtrl->setMaxSpeed(250.0f);

	GetTopWindow()->Connect(wxEVT_CLOSE_WINDOW, wxCloseEventHandler(BasicApp::onClose), nullptr, this);
	R::EngineCore::singleton().run(this);

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