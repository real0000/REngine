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
	
	R::SceneManager::singleton().setMainScene(m_pCanvas, l_pScene);

	auto l_pMeshNode = l_pScene->getRootNode()->addChild(wxT("sponza/sponza.FBX"));
	l_pScene->getRootNode()->addComponent<R::DirLight>();

	std::shared_ptr<R::SceneNode> l_pCameraNode = l_pScene->getRootNode()->find(wxT("Default Camera"));
	std::shared_ptr<R::CameraController> l_pCameraCtrl = l_pCameraNode->addComponent<R::CameraController>();
	l_pCameraCtrl->setMaxSpeed(0.01f);

	std::shared_ptr<R::RenderableMesh> l_pGridMesh = l_pScene->getRootNode()->addComponent<R::RenderableMesh>();
	l_pGridMesh->setMesh(R::AssetManager::singleton().getAsset(QUAD_MESH_ASSET_NAME), 0);

	std::shared_ptr<R::Asset> l_pGridMat = R::AssetManager::singleton().createAsset(wxT("Grid.Material"));
	R::MaterialAsset *l_pGridMatInst = l_pGridMat->getComponent<R::MaterialAsset>();
	l_pGridMatInst->setParam("m_XColor", 0, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
    l_pGridMatInst->setParam("m_YColor", 0, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
    l_pGridMatInst->setParam("m_ZColor", 0, glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
    l_pGridMatInst->setParam("m_LineColor", 0, glm::vec4(0.5f, 0.5f, 0.5f, 1.0f));
    l_pGridMatInst->setParam("m_PlaneType", 0, 0);
    l_pGridMatInst->setParam("m_MinUnit", 0, 10.0f);
	l_pGridMesh->setMaterial(R::MATSLOT_TRANSPARENT, l_pGridMat);

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