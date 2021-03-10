// REngine.cpp
//
// 2014/03/12 Ian Wu/Real0000
//

#include "CommonUtil.h"
#include "RImporters.h"
#include "RGDeviceWrapper.h"
#include "Core.h"
#include "Input/InputMediator.h"

#include "RenderObject/Light.h"
#include "RenderObject/Mesh.h"
#include "Scene/Camera.h"
#include "Scene/Graph/NoPartition.h"
#include "Scene/Graph/Octree.h"
#include "Scene/RenderPipline/Deferred.h"
#include "Scene/RenderPipline/ShadowMap.h"
#include "Scene/Scene.h"

#include "Asset/AssetBase.h"
#include "Asset/MaterialAsset.h"
#include "Asset/MeshAsset.h"
#include "Asset/SceneAsset.h"
#include "Asset/TextureAsset.h"

#include <chrono>
#include "boost/property_tree/ini_parser.hpp"

namespace R
{

STRING_ENUM_CLASS_INST(GraphicApi)

#pragma region EngineComponent
//
// EngineComponent
//
EngineComponent::EngineComponent(std::shared_ptr<Scene> a_pRefScene, std::shared_ptr<SceneNode> a_pOwner)
	: m_bHidden(false)
	, m_Name(wxT(""))
	, m_pRefScene(a_pRefScene)
	, m_pOwner(a_pOwner)
{
	memset(&m_Flags, 0, sizeof(m_Flags));
}

EngineComponent::~EngineComponent()
{
}

void EngineComponent::postInit()
{
	if( nullptr != m_pOwner ) m_pOwner->add(shared_from_this());
}

void EngineComponent::setHidden(bool a_bHidden)
{
	if( a_bHidden == m_bHidden ) return ;
	m_bHidden = a_bHidden;
	hiddenFlagChanged();
}

void EngineComponent::setOwner(std::shared_ptr<SceneNode> a_pOwner)
{
	assert(nullptr != a_pOwner);
	
	bool l_bTransformListener = 0 == m_Flags.m_bTransformListener;
	if( nullptr != m_pOwner )
	{
		removeTransformListener();
		m_pOwner->remove(shared_from_this());
	}

	m_pRefScene = a_pOwner->getScene();
	m_pOwner = a_pOwner;
	m_pOwner->add(shared_from_this());

	if( l_bTransformListener )
	{
		addTransformListener();
	}
}

void EngineComponent::remove()
{
	end();
	m_pOwner->remove(shared_from_this());
	removeAllListener();
	
	m_pRefScene = nullptr;
	m_pOwner = nullptr;
}

void EngineComponent::detach()
{
	end();
	removeAllListener();

	m_pRefScene = nullptr;
	m_pOwner = nullptr;
}

void EngineComponent::addAllListener()
{
	addInputListener();
	addUpdateListener();
	addTransformListener();
}

void EngineComponent::removeAllListener()
{
	removeInputListener();
	removeUpdateListener();
	removeTransformListener();
}

void EngineComponent::addInputListener()
{
	if( 0 != m_Flags.m_bInputListener ) return;
	m_pRefScene->addInputListener(shared_from_this());
	m_Flags.m_bInputListener = 1;
}

void EngineComponent::removeInputListener()
{
	if( 0 == m_Flags.m_bInputListener ) return;
	m_pRefScene->removeInputListener(shared_from_this());
	m_Flags.m_bInputListener = 0;
}

void EngineComponent::addUpdateListener()
{
	if( 0 != m_Flags.m_bUpdateListener ) return;
	m_pRefScene->addUpdateListener(shared_from_this());
	m_Flags.m_bUpdateListener = 1;
}

void EngineComponent::removeUpdateListener()
{
	if( 0 == m_Flags.m_bUpdateListener ) return;
	m_pRefScene->removeUpdateListener(shared_from_this());
	m_Flags.m_bUpdateListener = 0;
}

void EngineComponent::addTransformListener()
{
	if( 0 != m_Flags.m_bTransformListener || nullptr == m_pOwner ) return;
	m_pOwner->addTranformListener(shared_from_this());
	transformListener(m_pOwner->getTransform());
	m_Flags.m_bTransformListener = 1;
}

void EngineComponent::removeTransformListener()
{
	if( 0 == m_Flags.m_bTransformListener || nullptr == m_pOwner ) return;
	m_pOwner->removeTranformListener(shared_from_this());
	m_Flags.m_bTransformListener = 0;
}
#pragma endregion

#pragma region EngineSetting
//
// EngineSetting
//
EngineSetting& EngineSetting::singleton()
{
	static EngineSetting s_Inst;
	return s_Inst;
}

EngineSetting::EngineSetting()
	: m_Title(wxT("REngine"))
	, m_Api(GraphicApi::d3d12)
	, m_DefaultSize(1280, 720)
	, m_bFullScreen(false)
	, m_FPS(60)
	, m_ShadowMapSize(8192)
	, m_LightMapSample(1048576)
	, m_LightMapSampleDepth(64)
	, m_LightMapSize(8192)
	, m_TileSize(16.0f)
	, m_NumRenderCommandList(20)
	, m_CDN(wxT(""))
{
	boost::property_tree::ptree l_IniFile;
	try
	{
		boost::property_tree::ini_parser::read_ini(CONIFG_FILE, l_IniFile);
	}
	catch( ... )
	{
		save();
		return;
	}

	m_Title = l_IniFile.get("General.Title", "REngine");

	m_Api = GraphicApi::fromString(l_IniFile.get("Graphic.Api", "d3d12"));
	m_DefaultSize.x = l_IniFile.get("Graphic.DefaultWidth", 1280);
	m_DefaultSize.y = l_IniFile.get("Graphic.DefaultHeight", 720);
	m_bFullScreen = l_IniFile.get("Graphic.FullScreen", false);
	m_FPS = l_IniFile.get("Graphic.FPS", 60);
	m_ShadowMapSize = l_IniFile.get("Graphic.ShadowMapSize", 8192);
	m_LightMapSample = l_IniFile.get("Graphic.LightMapSample", 1024576);
	m_LightMapSampleDepth = l_IniFile.get("Graphic.LightMapSampleDepth", 64);
	m_LightMapSize = l_IniFile.get("Graphic.LightMapSize", 8192);
	m_TileSize = l_IniFile.get("Graphic.TileSize", 16.0f);
	m_NumRenderCommandList = l_IniFile.get("Graphic.NumRenderCommandList", 20);

	std::vector<wxString> l_Path;
	splitString(wxT('|'), l_IniFile.get("Asset.ImportPath", "./"), l_Path);
	for( unsigned int i=0 ; i<l_Path.size() ; ++i )
	{
		ImageManager::singleton().addSearchPath(l_Path[i]);
		AnimationManager::singleton().addSearchPath(l_Path[i]);
		ModelManager::singleton().addSearchPath(l_Path[i]);
	}
	AssetManager::singleton().addSearchPath("./");

	m_CDN = l_IniFile.get("Asset.CDN", "");
	
	splitString(wxT('|'), l_IniFile.get("Asset.ResourcePath", "./"), l_Path);
	for( unsigned int i=0 ; i<l_Path.size() ; ++i )
	{
		
	}
}

EngineSetting::~EngineSetting()
{
}

void EngineSetting::save()
{
	boost::property_tree::ptree l_IniFile;
	
	l_IniFile.put("General.Title", m_Title.c_str());

	l_IniFile.put("Graphic.Api", GraphicApi::toString(m_Api));
	l_IniFile.put("Graphic.Width", m_DefaultSize.x);
	l_IniFile.put("Graphic.Height", m_DefaultSize.y);
	l_IniFile.put("Graphic.FullScreen", m_bFullScreen);
	l_IniFile.put("Graphic.FPS", m_FPS);
	l_IniFile.put("Graphic.ShadowMapSize", m_ShadowMapSize);
	l_IniFile.put("Graphic.LightMapSample", m_LightMapSample);
	l_IniFile.put("Graphic.LightMapSampleDepth", m_LightMapSampleDepth);
	l_IniFile.put("Graphic.LightMapSize", m_LightMapSize);
	l_IniFile.put("Graphic.TileSize", m_TileSize);
	l_IniFile.put("Graphic.NumRenderCommandList", m_NumRenderCommandList);

	l_IniFile.put("Asset.CDN", m_CDN);

	boost::property_tree::ini_parser::write_ini(CONIFG_FILE, l_IniFile);
}
#pragma endregion

#pragma region EngineCore
//
// EngineCore
//
EngineCore& EngineCore::singleton()
{
	static EngineCore l_Inst;
	if( !l_Inst.m_bValid )
	{
		if( !l_Inst.init() ) wxMessageBox(wxT("Invalid to initilize engine"));
	}
	return l_Inst;
}

EngineCore::EngineCore()
	: m_bValid(false)
	, m_bShutdown(false)
	, m_Delta(0.0)
	, m_WhiteTexture(nullptr), m_BlueTexture(nullptr), m_DarkgrayTexture(nullptr)
	, m_pQuadMesh(nullptr)
	, m_pInput(new InputMediator())
	, m_pRefMain(nullptr)
	, m_ThreadPool(std::max(std::thread::hardware_concurrency() - 2, 1u))// -2 -> -1 for resource loop, -1 for graphic loop
{
	SceneNode::registComponentReflector<Camera>();
	SceneNode::registComponentReflector<RenderableMesh>();
	SceneNode::registComponentReflector<OmniLight>();
	SceneNode::registComponentReflector<SpotLight>();
	SceneNode::registComponentReflector<DirLight>();

	Scene::registSceneGraphReflector<NoPartition>();
	Scene::registSceneGraphReflector<OctreePartition>();

	Scene::registRenderPipelineReflector<DeferredRenderer>();
	Scene::registRenderPipelineReflector<ShadowMapRenderer>();
}

EngineCore::~EngineCore()
{
	SAFE_DELETE(m_pInput)
}

GraphicCanvas* EngineCore::createCanvas()
{
	if( !m_bValid ) return nullptr;

	wxFrame *l_pNewWindow = new wxFrame(NULL, wxID_ANY, EngineSetting::singleton().m_Title);
	l_pNewWindow->SetClientSize(EngineSetting::singleton().m_DefaultSize.x, EngineSetting::singleton().m_DefaultSize.y);
	l_pNewWindow->Show();

	GraphicCanvas *l_pCanvas = GDEVICE()->canvasFactory(l_pNewWindow, wxID_ANY);
	l_pCanvas->SetClientSize(EngineSetting::singleton().m_DefaultSize.x, EngineSetting::singleton().m_DefaultSize.y);
	l_pCanvas->init(EngineSetting::singleton().m_bFullScreen);
	l_pCanvas->setResizeCallback(std::bind(&SceneManager::canvasResized, &SceneManager::singleton(), std::placeholders::_1, std::placeholders::_2));
	return l_pCanvas;
}

GraphicCanvas* EngineCore::createCanvas(wxWindow *a_pParent)
{
	if( !m_bValid ) return nullptr;

	GraphicCanvas *l_pCanvas = GDEVICE()->canvasFactory(a_pParent, wxID_ANY);
	l_pCanvas->init(EngineSetting::singleton().m_bFullScreen);
	l_pCanvas->setResizeCallback(std::bind(&SceneManager::canvasResized, &SceneManager::singleton(), std::placeholders::_1, std::placeholders::_2));
	return l_pCanvas;
}

void EngineCore::run(wxApp *a_pMain)
{
	assert(nullptr == m_pRefMain);

	m_pRefMain = a_pMain;
	m_pRefMain->Bind(wxEVT_IDLE, &EngineCore::mainLoop, this);
}

bool EngineCore::isShutdown()
{
	return m_bShutdown;
}

void EngineCore::shutDown()
{
	std::lock_guard<std::mutex> l_LoopGuard(m_LoopGuard);

	m_bShutdown = true;
	m_bValid = false;
	m_pRefMain->Unbind(wxEVT_IDLE, &EngineCore::mainLoop, this);
	
	SDL_Quit();

	m_WhiteTexture = nullptr;
	m_BlueTexture = nullptr;
	m_DarkgrayTexture = nullptr;
	m_pQuadMesh = nullptr;

	ImageManager::singleton().finalize();
	ModelManager::singleton().finalize();
	AnimationManager::singleton().finalize();
	AssetManager::singleton().finalize();
	AssetManager::singleton().waitAssetClear();

	GDEVICE()->shutdown();
}

void EngineCore::addJob(std::function<void()> a_Job)
{
	m_ThreadPool.addJob(a_Job);
}

void EngineCore::join()
{
	m_ThreadPool.join();
}

bool EngineCore::init()
{
	switch( EngineSetting::singleton().m_Api )
	{
		case GraphicApi::d3d11:
			//GraphicDeviceManager::singleton().init<D3D11Device>();
			//m_bValid = true;
			break;

		case GraphicApi::d3d12:
			GraphicDeviceManager::singleton().init<D3D12Device>();
			m_bValid = true;
			break;

		case GraphicApi::vulkan:
			//GraphicDeviceManager::singleton().init<VulkanDevice>();
			//m_bValid = true;
			break;

		default:break;
	}

	unsigned int l_Color[64];
	memset(l_Color, 0xffffffff, sizeof(unsigned int) * 64);
	m_WhiteTexture = AssetManager::singleton().createAsset(WHITE_TEXTURE_ASSET_NAME);
	m_WhiteTexture->getComponent<TextureAsset>()->initTexture(glm::ivec2(8, 8), PixelFormat::rgba8_unorm, 1, false, l_Color);
	m_WhiteTexture->getComponent<TextureAsset>()->generateMipmap(0, ProgramManager::singleton().getData(DefaultPrograms::GenerateMipmap2D));
	m_WhiteTexture->setRuntimeOnly(true);
	
	memset(l_Color, 0x0000ffff, sizeof(unsigned int) * 64);
	m_BlueTexture = AssetManager::singleton().createAsset(BLUE_TEXTURE_ASSET_NAME);
	m_BlueTexture->getComponent<TextureAsset>()->initTexture(glm::ivec2(8, 8), PixelFormat::rgba8_unorm, 1, false, l_Color);
	m_BlueTexture->getComponent<TextureAsset>()->generateMipmap(0, ProgramManager::singleton().getData(DefaultPrograms::GenerateMipmap2D));
	m_BlueTexture->setRuntimeOnly(true);

	memset(l_Color, 0x404040ff, sizeof(unsigned int) * 64);
	m_DarkgrayTexture = AssetManager::singleton().createAsset(DARK_GRAY_TEXTURE_ASSET_NAME);
	m_DarkgrayTexture->getComponent<TextureAsset>()->initTexture(glm::ivec2(8, 8), PixelFormat::rgba8_unorm, 1, false, l_Color);
	m_DarkgrayTexture->getComponent<TextureAsset>()->generateMipmap(0, ProgramManager::singleton().getData(DefaultPrograms::GenerateMipmap2D));
	m_DarkgrayTexture->setRuntimeOnly(true);

	m_pQuadMesh = AssetManager::singleton().createAsset(QUAD_MESH_ASSET_NAME);
	m_pQuadMesh->setRuntimeOnly(true);
	const glm::vec3 c_QuadVtx[] = {
		glm::vec3(-1.0f, 1.0f, 0.0f),
		glm::vec3(-1.0f, -1.0f, 0.0f),
		glm::vec3(1.0f, 1.0f, 0.0f),
		glm::vec3(1.0f, -1.0f, 0.0f)};
	const unsigned int c_QuadIdx[] = {0, 1, 2, 2, 1, 3};
	glm::vec3 *l_pQuadVtxBuff = nullptr;
	unsigned int *l_pQuadIdxDstBuff = nullptr;
	m_pQuadMesh->getComponent<MeshAsset>()->init(VTXFLAG_POSITION);
	MeshAsset::Instance *l_pInstanceData = m_pQuadMesh->getComponent<MeshAsset>()->addSubMesh(4, 6, &l_pQuadIdxDstBuff, &l_pQuadVtxBuff);
	l_pInstanceData->m_IndexCount = 6;
	l_pInstanceData->m_Name = QUAD_MESH_ASSET_NAME;
	l_pInstanceData->m_VtxFlag = VTXFLAG_POSITION | VTXFLAG_WITHOUT_VP_MAT;
	l_pInstanceData->m_VisibleBoundingBox = glm::aabb(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(FLT_MAX, FLT_MAX, FLT_MAX));
	memcpy(l_pQuadVtxBuff, c_QuadVtx, sizeof(c_QuadVtx));
	memcpy(l_pQuadIdxDstBuff, c_QuadIdx, sizeof(c_QuadIdx));
	m_pQuadMesh->getComponent<MeshAsset>()->updateMeshData(glm::ivec2(0, 4), glm::ivec2(0, 6));

	SDL_Init(SDL_INIT_AUDIO | SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC | SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS);
	
	return m_bValid;
}

void EngineCore::mainLoop(wxIdleEvent &a_Event)
{
	std::lock_guard<std::mutex> l_LoopGuard(m_LoopGuard);
	if( m_bShutdown ) return;

	static auto l_PrevTick = std::chrono::high_resolution_clock::now();
	auto l_Now = std::chrono::high_resolution_clock::now();
	m_Delta = std::chrono::duration<double, std::milli>(l_Now - l_PrevTick).count() / 1000.0f;
	
	SceneManager::singleton().update(m_Delta);
	m_pInput->pollEvent();
	if( m_Delta >= 1.0f/EngineSetting::singleton().m_FPS )
	{
		BEGIN_TIME_RECORD()
		SceneManager::singleton().render();
		l_PrevTick = l_Now;
		END_TIME_RECORD("render time")
	}
		
	a_Event.RequestMore();
}
#pragma endregion

}