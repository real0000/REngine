// Deferred.cpp
//
// 2017/08/14 Ian Wu/Real0000
//

#include "CommonUtil.h"
#include "RGDeviceWrapper.h"

#include "Core.h"
#include "Asset/LightmapAsset.h"
#include "Asset/MaterialAsset.h"
#include "Asset/MeshAsset.h"
#include "Asset/TextureAsset.h"
#include "RenderObject/Mesh.h"
#include "RenderObject/Light.h"
#include "RenderObject/TextureAtlas.h"
#include "Scene/Camera.h"
#include "Scene/Scene.h"
#include "Scene/Graph/ScenePartition.h"
#include "Scene/RenderPipline/ShadowMap.h"

#include "Deferred.h"

namespace R
{

const std::pair<wxString, PixelFormat::Key> c_GBufferDef[] = {
	{DEFFERRED_GBUFFER_NORMAL_ASSET_NAME, PixelFormat::rgba8_snorm},
	{DEFFERRED_GBUFFER_MATERIAL_ASSET_NAME, PixelFormat::rgba8_unorm},
	{DEFFERRED_GBUFFER_BASE_COLOR_ASSET_NAME, PixelFormat::rgba8_unorm},
	{DEFFERRED_GBUFFER_MASK_ASSET_NAME, PixelFormat::rgba8_unorm},
	{DEFFERRED_GBUFFER_FACTOR_ASSET_NAME, PixelFormat::rgba8_unorm},
	{DEFFERRED_GBUFFER_MOTION_BLUR_ASSET_NAME, PixelFormat::rg16_float},
	{DEFFERRED_GBUFFER_DEPTH_ASSET_NAME, PixelFormat::d24_unorm_s8_uint}};

#pragma region DeferredRenderer
#pragma region DebugTexture
//
// DebugTexture
//
DeferredRenderer::DebugTexture::DebugTexture()
	: m_pComponent(nullptr)
	, m_pMat(nullptr)
	, m_MaterialName(wxT(""))
{
}

DeferredRenderer::DebugTexture::~DebugTexture()
{
	AssetManager::singleton().removeData(m_MaterialName);
	m_pMat = nullptr;
}

void DeferredRenderer::DebugTexture::init(std::shared_ptr<SceneNode> a_pNode, wxString a_MaterialName, std::shared_ptr<Asset> a_pTexture, glm::vec4 a_DockParam)
{
	assert(nullptr == m_pComponent);

	m_pComponent = a_pNode->addComponent<RenderableMesh>();
	m_pComponent->setMesh(AssetManager::singleton().getAsset(QUAD_MESH_ASSET_NAME), 0);

	m_MaterialName = a_MaterialName;
	m_pMat = AssetManager::singleton().createAsset(a_MaterialName);
	MaterialAsset *l_pMatInst = m_pMat->getComponent<MaterialAsset>();
	l_pMatInst->init(ProgramManager::singleton().getData(DefaultPrograms::CopyFrame));
	l_pMatInst->setTexture("m_SrcTex", a_pTexture);
	l_pMatInst->setParam("c_DockParam", 0, a_DockParam);

	m_pComponent->setMaterial(MATSLOT_TRANSPARENT, m_pMat);
}

void DeferredRenderer::DebugTexture::uninit()
{
	assert(nullptr != m_pComponent);

	m_pComponent->remove();
	AssetManager::singleton().removeData(m_MaterialName);
	m_pMat = nullptr;
}
#pragma endregion
//
// DeferredRenderer
//
DeferredRenderer* DeferredRenderer::create(boost::property_tree::ptree &a_Src, std::shared_ptr<Scene> a_pScene)
{
	return new DeferredRenderer(a_pScene);
}

unsigned int DeferredRenderer::sm_Seiral = 0;
DeferredRenderer::DeferredRenderer(std::shared_ptr<Scene> a_pScene)
	: RenderPipeline(a_pScene)
	, m_pCmdInit(nullptr)
	, m_LightIdx(nullptr)
	, m_ExtendSize(INIT_CONTAINER_SIZE)
	, m_TileDim(0, 0)
	, m_pQuadMesh(AssetManager::singleton().getAsset(QUAD_MESH_ASSET_NAME))
	, m_pQuadMeshInst(nullptr)
	, m_pFrameBuffer(nullptr)
	, m_pDepthMinmax(nullptr)
	, m_MinmaxStepCount(1)
	, m_pLightIndexMat(nullptr)
	, m_pDeferredLightMat(nullptr)
	, m_pCopyMat(AssetManager::singleton().createAsset(COPY_ASSET_NAME))
	, m_pLightIndexMatInst(nullptr), m_pDeferredLightMatInst(nullptr), m_pCopyMatInst(nullptr)
	, m_Serial(sm_Seiral++)
{
	m_pLightIndexMat = AssetManager::singleton().createAsset(wxString::Format(DEFFERRED_LIGHTINDEX_ASSET_NAME, m_Serial));
	m_pDeferredLightMat = AssetManager::singleton().createAsset(wxString::Format(DEFFERRED_LIGHTING_ASSET_NAME, m_Serial));

	m_pQuadMeshInst = m_pQuadMesh->getComponent<MeshAsset>();
	m_pLightIndexMatInst = m_pLightIndexMat->getComponent<MaterialAsset>();
	m_pDeferredLightMatInst = m_pDeferredLightMat->getComponent<MaterialAsset>();
	m_pCopyMatInst = m_pCopyMat->getComponent<MaterialAsset>();

	m_pLightIndexMatInst->init(ProgramManager::singleton().getData(DefaultPrograms::TiledLightIntersection));
	m_pDeferredLightMatInst->init(ProgramManager::singleton().getData(DefaultPrograms::TiledDeferredLighting));
	m_pCopyMatInst->init(ProgramManager::singleton().getData(DefaultPrograms::Copy));

	for( unsigned int i=0 ; i<GBUFFER_COUNT ; ++i )
	{
		m_pGBuffer[i] = AssetManager::singleton().createAsset(wxString::Format(c_GBufferDef[i].first, m_Serial));
		m_pGBuffer[i]->getComponent<TextureAsset>()->initRenderTarget(EngineSetting::singleton().m_DefaultSize, c_GBufferDef[i].second);

		char l_Buff[8];
		snprintf(l_Buff, 8, "GBuff%d", i);
		m_pDeferredLightMatInst->setTexture(l_Buff, m_pGBuffer[i]);
	}
	m_pFrameBuffer = AssetManager::singleton().createAsset(wxString::Format(DEFFERRED_FRAMEBUFFER_ASSET_NAME, m_Serial));
	m_pFrameBuffer->getComponent<TextureAsset>()->initRenderTarget(EngineSetting::singleton().m_DefaultSize, PixelFormat::rgba16_float);

	m_LightIdx = m_pLightIndexMatInst->createExternalBlock(ShaderRegType::UavBuffer, "g_SrcLights", m_ExtendSize / 2); // {index, type}

	m_pCmdInit = GDEVICE()->commanderFactory();

	m_TileDim.x = std::ceil(EngineSetting::singleton().m_DefaultSize.x / EngineSetting::singleton().m_TileSize);
	m_TileDim.y = std::ceil(EngineSetting::singleton().m_DefaultSize.y / EngineSetting::singleton().m_TileSize);
	
	m_pDepthMinmax = AssetManager::singleton().createAsset(wxString::Format(DEFFERRED_DEPTHMINMAX_ASSET_NAME, m_Serial));
	m_pDepthMinmax->getComponent<TextureAsset>()->initRenderTarget(EngineSetting::singleton().m_DefaultSize, PixelFormat::rg16_float);
	m_MinmaxStepCount = std::ceill(log2f(EngineSetting::singleton().m_TileSize));
	m_TiledValidLightIdx = m_pLightIndexMatInst->createExternalBlock(ShaderRegType::UavBuffer, "g_DstLights", m_TileDim.x * m_TileDim.y * (INIT_LIGHT_SIZE / 2 + 1)); // {index, type}

	m_pLightIndexMatInst->setParam<glm::vec2>("c_PixelSize", 0, glm::vec2(1.0f / EngineSetting::singleton().m_DefaultSize.x, 1.0f / EngineSetting::singleton().m_DefaultSize.y));
	m_pLightIndexMatInst->setParam<int>("c_MipmapLevel", 0, (int)m_MinmaxStepCount);
	m_pLightIndexMatInst->setParam<glm::ivec2>("c_TileCount", 0, m_TileDim);
	m_pLightIndexMatInst->setBlock("g_SrcLights", m_LightIdx);
	m_pLightIndexMatInst->setBlock("g_DstLights", m_TiledValidLightIdx);
	m_pLightIndexMatInst->setBlock("g_OmniLights", a_pScene->getOmniLightContainer()->getMaterialBlock());
	m_pLightIndexMatInst->setBlock("g_SpotLights", a_pScene->getSpotLightContainer()->getMaterialBlock());
	m_pLightIndexMatInst->setTexture("g_MinMapTexture", m_pDepthMinmax);

	m_pDeferredLightMatInst->setParam<glm::ivec2>("c_TileCount", 0, m_TileDim);
	m_pDeferredLightMatInst->setParam<int>("c_BoxLevel", 0, a_pScene->getLightmap()->getComponent<LightmapAsset>()->getMaxBoxLevel());
	m_pDeferredLightMatInst->setBlock("Boxes", a_pScene->getLightmap()->getComponent<LightmapAsset>()->getBoxes());
	m_pDeferredLightMatInst->setBlock("Harmonics", a_pScene->getLightmap()->getComponent<LightmapAsset>()->getHarmonics());
	m_pDeferredLightMatInst->setBlock("TileLights", m_TiledValidLightIdx);
	m_pDeferredLightMatInst->setBlock("DirLights", a_pScene->getDirLightContainer()->getMaterialBlock());
    m_pDeferredLightMatInst->setBlock("OmniLights", a_pScene->getOmniLightContainer()->getMaterialBlock());
    m_pDeferredLightMatInst->setBlock("SpotLights", a_pScene->getSpotLightContainer()->getMaterialBlock());
	
	ShadowMapRenderer *l_pShadowMap = reinterpret_cast<ShadowMapRenderer *>(getScene()->getShadowMapBaker());
	glm::vec2 l_ShadowMapSize;
	l_ShadowMapSize.x = l_ShadowMapSize.y = l_pShadowMap->getShadowMap()->getComponent<TextureAsset>()->getDimension().x;
	m_pDeferredLightMatInst->setParam<glm::vec2>("c_ShadowMapSize", 0, l_ShadowMapSize);

	m_pCopyMatInst->setTexture("m_SrcTex", m_pFrameBuffer);

	m_DrawCommand.resize(EngineSetting::singleton().m_NumRenderCommandList);
	for( unsigned int i=0 ; i<m_DrawCommand.size() ; ++i ) m_DrawCommand[i] = GDEVICE()->commanderFactory();
}

DeferredRenderer::~DeferredRenderer()
{
	AssetManager::singleton().removeData(wxString::Format(DEFFERRED_FRAMEBUFFER_ASSET_NAME, m_Serial));
	AssetManager::singleton().removeData(wxString::Format(DEFFERRED_DEPTHMINMAX_ASSET_NAME, m_Serial));

	m_pQuadMeshInst = nullptr;
	m_pQuadMesh = nullptr;
	m_pDepthMinmax = nullptr;
	m_pFrameBuffer = nullptr;
	for( unsigned int i=0 ; i<GBUFFER_COUNT ; ++i )
	{
		AssetManager::singleton().removeData(wxString::Format(c_GBufferDef[i].first, m_Serial));
		m_pGBuffer[i] = nullptr;
	}

	SAFE_DELETE(m_pCmdInit);
	for( unsigned int i=0 ; i<m_DrawCommand.size() ; ++i ) SAFE_DELETE(m_DrawCommand[i])
	m_DrawCommand.clear();
	// add memset ?

	m_pLightIndexMatInst = nullptr;
	m_pDeferredLightMatInst = nullptr;
	m_pCopyMatInst = nullptr;
	m_pDeferredLightMat = nullptr;
	m_pLightIndexMat = nullptr;
	m_LightIdx = nullptr;
	m_TiledValidLightIdx = nullptr;
}

void DeferredRenderer::saveSetting(boost::property_tree::ptree &a_Dst)
{
	boost::property_tree::ptree l_Attr;
	l_Attr.add("type", DeferredRenderer::typeName());

	a_Dst.add_child("<xmlattr>", l_Attr);
}

void DeferredRenderer::render(std::shared_ptr<Camera> a_pCamera, GraphicCanvas *a_pCanvas)
{
	m_VisibleMeshes.clear();
	m_VisibleLights.clear();

	{// setup light map
		std::shared_ptr<Asset> l_pLightmap = getScene()->getLightmap();
		LightmapAsset *l_pLightmapInst = l_pLightmap->getComponent<LightmapAsset>();
		if( l_pLightmapInst->isBaking() )
		{
			l_pLightmapInst->stepBake(m_pCmdInit);
			if( !l_pLightmapInst->isBaking() )
			{
				m_pDeferredLightMatInst->setParam<int>("c_BoxLevel", 0, getScene()->getLightmap()->getComponent<LightmapAsset>()->getMaxBoxLevel());
				m_pDeferredLightMatInst->setBlock("Boxes", getScene()->getLightmap()->getComponent<LightmapAsset>()->getBoxes());
				m_pDeferredLightMatInst->setBlock("Harmonics", getScene()->getLightmap()->getComponent<LightmapAsset>()->getHarmonics());
			}
		}
	}

	if( !setupVisibleList(a_pCamera) )
	{
		// clear backbuffer only if mesh list is empty
		m_pCmdInit->begin(false);

		if( nullptr == a_pCanvas )
		{
			// to do : clear camera texture?
		}
		else
		{
			m_pCmdInit->setRenderTargetWithBackBuffer(-1, a_pCanvas);
			m_pCmdInit->clearBackBuffer(a_pCanvas, glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
		}
		m_pCmdInit->end();

		if( nullptr != a_pCanvas ) a_pCanvas->present();
		return;
	}
	
	EngineCore::singleton().addJob([=](){ this->setupIndexUav();});
	
	// sort meshes
	{
		for( unsigned int i=0 ; i<cm_NumMatSlot ; ++i )
		{
			EngineCore::singleton().addJob([=]() -> void
			{
				m_SortedMesh[i].clear();
				m_SortedMesh[i].reserve(m_VisibleMeshes.size());
				for( unsigned int j=0 ; j<m_VisibleMeshes.size() ; ++j )
				{
					RenderableMesh *l_pMesh = reinterpret_cast<RenderableMesh*>(m_VisibleMeshes[j].get());
					if( l_pMesh->getSortKey(i).m_Members.m_bValid ) m_SortedMesh[i].push_back(l_pMesh);
				}
				std::sort(m_SortedMesh[i].begin(), m_SortedMesh[i].end(), [=](RenderableMesh *a_pLeft, RenderableMesh *a_pRight) -> bool
				{
					return a_pLeft->getSortKey(i).m_Key < a_pRight->getSortKey(i).m_Key;
				});
			});
		}
	}

	// flush view port param
	glm::vec2 l_ViewPortSize(m_pGBuffer[0]->getComponent<TextureAsset>()->getDimension().x, m_pGBuffer[0]->getComponent<TextureAsset>()->getDimension().y);
	{
		glm::vec4 l_CameraParam(a_pCamera->getViewParam());
		glm::mat4x4 l_Dummy(1.0f);
		switch( a_pCamera->getCameraType() )
		{
			case Camera::ORTHO:
				a_pCamera->setOrthoView(l_ViewPortSize.x, l_ViewPortSize.y, l_CameraParam.z, l_CameraParam.w, l_Dummy);
				break;

			case Camera::PERSPECTIVE:
				a_pCamera->setPerspectiveView(l_CameraParam.x, l_ViewPortSize.x / l_ViewPortSize.y, l_CameraParam.z, l_Dummy);
				break;
		}
	}

	EngineCore::singleton().join();

	// shadow map render
	//ShadowMapRenderer *l_pShadowMap = reinterpret_cast<ShadowMapRenderer *>(getScene()->getShadowMapBaker());
	//l_pShadowMap->bake(m_VisibleLights, m_SortedMesh[MATSLOT_DIR_SHADOWMAP], m_SortedMesh[MATSLOT_OMNI_SHADOWMAP], m_SortedMesh[MATSLOT_SPOT_SHADOWMAP], m_pCmdInit, m_DrawCommand);
	
	{// graphic step, divide by stage
		//bind gbuffer
		m_pCmdInit->begin(false);

		for( unsigned int i=0 ; i<GBUFFER_COUNT - 1 ; ++i )
		{
			m_pCmdInit->clearRenderTarget(m_pGBuffer[i]->getComponent<TextureAsset>()->getTextureID(), glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
		}
		m_pCmdInit->clearDepthTarget(m_pGBuffer[GBUFFER_DEPTH]->getComponent<TextureAsset>()->getTextureID(), true, 1.0f, false, 0);

		m_pCmdInit->end();

		// draw gbuffer
		unsigned int l_NumCommand = m_DrawCommand.size();
		for( unsigned int i=0 ; i<m_DrawCommand.size() ; ++i )
		{
			EngineCore::singleton().addJob([=]() -> void
			{
				m_DrawCommand[i]->begin(false);
				
				glm::viewport l_Viewport(0.0f, 0.0f, l_ViewPortSize.x, l_ViewPortSize.y, 0.0f, 1.0f);
				m_DrawCommand[i]->setViewPort(1, l_Viewport);
				m_DrawCommand[i]->setScissor(1, glm::ivec4(0, 0, l_ViewPortSize.x, l_ViewPortSize.y));
				m_DrawCommand[i]->setRenderTarget(m_pGBuffer[GBUFFER_DEPTH]->getComponent<TextureAsset>()->getTextureID(), 6,
												m_pGBuffer[GBUFFER_NORMAL]->getComponent<TextureAsset>()->getTextureID(),
												m_pGBuffer[GBUFFER_MATERIAL]->getComponent<TextureAsset>()->getTextureID(),
												m_pGBuffer[GBUFFER_BASECOLOR]->getComponent<TextureAsset>()->getTextureID(),
												m_pGBuffer[GBUFFER_MASK]->getComponent<TextureAsset>()->getTextureID(),
												m_pGBuffer[GBUFFER_FACTOR]->getComponent<TextureAsset>()->getTextureID(),
												m_pGBuffer[GBUFFER_MOTIONBLUR]->getComponent<TextureAsset>()->getTextureID());

				getScene()->getRenderBatcher()->drawSortedMeshes(m_DrawCommand[i], m_SortedMesh[MATSLOT_OPAQUE]
					, i, l_NumCommand, MATSLOT_OPAQUE
					, [=](MaterialAsset *a_pMat) -> void
					{
						a_pMat->bindBlock(m_DrawCommand[i], "Camera", a_pCamera->getMaterialBlock());
					}
					, [=](std::vector<glm::ivec4> &a_Instance, unsigned int a_Idx) -> unsigned int
					{
						a_Instance.push_back(glm::ivec4(m_SortedMesh[MATSLOT_OPAQUE][a_Idx]->getWorldOffset(), 0, 0, 0));
						return 1;
					});
				m_DrawCommand[i]->end();
			});
		}

		EngineCore::singleton().join();

		{// copy depth data
			m_pCmdInit->begin(false);

			m_pCmdInit->useProgram(DefaultPrograms::CopyDepth);
			m_pCmdInit->setRenderTarget(-1, 1, m_pDepthMinmax->getComponent<TextureAsset>()->getTextureID());

			glm::ivec3 l_DepthSize(m_pDepthMinmax->getComponent<TextureAsset>()->getDimension());
			glm::viewport l_DepthView(0.0f, 0.0f, l_DepthSize.x, l_DepthSize.y, 0.0f, 1.0f);
			m_pCmdInit->setViewPort(1, l_DepthView);
			m_pCmdInit->setScissor(1, glm::ivec4(0, 0, l_DepthSize.x, l_DepthSize.y));

			m_pCmdInit->bindVertex(m_pQuadMeshInst->getVertexBuffer().get());
			m_pCmdInit->bindTexture(m_pGBuffer[GBUFFER_DEPTH]->getComponent<TextureAsset>()->getTextureID(), 0, GraphicCommander::BIND_RENDER_TARGET);
			m_pCmdInit->bindSampler(m_pGBuffer[GBUFFER_DEPTH]->getComponent<TextureAsset>()->getSamplerID(), 0);
			m_pCmdInit->setTopology(Topology::triangle_strip);
			m_pCmdInit->drawVertex(4, 0);

			m_pCmdInit->end();
		}

		m_pDepthMinmax->getComponent<TextureAsset>()->generateMipmap(m_MinmaxStepCount, ProgramManager::singleton().getData(DefaultPrograms::GenerateMinmaxDepth), false);

		m_pCmdInit->begin(true);
		
		m_pCmdInit->useProgram(DefaultPrograms::TiledLightIntersection);
		m_pLightIndexMatInst->setBlock("Camera", a_pCamera->getMaterialBlock());
		m_pLightIndexMatInst->setParam<int>("c_NumLight", 0, (int)m_VisibleLights.size());
		m_pLightIndexMatInst->bindAll(m_pCmdInit);
		m_pCmdInit->compute(std::max(m_TileDim.x / 8, 1), std::max(m_TileDim.y / 8, 1));

		m_pCmdInit->end();

		// draw frame buffer
		m_pCmdInit->begin(false);
		
		m_pCmdInit->useProgram(m_pDeferredLightMatInst->getProgram());
		m_pCmdInit->setRenderTarget(-1, 1, m_pFrameBuffer->getComponent<TextureAsset>()->getTextureID());
		m_pCmdInit->clearRenderTarget(m_pFrameBuffer->getComponent<TextureAsset>()->getTextureID(), glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));

		glm::ivec3 l_FrameSize(m_pFrameBuffer->getComponent<TextureAsset>()->getDimension());
		glm::viewport l_FrameView(0.0f, 0.0f, l_FrameSize.x, l_FrameSize.y, 0.0f, 1.0f);
		m_pCmdInit->setViewPort(1, l_FrameView);
		m_pCmdInit->setScissor(1, glm::ivec4(0, 0, l_FrameSize.x, l_FrameSize.y));
		m_pCmdInit->bindVertex(m_pQuadMeshInst->getVertexBuffer().get());
		m_pDeferredLightMatInst->setParam<int>("c_NumLight", 0, (int)m_VisibleLights.size());
		m_pDeferredLightMatInst->bindTexture(m_pCmdInit, "ShadowMap", reinterpret_cast<ShadowMapRenderer*>(getScene()->getShadowMapBaker())->getShadowMap());
		m_pDeferredLightMatInst->bindBlock(m_pCmdInit, "Camera", a_pCamera->getMaterialBlock());
		m_pDeferredLightMatInst->bindAll(m_pCmdInit);
		m_pCmdInit->setTopology(Topology::triangle_strip);
		m_pCmdInit->drawVertex(4, 0);

		m_pCmdInit->end();

		// draw transparent/post objects
		for( unsigned int i=0 ; i<m_DrawCommand.size() ; ++i )
		{
			EngineCore::singleton().addJob([=]() -> void
			{
				m_DrawCommand[i]->begin(false);
				
				glm::viewport l_Viewport(0.0f, 0.0f, l_ViewPortSize.x, l_ViewPortSize.y, 0.0f, 1.0f);
				m_DrawCommand[i]->setViewPort(1, l_Viewport);
				m_DrawCommand[i]->setScissor(1, glm::ivec4(0, 0, l_ViewPortSize.x, l_ViewPortSize.y));
				m_DrawCommand[i]->setRenderTarget(m_pGBuffer[GBUFFER_DEPTH]->getComponent<TextureAsset>()->getTextureID(), 1, m_pFrameBuffer->getComponent<TextureAsset>()->getTextureID());

				getScene()->getRenderBatcher()->drawSortedMeshes(m_DrawCommand[i], m_SortedMesh[MATSLOT_TRANSPARENT]
					, i, l_NumCommand, MATSLOT_TRANSPARENT
					, [=](MaterialAsset *a_pMat) -> void
					{
						a_pMat->bindBlock(m_DrawCommand[i], "Camera", a_pCamera->getMaterialBlock());
					}
					, [=](std::vector<glm::ivec4> &a_Instance, unsigned int a_Idx) -> unsigned int
					{
						a_Instance.push_back(glm::ivec4(m_SortedMesh[MATSLOT_TRANSPARENT][a_Idx]->getWorldOffset(), 0, 0, 0));
						return 1;
					});

				m_DrawCommand[i]->end();
			});
		}
		EngineCore::singleton().join();
		
		if( nullptr != a_pCanvas )
		{
			// to do : draw post process ?

			// temp : copy only
			m_pCmdInit->begin(false);

			m_pCmdInit->useProgram(m_pCopyMatInst->getProgram());
			m_pCmdInit->setRenderTargetWithBackBuffer(-1, a_pCanvas);
			m_pCmdInit->clearBackBuffer(a_pCanvas, glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
			
			wxSize l_CanvasSize(a_pCanvas->GetClientSize());
			glm::viewport l_CanvasView(0.0f, 0.0f, l_CanvasSize.x, l_CanvasSize.y, 0.0f, 1.0f);
			m_pCmdInit->setViewPort(1, l_CanvasView);
			m_pCmdInit->setScissor(1, glm::ivec4(0, 0, l_CanvasSize.x, l_CanvasSize.y));

			m_pCmdInit->bindVertex(m_pQuadMeshInst->getVertexBuffer().get());
			m_pCopyMatInst->bindAll(m_pCmdInit);
			m_pCmdInit->setTopology(Topology::triangle_strip);
			m_pCmdInit->drawVertex(4, 0);

			m_pCmdInit->end();

			a_pCanvas->present();
		}
	}
}

void DeferredRenderer::canvasResize(glm::ivec2 a_Size)
{
	for( unsigned int i=0 ; i<GBUFFER_COUNT ; ++i )
	{
		m_pGBuffer[i]->getComponent<TextureAsset>()->resizeRenderTarget(a_Size);
	}
	
	m_pFrameBuffer->getComponent<TextureAsset>()->resizeRenderTarget(a_Size);

	m_TileDim.x = std::ceil(a_Size.x / EngineSetting::singleton().m_TileSize);
	m_TileDim.y = std::ceil(a_Size.y / EngineSetting::singleton().m_TileSize);

	m_pDepthMinmax->getComponent<TextureAsset>()->resizeRenderTarget(a_Size);
	m_TiledValidLightIdx = m_pLightIndexMatInst->createExternalBlock(ShaderRegType::UavBuffer, "g_DstLights", m_TileDim.x * m_TileDim.y * (INIT_LIGHT_SIZE / 2 + 1)); // {index, type}
	
	m_pLightIndexMatInst->setParam<glm::vec2>("c_PixelSize", 0, glm::vec2(1.0f / a_Size.x, 1.0f / a_Size.y));
	m_pLightIndexMatInst->setParam<glm::ivec2>("c_TileCount", 0, m_TileDim);
}

void DeferredRenderer::drawFlagChanged(unsigned int a_Flag)
{
	unsigned int l_OldFlag = getDrawFlag();
	if( IS_FLAG_OPENED(l_OldFlag, a_Flag, DRAW_DEBUG_TEXTURE) )
	{
		const std::pair<wxString, glm::vec4> c_DebugMaterials[] = {
			std::make_pair(DEFFERRED_GBUFFER_DEBUG_NORMAL_ASSET_NAME,		glm::vec4(0.1f, 0.1f, 0.9f, -0.9f)),
			std::make_pair(DEFFERRED_GBUFFER_DEBUG_MATERIAL_ASSET_NAME,		glm::vec4(0.1f, 0.1f, 0.7f, -0.9f)),
			std::make_pair(DEFFERRED_GBUFFER_DEBUG_BASE_COLOR_ASSET_NAME,	glm::vec4(0.1f, 0.1f, 0.5f, -0.9f)),
			std::make_pair(DEFFERRED_GBUFFER_DEBUG_MASK_ASSET_NAME,			glm::vec4(0.1f, 0.1f, 0.9f, -0.7f)),
			std::make_pair(DEFFERRED_GBUFFER_DEBUG_FACTOR_ASSET_NAME,		glm::vec4(0.1f, 0.1f, 0.7f, -0.7f)),
			std::make_pair(DEFFERRED_GBUFFER_DEBUG_MOTION_BLUR_ASSET_NAME,	glm::vec4(0.1f, 0.1f, 0.5f, -0.7f)),
			std::make_pair(DEFFERRED_GBUFFER_DEBUG_DEPTH_ASSET_NAME,		glm::vec4(0.1f, 0.1f, 0.9f, -0.5f))};

		auto l_pRootNode = getScene()->getRootNode();
		for( unsigned int i=0 ; i<GBUFFER_COUNT ; ++i )
		{
			m_DebugTextures[i].init(l_pRootNode, wxString::Format(c_DebugMaterials[i].first, m_Serial), m_pGBuffer[i], c_DebugMaterials[i].second);
		}
	}
	else if( IS_FLAG_CLOSED(l_OldFlag, a_Flag, DRAW_DEBUG_TEXTURE) )
	{
		for( unsigned int i=0 ; i<GBUFFER_COUNT ; ++i ) m_DebugTextures[i].uninit();
	}
}

bool DeferredRenderer::setupVisibleList(std::shared_ptr<Camera> a_pCamera)
{
	getScene()->getSceneGraph(GRAPH_MESH)->getVisibleList(a_pCamera, m_VisibleMeshes);
	if( m_VisibleMeshes.empty() ) return false;

	getScene()->getSceneGraph(GRAPH_LIGHT)->getVisibleList(a_pCamera, m_VisibleLights);

	if( (int)m_VisibleLights.size() >= m_LightIdx->getNumSlot() / 2 )
	{
		int l_Unit = m_ExtendSize / 2;
		int l_Extend = (int)m_VisibleLights.size() / 2 - m_LightIdx->getNumSlot();
		l_Extend = (l_Extend + l_Unit - 1) / l_Unit * l_Unit;

		m_LightIdx->extend(l_Extend);
		m_TiledValidLightIdx->extend((m_LightIdx->getNumSlot() + 1) * m_TileDim.x * m_TileDim.y - m_TiledValidLightIdx->getNumSlot());
	}

	return true;
}

void DeferredRenderer::setupIndexUav()
{
	struct LightInfo
	{
		unsigned int m_Index;
		unsigned int m_Type;
	};

	char *l_pBuff = m_LightIdx->getBlockPtr(0);
	for( unsigned int i=0 ; i<m_VisibleLights.size() ; ++i )
	{
		LightInfo *l_pTarget = reinterpret_cast<LightInfo*>(l_pBuff + sizeof(LightInfo) * i);
		Light *l_pLight = reinterpret_cast<Light*>(m_VisibleLights[i].get());
		l_pTarget->m_Index = l_pLight->getID();
		l_pTarget->m_Type = l_pLight->typeID();
	}
	m_LightIdx->sync(true, false);
}
#pragma endregion

}