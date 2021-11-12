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

	//R::ModelManager::singleton().setFlipYZ(true);
	R::ModelManager::singleton().setReverseFace(true);
	std::shared_ptr<R::Asset> l_pMeshAsset = R::AssetManager::singleton().getAsset("sponza/sponza.Mesh");
	//R::AssetManager::singleton().saveAsset(l_pMeshAsset);
	auto l_pMeshNode = l_pScene->getRootNode()->addChild(l_pMeshAsset);

	std::vector<std::shared_ptr<R::RenderableMesh>> l_Meshes;
	l_pMeshNode->getComponentsInChildren<R::RenderableMesh>(l_Meshes);
	for( unsigned int i=0 ; i<l_Meshes.size() ; ++i ) l_Meshes[i]->setStatic(true);
	
	{// add dir light
		std::shared_ptr<R::SceneNode> l_pDirLightNode = l_pScene->getRootNode()->addChild(); 
		std::shared_ptr<R::DirLight> l_pDirLight = l_pDirLightNode->addComponent<R::DirLight>();
		l_pDirLight->setColor(glm::vec3(1.0f, 1.0f, 0.7f));
		l_pDirLight->setIntensity(5.0f);
		l_pDirLight->setShadowed(true);
		//l_pDirLight->setStatic(true);
		l_pDirLightNode->setRotate(glm::eulerAngleYZ(-0.25f * glm::pi<float>(), -0.25f * glm::pi<float>()));
	}

	{// add omni light
		std::shared_ptr<R::SceneNode> l_pLightNode = l_pScene->getRootNode()->addChild(); 
		std::shared_ptr<R::OmniLight> l_pOmniLight = l_pLightNode->addComponent<R::OmniLight>();
		l_pOmniLight->setColor(glm::vec3(0.5f, 1.0f, 1.0f));
		l_pOmniLight->setIntensity(5000000.0f);
		l_pLightNode->setScale(glm::vec3(250.0f, 250.0f, 250.0f));
		l_pLightNode->setPosition(glm::vec3(0.0f, 0.0f, 0.0f));
		l_pOmniLight->setShadowed(true);
	}
	
	std::shared_ptr<R::SceneNode> l_pCameraNode = l_pScene->getRootNode()->find(wxT("Default Camera"));
	std::shared_ptr<R::CameraController> l_pCameraCtrl = l_pCameraNode->addComponent<R::CameraController>();
	l_pCameraCtrl->setMaxSpeed(500.0f);

	R::DebugLineHelper::singleton().addDebugLine(l_pScene->getRootNode());
	//l_pScene->getLightmap()->getComponent<R::LightmapAsset>()->bake(l_pScene);

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