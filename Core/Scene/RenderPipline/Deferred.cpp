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
#include "Physical/IntersectHelper.h"
#include "RenderObject/Mesh.h"
#include "RenderObject/Light.h"
#include "RenderObject/TextureAtlas.h"
#include "Scene/Camera.h"
#include "Scene/Scene.h"
#include "Scene/RenderPipline/ShadowMap.h"

#include "Deferred.h"

namespace R
{

#pragma region DeferredRenderer
#pragma region DebugTexture
//
// DebugTexture
//
DeferredRenderer::DebugTexture::DebugTexture()
	: m_pComponent(nullptr)
	, m_pMat(nullptr)
{
}

DeferredRenderer::DebugTexture::~DebugTexture()
{
	if( nullptr != m_pMat )
	{
		AssetManager::singleton().removeAsset(m_pMat);
		m_pMat = nullptr;
	}
}

void DeferredRenderer::DebugTexture::init(std::shared_ptr<SceneNode> a_pNode, std::shared_ptr<Asset> a_pTexture, glm::vec4 a_DockParam)
{
	assert(nullptr == m_pComponent);

	m_pComponent = a_pNode->addComponent<RenderableMesh>();
	m_pComponent->setMesh(AssetManager::singleton().getAsset(QUAD_MESH_ASSET_NAME), 0);

	m_pMat = AssetManager::singleton().createRuntimeAsset<MaterialAsset>();
	MaterialAsset *l_pMatInst = m_pMat->getComponent<MaterialAsset>();
	l_pMatInst->init(ProgramManager::singleton().getData(DefaultPrograms::CopyFrame));
	l_pMatInst->setTexture("m_SrcTex", a_pTexture);
	l_pMatInst->setParam("c_DockParam", "", 0, a_DockParam);

	m_pComponent->setMaterial(MATSLOT_TRANSPARENT, m_pMat);
}

void DeferredRenderer::DebugTexture::uninit()
{
	assert(nullptr != m_pComponent);

	m_pComponent->remove();
	AssetManager::singleton().removeAsset(m_pMat);
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
	, m_pCopyDepthMat(AssetManager::singleton().createAsset(COPYDEPTH_ASSET_NAME))
	, m_pLightIndexMatInst(nullptr), m_pDeferredLightMatInst(nullptr), m_pCopyMatInst(nullptr), m_pCopyDepthMatInst(nullptr)
	, m_pQueryBuffer()
	, m_pPredMat(AssetManager::singleton().createRuntimeAsset<MaterialAsset>())
	, m_pPredMatInst(nullptr)
	, m_QueryID(-1), m_QuerySize(1024)
{
	m_pLightIndexMat = AssetManager::singleton().createRuntimeAsset<MaterialAsset>();
	m_pDeferredLightMat = AssetManager::singleton().createRuntimeAsset<MaterialAsset>();

	m_pQuadMeshInst = m_pQuadMesh->getComponent<MeshAsset>();
	m_pLightIndexMatInst = m_pLightIndexMat->getComponent<MaterialAsset>();
	m_pDeferredLightMatInst = m_pDeferredLightMat->getComponent<MaterialAsset>();
	m_pCopyMatInst = m_pCopyMat->getComponent<MaterialAsset>();
	m_pCopyDepthMatInst = m_pCopyDepthMat->getComponent<MaterialAsset>();
	m_pPredMatInst = m_pPredMat->getComponent<MaterialAsset>();

	m_pLightIndexMatInst->init(ProgramManager::singleton().getData(DefaultPrograms::TiledLightIntersection));
	m_pDeferredLightMatInst->init(ProgramManager::singleton().getData(DefaultPrograms::TiledDeferredLighting));
	m_pCopyMatInst->init(ProgramManager::singleton().getData(DefaultPrograms::Copy));
	m_pCopyDepthMatInst->init(ProgramManager::singleton().getData(DefaultPrograms::CopyDepth));
	m_pPredMatInst->init(ProgramManager::singleton().getData(DefaultPrograms::Predication));

	m_QueryID = GDEVICE()->requestQueryBuffer(m_QuerySize);
	m_pQueryBuffer[0] = AssetManager::singleton().createRuntimeAsset<TextureAsset>();
	m_pQueryBuffer[0]->getComponent<TextureAsset>()->initRenderTarget(EngineSetting::singleton().m_DefaultSize / 4, PixelFormat::d32_float);
	m_pQueryBuffer[1] = AssetManager::singleton().createRuntimeAsset<TextureAsset>();
	m_pQueryBuffer[1]->getComponent<TextureAsset>()->initRenderTarget(EngineSetting::singleton().m_DefaultSize / 4, PixelFormat::rgba8_unorm);
	
	const PixelFormat::Key c_GBufferDef[] = {
		PixelFormat::rgba8_snorm,
		PixelFormat::rgba8_unorm,
		PixelFormat::rgba8_unorm,
		PixelFormat::rgba8_unorm,
		PixelFormat::rgba8_unorm,
		PixelFormat::rg16_float,
		PixelFormat::d24_unorm_s8_uint};
	for( unsigned int i=0 ; i<GBUFFER_COUNT ; ++i )
	{
		m_pGBuffer[i] = AssetManager::singleton().createRuntimeAsset<TextureAsset>();
		m_pGBuffer[i]->getComponent<TextureAsset>()->initRenderTarget(EngineSetting::singleton().m_DefaultSize, c_GBufferDef[i]);

		char l_Buff[8];
		snprintf(l_Buff, 8, "GBuff%d", i);
		m_pDeferredLightMatInst->setTexture(l_Buff, m_pGBuffer[i]);
	}
	m_pFrameBuffer = AssetManager::singleton().createRuntimeAsset<TextureAsset>();
	m_pFrameBuffer->getComponent<TextureAsset>()->initRenderTarget(EngineSetting::singleton().m_DefaultSize, PixelFormat::rgba16_float);

	m_LightIdx = m_pLightIndexMatInst->createExternalBlock(ShaderRegType::UavBuffer, "g_SrcLights", m_ExtendSize / 2); // {index, type}

	m_pCmdInit = GDEVICE()->commanderFactory();

	m_TileDim.x = std::ceil(EngineSetting::singleton().m_DefaultSize.x / EngineSetting::singleton().m_TileSize);
	m_TileDim.y = std::ceil(EngineSetting::singleton().m_DefaultSize.y / EngineSetting::singleton().m_TileSize);
	
	m_pDepthMinmax = AssetManager::singleton().createRuntimeAsset<TextureAsset>();
	m_pDepthMinmax->getComponent<TextureAsset>()->initRenderTarget(EngineSetting::singleton().m_DefaultSize, PixelFormat::rg32_float);
	m_pDepthMinmax->getComponent<TextureAsset>()->setSamplerFilter(Filter::min_mag_mip_point);
	m_MinmaxStepCount = std::ceill(log2f(EngineSetting::singleton().m_TileSize));
	m_TiledValidLightIdx = m_pLightIndexMatInst->createExternalBlock(ShaderRegType::UavBuffer, "g_DstLights", m_TileDim.x * m_TileDim.y * (INIT_LIGHT_SIZE / 2 + 1)); // {index, type}

	m_pLightIndexMatInst->setParam<glm::vec2>("c_PixelSize", "", 0, glm::vec2(1.0f / EngineSetting::singleton().m_DefaultSize.x, 1.0f / EngineSetting::singleton().m_DefaultSize.y));
	m_pLightIndexMatInst->setParam<int>("c_MipmapLevel", "", 0, (int)m_MinmaxStepCount);
	m_pLightIndexMatInst->setParam<glm::ivec2>("c_TileCount", "", 0, m_TileDim);
	m_pLightIndexMatInst->setBlock("g_SrcLights", m_LightIdx);
	m_pLightIndexMatInst->setBlock("g_DstLights", m_TiledValidLightIdx);
	m_pLightIndexMatInst->setBlock("g_OmniLights", a_pScene->getOmniLightContainer()->getMaterialBlock());
	m_pLightIndexMatInst->setBlock("g_SpotLights", a_pScene->getSpotLightContainer()->getMaterialBlock());
	m_pLightIndexMatInst->setTexture("g_MinMapTexture", m_pDepthMinmax);

	m_pDeferredLightMatInst->setParam<glm::ivec2>("c_TileCount", "", 0, m_TileDim);
	m_pDeferredLightMatInst->setParam<int>("c_BoxLevel", "", 0, a_pScene->getLightmap()->getComponent<LightmapAsset>()->getMaxBoxLevel());
	m_pDeferredLightMatInst->setBlock("Boxes", a_pScene->getLightmap()->getComponent<LightmapAsset>()->getBoxes());
	m_pDeferredLightMatInst->setBlock("Harmonics", a_pScene->getLightmap()->getComponent<LightmapAsset>()->getHarmonics());
	m_pDeferredLightMatInst->setBlock("TileLights", m_TiledValidLightIdx);
	m_pDeferredLightMatInst->setBlock("DirLights", a_pScene->getDirLightContainer()->getMaterialBlock());
    m_pDeferredLightMatInst->setBlock("OmniLights", a_pScene->getOmniLightContainer()->getMaterialBlock());
    m_pDeferredLightMatInst->setBlock("SpotLights", a_pScene->getSpotLightContainer()->getMaterialBlock());
	
	ShadowMapRenderer *l_pShadowMap = reinterpret_cast<ShadowMapRenderer *>(getScene()->getShadowMapBaker());
	glm::vec2 l_ShadowMapSize;
	l_ShadowMapSize.x = l_ShadowMapSize.y = l_pShadowMap->getShadowMap()->getComponent<TextureAsset>()->getDimension().x;
	m_pDeferredLightMatInst->setParam<glm::vec2>("c_ShadowMapSize", "", 0, l_ShadowMapSize);

	m_pCopyMatInst->setTexture("m_SrcTex", m_pFrameBuffer);
	m_pCopyDepthMatInst->setTexture("m_SrcTex", m_pGBuffer[GBUFFER_DEPTH]);

	m_DrawCommand.resize(EngineSetting::singleton().m_NumRenderCommandList);
	for( unsigned int i=0 ; i<m_DrawCommand.size() ; ++i ) m_DrawCommand[i] = GDEVICE()->commanderFactory();
}

DeferredRenderer::~DeferredRenderer()
{
	GDEVICE()->freeQueryBuffer(m_QueryID);
	m_QueryID = -1;
	AssetManager::singleton().removeAsset(m_pPredMat);
	m_pPredMat = nullptr;
	for( unsigned int i=0 ; i<2 ; ++i )
	{
		AssetManager::singleton().removeAsset(m_pQueryBuffer[i]);
		m_pQueryBuffer[i] = nullptr;
	}

	m_pQuadMeshInst = nullptr;
	m_pQuadMesh = nullptr;

	AssetManager::singleton().removeAsset(m_pDepthMinmax);
	m_pDepthMinmax = nullptr;
	AssetManager::singleton().removeAsset(m_pFrameBuffer);
	m_pFrameBuffer = nullptr;
	for( unsigned int i=0 ; i<GBUFFER_COUNT ; ++i )
	{
		AssetManager::singleton().removeAsset(m_pGBuffer[i]);
		m_pGBuffer[i] = nullptr;
	}

	SAFE_DELETE(m_pCmdInit);
	for( unsigned int i=0 ; i<m_DrawCommand.size() ; ++i ) SAFE_DELETE(m_DrawCommand[i])
	m_DrawCommand.clear();
	// add memset ?
	
	AssetManager::singleton().removeAsset(m_pLightIndexMat);
	m_pLightIndexMat = nullptr;
	m_pLightIndexMatInst = nullptr;

	AssetManager::singleton().removeAsset(m_pDeferredLightMat);
	m_pDeferredLightMat = nullptr;
	m_pDeferredLightMatInst = nullptr;
	
	AssetManager::singleton().removeAsset(m_pCopyMat);
	m_pCopyMatInst = nullptr;
	m_pCopyMat = nullptr;

	AssetManager::singleton().removeAsset(m_pCopyDepthMat);
	m_pCopyDepthMat = nullptr;
	m_pCopyDepthMatInst = nullptr;

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
	m_DynamicLights.clear();
	{
		auto &l_Lights = a_pCamera->getHelper()->getLights();
		for( auto it=l_Lights.begin() ; it!=l_Lights.end() ; ++it )
		{
			if( (*it)->isStatic() ) continue;
			m_DynamicLights.push_back(*it);
		}
	}

	CONSOLE_LOG(wxT("draw count info : mesh(%d), light(%d)"), int(a_pCamera->getHelper()->getMeshes().size()), int(m_DynamicLights.size()));
	{
		glm::vec3 l_Eye, l_Dir, l_Up;
		a_pCamera->getCameraParam(l_Eye, l_Dir, l_Up);
		CONSOLE_LOG(wxT("Camera position : (%f, %f, %f)"), l_Eye.x, l_Eye.y, l_Eye.z);
	}

	{// setup light map
		std::shared_ptr<Asset> l_pLightmap = getScene()->getLightmap();
		LightmapAsset *l_pLightmapInst = l_pLightmap->getComponent<LightmapAsset>();
		if( l_pLightmapInst->isBaking() )
		{
			l_pLightmapInst->stepBake(m_pCmdInit);
			if( !l_pLightmapInst->isBaking() )
			{
				m_pDeferredLightMatInst->setParam<int>("c_BoxLevel", "", 0, getScene()->getLightmap()->getComponent<LightmapAsset>()->getMaxBoxLevel());
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
	
	EngineCore::singleton().addJob([=](){ this->setupIndexUav(); });
	
	// sort meshes
	auto &l_Meshes = a_pCamera->getHelper()->getMeshes();
	{
		for( unsigned int i=0 ; i<cm_NumMatSlot ; ++i )
		{
			// to do : fix unknown freeze on multi-thread
			//EngineCore::singleton().addJob([=]() -> void
			{
				m_SortedMesh[i].clear();
				m_SortedMesh[i].reserve(l_Meshes.size());
				for( auto it=l_Meshes.begin() ; it!=l_Meshes.end() ; ++it )
				{
					RenderableMesh *l_pMesh = (*it).get();
					if( l_pMesh->getSortKey(i).m_Members.m_bValid ) m_SortedMesh[i].push_back(l_pMesh);
				}
				if( !m_SortedMesh[i].empty() )
				{
					std::sort(m_SortedMesh[i].begin(), m_SortedMesh[i].end(), [=](RenderableMesh *a_pLeft, RenderableMesh *a_pRight) -> bool
					{
						return a_pLeft->getSortKey(i).m_Key < a_pRight->getSortKey(i).m_Key;
					});
				}
			}//);
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
	
	{// graphic step
		m_pCmdInit->begin(false);

		for( unsigned int i=0 ; i<GBUFFER_COUNT - 1 ; ++i )
		{
			m_pCmdInit->clearRenderTarget(m_pGBuffer[i]->getComponent<TextureAsset>()->getTextureID(), glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
		}
		m_pCmdInit->clearDepthTarget(m_pGBuffer[GBUFFER_DEPTH]->getComponent<TextureAsset>()->getTextureID(), true, 1.0f, true, 0);
		//m_pCmdInit->clearRenderTarget(m_pQueryBuffer[1]->getComponent<TextureAsset>()->getTextureID(), glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
		//m_pCmdInit->clearDepthTarget(m_pQueryBuffer[0]->getComponent<TextureAsset>()->getTextureID(), true, 1.0f, false, 0);

		m_pCmdInit->end();

		// draw gbuffer
		unsigned int l_NumCommand = m_DrawCommand.size();
		for( unsigned int i=0 ; i<m_DrawCommand.size() ; ++i )
		{
			EngineCore::singleton().addJob([=]() -> void
			{
				m_DrawCommand[i]->begin(false);
				
				std::pair<unsigned int, unsigned int> l_Range = calculateSegment(m_SortedMesh[MATSLOT_OPAQUE].size(), l_NumCommand, i);

				/*glm::viewport l_Viewport(0.0f, 0.0f, l_ViewPortSize.x / 4.0f, l_ViewPortSize.y / 4.0f, 0.0f, 1.0f);
				m_DrawCommand[i]->setViewPort(1, l_Viewport);
				m_DrawCommand[i]->setScissor(1, glm::ivec4(0, 0, l_ViewPortSize.x, l_ViewPortSize.y));
				m_DrawCommand[i]->setRenderTarget(m_pQueryBuffer[0]->getComponent<TextureAsset>()->getTextureID(), 1,
												m_pQueryBuffer[1]->getComponent<TextureAsset>()->getTextureID());

				m_DrawCommand[i]->beginQuery(m_QueryID, l_Range.first);
				getScene()->getRenderBatcher()->drawSortedMeshes(m_DrawCommand[i], m_SortedMesh[MATSLOT_OPAQUE]
					, l_Range.first, l_Range.second
					, [=](RenderableMesh *a_pMesh) -> MaterialAsset*
					{
						return m_pPredMatInst;
					}
					, [=](MaterialAsset *a_pMat) -> void
					{
						a_pMat->bindBlock(m_DrawCommand[i], "Camera", a_pCamera->getMaterialBlock());
					}
					, [=](std::vector<glm::ivec4> &a_Instance, unsigned int a_Idx) -> unsigned int
					{
						a_Instance.push_back(glm::ivec4(m_SortedMesh[MATSLOT_OPAQUE][a_Idx]->getWorldOffset(), 0, 0, 0));
						return 1;
					});
				m_DrawCommand[i]->endQuery(m_QueryID, l_Range.first);
				m_DrawCommand[i]->resolveQuery(m_QueryID, l_Range.second - l_Range.first, l_Range.first);*/

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

				//m_DrawCommand[i]->bindPredication(m_QueryID, l_Range.first);
				getScene()->getRenderBatcher()->drawSortedMeshes(m_DrawCommand[i], m_SortedMesh[MATSLOT_OPAQUE]
					, l_Range.first, l_Range.second
					, [=](RenderableMesh *a_pMesh) -> MaterialAsset*
					{
						return a_pMesh->getMaterial(MATSLOT_OPAQUE).second->getComponent<MaterialAsset>();
					}
					, [=](MaterialAsset *a_pMat) -> void
					{
						a_pMat->bindBlock(m_DrawCommand[i], "Camera", a_pCamera->getMaterialBlock());
					}
					, [=](std::vector<glm::ivec4> &a_Instance, unsigned int a_Idx) -> unsigned int
					{
						glm::ivec4 l_Instance(0, 0, 0, 0);
						l_Instance.x = m_SortedMesh[MATSLOT_OPAQUE][a_Idx]->getWorldOffset();
						l_Instance.y = m_SortedMesh[MATSLOT_OPAQUE][a_Idx]->getMaterial(MATSLOT_OPAQUE).first;
						a_Instance.push_back(l_Instance);
						return 1;
					});
				//m_DrawCommand[i]->bindPredication(-1);

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
			m_pCopyDepthMatInst->bindAll(m_pCmdInit);
			m_pCmdInit->setTopology(Topology::triangle_strip);
			m_pCmdInit->drawVertex(4, 0);

			m_pCmdInit->end();
		}

		m_pDepthMinmax->getComponent<TextureAsset>()->generateMipmap(0, ProgramManager::singleton().getData(DefaultPrograms::GenerateMinmaxDepth), false);
		
		m_pCmdInit->begin(true);
		
		m_pCmdInit->useProgram(DefaultPrograms::TiledLightIntersection);
		m_pLightIndexMatInst->setBlock("Camera", a_pCamera->getMaterialBlock());
		m_pLightIndexMatInst->setParam<int>("c_NumLight", "", 0, (int)m_DynamicLights.size());
		m_pLightIndexMatInst->bindAll(m_pCmdInit);
		m_pCmdInit->compute(std::max((m_TileDim.x + 7) / 8, 1), std::max((m_TileDim.y + 7) / 8, 1));

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
		m_pDeferredLightMatInst->setParam<int>("c_NumLight", "", 0, (int)m_DynamicLights.size());
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
				
				std::pair<unsigned int, unsigned int> l_Range = calculateSegment(m_SortedMesh[MATSLOT_TRANSPARENT].size(), l_NumCommand, i);

				getScene()->getRenderBatcher()->drawSortedMeshes(m_DrawCommand[i], m_SortedMesh[MATSLOT_TRANSPARENT]
					, l_Range.first, l_Range.second
					, [=](RenderableMesh *a_pMesh) -> MaterialAsset*
					{
						return a_pMesh->getMaterial(MATSLOT_TRANSPARENT).second->getComponent<MaterialAsset>();
					}
					, [=](MaterialAsset *a_pMat) -> void
					{
						a_pMat->bindBlock(m_DrawCommand[i], "Camera", a_pCamera->getMaterialBlock());
					}
					, [=](std::vector<glm::ivec4> &a_Instance, unsigned int a_Idx) -> unsigned int
					{
						glm::ivec4 l_Instance(0, 0, 0, 0);
						l_Instance.x = m_SortedMesh[MATSLOT_TRANSPARENT][a_Idx]->getWorldOffset();
						l_Instance.y = m_SortedMesh[MATSLOT_TRANSPARENT][a_Idx]->getMaterial(MATSLOT_TRANSPARENT).first;
						a_Instance.push_back(l_Instance);
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
	for( unsigned int i=0 ; i<2 ; ++i )
	{
		m_pQueryBuffer[i]->getComponent<TextureAsset>()->resizeRenderTarget(a_Size / 4);
	}
	for( unsigned int i=0 ; i<GBUFFER_COUNT ; ++i )
	{
		m_pGBuffer[i]->getComponent<TextureAsset>()->resizeRenderTarget(a_Size);
	}
	
	m_pFrameBuffer->getComponent<TextureAsset>()->resizeRenderTarget(a_Size);

	m_TileDim.x = std::ceil(a_Size.x / EngineSetting::singleton().m_TileSize);
	m_TileDim.y = std::ceil(a_Size.y / EngineSetting::singleton().m_TileSize);

	m_pDepthMinmax->getComponent<TextureAsset>()->resizeRenderTarget(a_Size);
	
	m_pLightIndexMatInst->setParam<glm::vec2>("c_PixelSize", "", 0, glm::vec2(1.0f / a_Size.x, 1.0f / a_Size.y));
	m_pLightIndexMatInst->setParam<glm::ivec2>("c_TileCount", "", 0, m_TileDim);
	m_pDeferredLightMatInst->setParam<glm::ivec2>("c_TileCount", "", 0, m_TileDim);
}

void DeferredRenderer::drawFlagChanged(unsigned int a_Flag)
{
	unsigned int l_OldFlag = getDrawFlag();
	if( IS_FLAG_OPENED(l_OldFlag, a_Flag, DRAW_DEBUG_TEXTURE) )
	{
		const glm::vec4 c_DebugMaterials[] = {
			glm::vec4(0.1f, 0.1f, 0.9f, -0.9f),
			glm::vec4(0.1f, 0.1f, 0.7f, -0.9f),
			glm::vec4(0.1f, 0.1f, 0.5f, -0.9f),
			glm::vec4(0.1f, 0.1f, 0.9f, -0.7f),
			glm::vec4(0.1f, 0.1f, 0.7f, -0.7f),
			glm::vec4(0.1f, 0.1f, 0.5f, -0.7f),
			glm::vec4(0.1f, 0.1f, 0.9f, -0.5f)};

		auto l_pRootNode = getScene()->getRootNode();
		for( unsigned int i=0 ; i<GBUFFER_COUNT ; ++i ) m_DebugTextures[i].init(l_pRootNode, m_pGBuffer[i], c_DebugMaterials[i]);
	}
	else if( IS_FLAG_CLOSED(l_OldFlag, a_Flag, DRAW_DEBUG_TEXTURE) )
	{
		for( unsigned int i=0 ; i<GBUFFER_COUNT ; ++i ) m_DebugTextures[i].uninit();
	}
}

bool DeferredRenderer::setupVisibleList(std::shared_ptr<Camera> a_pCamera)
{
	auto &l_Meshes = a_pCamera->getHelper()->getMeshes();
	if( l_Meshes.empty() ) return false;

	if( l_Meshes.size() >= m_QuerySize )
	{
		GDEVICE()->freeQueryBuffer(m_QueryID);
		if( 0 == l_Meshes.size() % m_ExtendSize ) m_QuerySize = (int)l_Meshes.size();
		else m_QuerySize = ((int)l_Meshes.size() + m_ExtendSize) / m_ExtendSize * m_ExtendSize;
		m_QueryID = GDEVICE()->requestQueryBuffer(m_QuerySize);
	}

	if( (int)m_DynamicLights.size() >= m_LightIdx->getNumSlot() / 2 )
	{
		int l_Unit = m_ExtendSize / 2;
		int l_Extend = (int)m_DynamicLights.size() / 2 - m_LightIdx->getNumSlot();
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
	for( unsigned int i=0 ; i<m_DynamicLights.size() ; ++i )
	{
		LightInfo *l_pTarget = reinterpret_cast<LightInfo*>(l_pBuff + sizeof(LightInfo) * i);
		Light *l_pLight = m_DynamicLights[i].get();
		l_pTarget->m_Index = l_pLight->getID();
		l_pTarget->m_Type = l_pLight->typeID();
	}
	m_LightIdx->sync(true, false);
}
#pragma endregion

}