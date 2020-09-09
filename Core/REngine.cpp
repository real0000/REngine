// REngine.cpp
//
// 2014/03/12 Ian Wu/Real0000
//

#include "CommonUtil.h"
#include "RImporters.h"
#include "RGDeviceWrapper.h"
#include "Core.h"
#include "Input/InputMediator.h"
#include "Scene/Scene.h"
#include "Asset/AssetBase.h"
#include "Asset/MaterialAsset.h"
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
EngineComponent::EngineComponent(SharedSceneMember *a_pSharedMember, std::shared_ptr<SceneNode> a_pOwner)
	: m_bHidden(false)
	, m_Name(wxT(""))
	, m_pMembers(new SharedSceneMember)
{
	*m_pMembers = *a_pSharedMember;
	m_pMembers->m_pSceneNode = a_pOwner;
	memset(&m_Flags, 0, sizeof(m_Flags));
}

EngineComponent::~EngineComponent()
{
}

void EngineComponent::postInit()
{
	if( nullptr != m_pMembers->m_pSceneNode ) m_pMembers->m_pSceneNode->add(shared_from_this());
}

void EngineComponent::setHidden(bool a_bHidden)
{
	if( a_bHidden == m_bHidden ) return ;
	m_bHidden = a_bHidden;
	hiddenFlagChanged();
}

std::shared_ptr<SceneNode> EngineComponent::getOwnerNode()
{
	return m_pMembers->m_pSceneNode;
}

void EngineComponent::setOwner(std::shared_ptr<SceneNode> a_pOwner)
{
	assert(nullptr != a_pOwner);
	
	bool l_bTransformListener = 0 != m_Flags.m_bTransformListener;
	removeTransformListener();

	m_pMembers->m_pSceneNode->remove(shared_from_this());
	m_pMembers->m_pSceneNode = a_pOwner;
	m_pMembers->m_pSceneNode->add(shared_from_this());

	if( l_bTransformListener )
	{
		addTransformListener();
		transformListener(a_pOwner->getTransform());
	}
}

void EngineComponent::remove()
{
	end();
	m_pMembers->m_pSceneNode->remove(shared_from_this());
	removeAllListener();
	SAFE_DELETE(m_pMembers)
}

void EngineComponent::detach()
{
	end();
	removeAllListener();
	SAFE_DELETE(m_pMembers)
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
	m_pMembers->m_pScene->addInputListener(shared_from_this());
	m_Flags.m_bInputListener = 1;
}

void EngineComponent::removeInputListener()
{
	if( 0 == m_Flags.m_bInputListener ) return;
	m_pMembers->m_pScene->removeInputListener(shared_from_this());
	m_Flags.m_bInputListener = 0;
}

void EngineComponent::addUpdateListener()
{
	if( 0 != m_Flags.m_bUpdateListener ) return;
	m_pMembers->m_pScene->addUpdateListener(shared_from_this());
	m_Flags.m_bUpdateListener = 1;
}

void EngineComponent::removeUpdateListener()
{
	if( 0 == m_Flags.m_bUpdateListener ) return;
	m_pMembers->m_pScene->removeUpdateListener(shared_from_this());
	m_Flags.m_bUpdateListener = 0;
}

void EngineComponent::addTransformListener()
{
	if( 0 != m_Flags.m_bTransformListener ) return;
	m_pMembers->m_pSceneNode->addTranformListener(shared_from_this());
	m_Flags.m_bTransformListener = 1;
}

