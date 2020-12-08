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

/*class TempComponent : public R::EngineComponent
{
	friend class R::EngineComponent;
public:
	virtual void start()
	{
		addUpdateListener();
	}

	virtual unsigned int typeID(){ return R::CUSTOM_COMPONENT + 1; }

	virtual void updateListener(float a_Delta)
	{
		m_Angle += glm::pi<float>() / 6.0f * a_Delta;
		if( m_Angle > glm::pi<float>() * 2.0f ) m_Angle -= glm::pi<float>() * 2.0f;

		glm::mat4x4 l_World(1.0f);
		glm::vec3 l_CameraPos(glm::vec3(200.0f * std::sin(m_Angle), 0.0f, 200.0f * std::cos(m_Angle)));
		l_World = glm::translate(l_World, l_CameraPos);
		//getSharedMember()->m_pSceneNode->setTransform(l_World);
	}

	virtual void loadComponent(boost::property_tree::ptree &a_Src){}
	virtual void saveComponent(boost::property_tree::ptree &a_Dst){}

private:
	TempComponent(std::shared_ptr<R::Scene> a_pRefScene, std::shared_ptr<R::SceneNode> a_pOwner)
		: R::EngineComponent(a_pRefScene, a_pOwner)
		, m_Angle(0.0f)
	{
	}

	float m_Angle;
};*/

bool BasicApp::OnInit()
{
	m_pCanvas = R::EngineCore::singleton().createCanvas();
	
	std::shared_ptr<R::Scene> l_pScene = R::SceneManager::singleton().create(wxT("Test"));
	l_pScene->initEmpty();
	
	R::SceneManager::singleton().setMainScene(m_pCanvas, l_pScene);

	std::shared_ptr<R::SceneNode> l_pMeshNode = l_pScene->getRootNode()->addChild();
	std::shared_ptr<R::RenderableMesh> l_pNewMesh = l_pMeshNode->addComponent<R::RenderableMesh>();

	std::shared_ptr<R::Asset> l_pMeshAsset = R::AssetManager::singleton().getAsset(wxT("sponza/sponza.obj"));
	l_pNewMesh->setMesh(l_pMeshAsset, 0);

	std::shared_ptr<R::Asset> l_pDirShadow = R::AssetManager::singleton().createAsset(wxT("sponza_00_Dir.Material"));
	//l_pDirShadow->getComponent<R::MaterialAsset>()->setTexture();
	l_pNewMesh->setMaterial(R::MATSLOT_DIR_SHADOWMAP, l_pDirShadow);

	std::shared_ptr<R::SceneNode> l_pCameraNode = l_pScene->getRootNode()->find(wxT("Default Camera"));
	//l_pCameraNode->addComponent<TempComponent>();

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