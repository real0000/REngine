// REngine.cpp
//
// 2014/03/12 Ian Wu/Real0000
//

#include "CommonUtil.h"
#include "RGDeviceWrapper.h"
#include "Core.h"
#include "Canvas.h"
#include "Scene/Scene.h"

#include <chrono>
#include "boost/property_tree/ini_parser.hpp"

namespace R
{

STRING_ENUM_CLASS_INST(GraphicApi)

#pragma region EngineComponent
//
// EngineComponent
//
EngineComponent::EngineComponent(std::shared_ptr<SceneNode> a_pOwner)
	: m_Name(wxT(""))
	, m_pOwner(a_pOwner)
{
	m_pOwner->add(shared_from_this());
}

EngineComponent::~EngineComponent()
{
}

void EngineComponent::setOwnerNode(std::shared_ptr<SceneNode> a_pOwner)
{
	assert(a_pOwner);
	m_pOwner->remove(shared_from_this());
	m_pOwner = a_pOwner;
	m_pOwner->add(shared_from_this());
}

void EngineComponent::remove()
{
	m_pOwner->remove(shared_from_this());
	m_pOwner = nullptr;
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
{
	boost::property_tree::ptree l_IniFile;
	boost::property_tree::ini_parser::read_ini(CONIFG_FILE, l_IniFile);
	if( l_IniFile.empty() ) return;

	m_Title = l_IniFile.get("General.Title", "REngine");

	m_Api = GraphicApi::fromString(l_IniFile.get("Graphic.Api", "d3d12"));
	m_DefaultSize.x = l_IniFile.get("Graphic.DefaultWidth", 1280);
	m_DefaultSize.y = l_IniFile.get("Graphic.DefaultHeight", 720);
	m_bFullScreen = l_IniFile.get("Graphic.FullScreen", false);
	m_FPS = l_IniFile.get("Graphic.FPS", 60);
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
	l_IniFile.put("Graphic.FPS", 60);

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
	, m_pMainLoop(nullptr)
{
}

EngineCore::~EngineCore()
{
	if( !m_bValid ) return;

	if( nullptr != m_pMainLoop )
	{
		delete m_pMainLoop;
		m_pMainLoop = nullptr;
	}
}

bool EngineCore::init()
{
	switch( EngineSetting::singleton().m_Api )
	{
		case GraphicApi::d3d11:
			//GraphicDeviceManager::singleton().init<D3D12Device>();
			//m_bValid = true;
			break;

		case GraphicApi::d3d12:
			GraphicDeviceManager::singleton().init<D3D12Device>();
			m_bValid = true;
			break;

		case GraphicApi::vulkan:
			//GraphicDeviceManager::singleton().init<D3D12Device>();
			//m_bValid = true;
			break;

		default:break;
	}
	if( m_bValid ) m_pMainLoop = new std::thread(&EngineCore::mainLoop, this);
	
	return m_bValid;
}

EngineCanvas* EngineCore::createCanvas()
{
	if( !m_bValid ) return nullptr;
	std::lock_guard<std::mutex> l_CanvasLock(m_CanvasLock);

	wxFrame *l_pNewWindow = new wxFrame(NULL, wxID_ANY, EngineSetting::singleton().m_Title);
	l_pNewWindow->Show();

	EngineCanvas *l_pCanvas = new EngineCanvas(l_pNewWindow);
	l_pCanvas->SetClientSize(EngineSetting::singleton().m_DefaultSize.x, EngineSetting::singleton().m_DefaultSize.y);
	l_pCanvas->setFullScreen(EngineSetting::singleton().m_bFullScreen);
	l_pCanvas->init();
	
	m_ManagedCanvas.insert(l_pCanvas);
	return l_pCanvas;
}

EngineCanvas* EngineCore::createCanvas(wxWindow *a_pParent)
{
	if( !m_bValid ) return nullptr;
	std::lock_guard<std::mutex> l_CanvasLock(m_CanvasLock);

	EngineCanvas *l_pCanvas = new EngineCanvas(a_pParent);
	l_pCanvas->init();
	m_ManagedCanvas.insert(l_pCanvas);
	return l_pCanvas;
}

void EngineCore::destroyCanvas(EngineCanvas *a_pCanvas)
{
	if( !m_bValid ) return;
	std::lock_guard<std::mutex> l_CanvasLock(m_CanvasLock);

	a_pCanvas->getCommander()->flush();
	m_ManagedCanvas.erase(a_pCanvas);
	delete a_pCanvas;
}

bool EngineCore::isShutdown()
{
	return m_bShutdown;
}

void EngineCore::shutDown()
{
	m_bShutdown = true;
	m_bValid = false;
	m_pMainLoop->join();
	for( auto it = m_ManagedCanvas.begin() ; it != m_ManagedCanvas.end() ; ++it ) delete *it;
	m_ManagedCanvas.clear();
}

void EngineCore::mainLoop()
{
	auto l_Start = std::chrono::high_resolution_clock::now();
	while( !m_bShutdown )
	{
		auto l_Now = std::chrono::high_resolution_clock::now();
		auto l_Delta = std::chrono::duration<double, std::milli>(l_Now - l_Start).count();
		if( l_Delta < 1000.0f/EngineSetting::singleton().m_FPS )
		{
			std::this_thread::yield();
			continue;
		}

		l_Delta *= 0.001f;// to second
		{
			std::lock_guard<std::mutex> l_CanvasLock(m_CanvasLock);
			
			for( auto it = m_ManagedCanvas.begin() ; it != m_ManagedCanvas.end() ; ++it ) (*it)->processInput();
			SceneManager::singleton().update(l_Delta);
			SceneManager::singleton().render();
		}

		l_Start = l_Now;
	}
}
#pragma endregion

}