void EngineComponent::removeTransformListener()
{
	if( 0 == m_Flags.m_bTransformListener ) return;
	m_pMembers->m_pSceneNode->removeTranformListener(shared_from_this());
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
	, m_LightMapSample(1024576)
	, m_LightMapSampleDepth(64)
	, m_LightMapSize(8192)
	, m_TileSize(16.0f)
	, m_NumRenderCommandList(20)
	, m_OmniMaterial(wxT("OmniShadowMap.Material"))
	, m_SpotMaterial(wxT("SpotShadowMap.Material"))
	, m_DirMaterial(wxT("DirShadowMap.Material"))
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

	m_OmniMaterial = l_IniFile.get("Graphic.OmniShadowMap", "OmniShadowMap.Material");
	if( !wxFileExists(m_OmniMaterial) )
	{
		std::shared_ptr<Asset> l_pShadowMapMat = AssetManager::singleton().createAsset(m_OmniMaterial).second;
		MaterialAsset *l_pMaterialInst = l_pShadowMapMat->getComponent<MaterialAsset>();
		l_pMaterialInst->init(ProgramManager::singleton().getData(DefaultPrograms::OmniShadowMap));
		AssetManager::singleton().saveAsset(l_pShadowMapMat);
		l_pShadowMapMat = nullptr;
	}

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
	l_IniFile.put("Graphic.OmniShadowMap", m_OmniMaterial);

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
	, m_WhiteTexture(std::make_pair(-1, nullptr)), m_pQuad(nullptr)
	, m_pInput(new InputMediator())
{
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
	return l_pCanvas;
}

GraphicCanvas* EngineCore::createCanvas(wxWindow *a_pParent)
{
	if( !m_bValid ) return nullptr;

	GraphicCanvas *l_pCanvas = GDEVICE()->canvasFactory(a_pParent, wxID_ANY);
	l_pCanvas->init(EngineSetting::singleton().m_bFullScreen);
	return l_pCanvas;
}

bool EngineCore::isShutdown()
{
	return m_bShutdown;
}

void EngineCore::shutDown()
{
	m_bShutdown = true;
	m_bValid = false;
	
	m_MainLoop.join();
	m_WhiteTexture.second = nullptr;
	m_pQuad = nullptr;

	ImageManager::singleton().finalize();
	ModelManager::singleton().finalize();
	AnimationManager::singleton().finalize();
	AssetManager::singleton().finalize();

	GDEVICE()->shutdown();
	SDL_Quit();
}

bool EngineCore::init()
{
	SDL_Init(SDL_INIT_AUDIO | SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC | SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS);
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

	unsigned int l_White[64];
	memset(l_White, 0xffffffff, sizeof(unsigned int) * 64);
	m_WhiteTexture = AssetManager::singleton().createAsset(wxT("Default_White.Image"));
	m_WhiteTexture.second->getComponent<TextureAsset>()->initTexture(glm::ivec2(8, 8), PixelFormat::rgba8_unorm, 1, false, l_White);
	m_WhiteTexture.second->getComponent<TextureAsset>()->generateMipmap(0, ProgramManager::singleton().getData(DefaultPrograms::GenerateMipmap2D));

	m_pQuad = std::shared_ptr<VertexBuffer>(new VertexBuffer());
	const glm::vec3 c_QuadVtx[] = {
		glm::vec3(-1.0f, 1.0f, 0.0f),
		glm::vec3(-1.0f, -1.0f, 0.0f),
		glm::vec3(1.0f, 1.0f, 0.0f),
		glm::vec3(1.0f, -1.0f, 0.0f)};
	m_pQuad->setNumVertex(4);
	m_pQuad->setVertex(VTXSLOT_POSITION, (void *)c_QuadVtx);
	m_pQuad->init();

	if( m_bValid ) m_MainLoop = std::thread(&EngineCore::mainLoop, this);
	
	return m_bValid;
}

void EngineCore::mainLoop()
{
	auto l_Start = std::chrono::high_resolution_clock::now();
	while( !m_bShutdown )
	{
		auto l_Now = std::chrono::high_resolution_clock::now();
		auto l_Delta = std::chrono::duration<double, std::milli>(l_Now - l_Start).count();
		SceneManager::singleton().update(l_Delta / 1000.0f);
		if( l_Delta < 1000.0f/EngineSetting::singleton().m_FPS )
		{
			std::this_thread::yield();
			continue;
		}

		l_Delta *= 0.001f;// to second
		{
			m_pInput->pollEvent();
			SceneManager::singleton().render();
		}

		l_Start = l_Now;
	}
}
#pragma endregion

}