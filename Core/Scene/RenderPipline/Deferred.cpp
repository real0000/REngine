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

#define FRAMEBUFFER_ASSET_NAME wxT("DefferredFrame.Image")
#define DEPTHMINMAX_ASSET_NAME wxT("DefferredDepthMinmax.Image")

#define LIGHTINDEX_ASSET_NAME wxT("DefferredLightIndex.Material")
#define LIGHTING_ASSET_NAME wxT("TiledDefferredLighting.Material")
#define COPY_ASSET_NAME wxT("Copy.Material")

const std::pair<wxString, PixelFormat::Key> c_GBufferDef[] = {
	{wxT("DefferredGBufferNormal.Image"), PixelFormat::rgba8_snorm},
	{wxT("DefferredGBufferMaterial.Image"), PixelFormat::rgba8_uint},
	{wxT("DefferredGBufferDiffuse.Image"), PixelFormat::rgba8_unorm},
	{wxT("DefferredGBufferMask.Image"), PixelFormat::rgba8_unorm},
	{wxT("DefferredGBufferFactor.Image"), PixelFormat::rgba8_unorm},
	{wxT("DefferredGBufferMotionBlur.Image"), PixelFormat::rg16_float},
	{wxT("DefferredGBufferDepth.Image"), PixelFormat::d24_unorm_s8_uint}};

#pragma region DeferredRenderer
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
	, m_pFrameBuffer(nullptr)
	, m_pDepthMinmax(nullptr)
	, m_MinmaxStepCount(1)
	, m_pLightIndexMat(AssetManager::singleton().createAsset(LIGHTINDEX_ASSET_NAME))
	, m_pDeferredLightMat(AssetManager::singleton().createAsset(LIGHTING_ASSET_NAME))
	, m_pCopyMat(AssetManager::singleton().createAsset(COPY_ASSET_NAME))
	, m_pLightIndexMatInst(nullptr), m_pDeferredLightMatInst(nullptr), m_pCopyMatInst(nullptr)
{
	m_pLightIndexMatInst = m_pLightIndexMat->getComponent<MaterialAsset>();
	m_pDeferredLightMatInst = m_pDeferredLightMat->getComponent<MaterialAsset>();
	m_pCopyMatInst = m_pCopyMat->getComponent<MaterialAsset>();

	m_pLightIndexMatInst->init(ProgramManager::singleton().getData(DefaultPrograms::TiledLightIntersection));
	m_pDeferredLightMatInst->init(ProgramManager::singleton().getData(DefaultPrograms::TiledDeferredLighting));
	m_pCopyMatInst->init(ProgramManager::singleton().getData(DefaultPrograms::Copy));

	for( unsigned int i=0 ; i<GBUFFER_COUNT ; ++i )
	{
		m_pGBuffer[i] = AssetManager::singleton().createAsset(c_GBufferDef[i].first);
		m_pGBuffer[i]->getComponent<TextureAsset>()->initRenderTarget(EngineSetting::singleton().m_DefaultSize, c_GBufferDef[i].second);

		char l_Buff[8];
		snprintf(l_Buff, 8, "GBuff%d", i);
		m_pDeferredLightMatInst->setTexture(l_Buff, m_pGBuffer[i]);
	}
	m_pFrameBuffer = AssetManager::singleton().createAsset(FRAMEBUFFER_ASSET_NAME);
	m_pFrameBuffer->getComponent<TextureAsset>()->initRenderTarget(EngineSetting::singleton().m_DefaultSize, PixelFormat::rgba16_float);

	m_LightIdx = m_pLightIndexMatInst->createExternalBlock(ShaderRegType::UavBuffer, "g_SrcLights", m_ExtendSize / 2); // {index, type}

	m_pCmdInit = GDEVICE()->commanderFactory();

	m_TileDim.x = std::ceil(EngineSetting::singleton().m_DefaultSize.x / EngineSetting::singleton().m_TileSize);
	m_TileDim.y = std::ceil(EngineSetting::singleton().m_DefaultSize.y / EngineSetting::singleton().m_TileSize);
	
	m_pDepthMinmax = AssetManager::singleton().createAsset(DEPTHMINMAX_ASSET_NAME);
	m_pDepthMinmax->getComponent<TextureAsset>()->initRenderTarget(EngineSetting::singleton().m_DefaultSize, PixelFormat::rg16_float);
	m_MinmaxStepCount = std::ceill(log2f(EngineSetting::singleton().m_TileSize));
	m_TiledValidLightIdx = m_pLightIndexMatInst->createExternalBlock(ShaderRegType::UavBuffer, "g_DstLights", m_TileDim.x * m_TileDim.y * INIT_LIGHT_SIZE / 2); // {index, type}

	m_pLightIndexMatInst->setParam<glm::vec2>("c_PixelSize", 0, glm::vec2(1.0f / EngineSetting::singleton().m_DefaultSize.x, 1.0f / EngineSetting::singleton().m_DefaultSize.y));
	m_pLightIndexMatInst->setParam<int>("c_MipmapLevel", 0, (int)m_MinmaxStepCount);
	m_pLightIndexMatInst->setParam<glm::ivec2>("c_TileCount", 0, m_TileDim);
	
	m_pLightIndexMatInst->setBlock("g_SrcLights", m_LightIdx);
	m_pLightIndexMatInst->setBlock("g_DstLights", m_TiledValidLightIdx);
	m_pLightIndexMatInst->setBlock("g_OmniLights", a_pScene->getOmniLightContainer()->getMaterialBlock());
	m_pLightIndexMatInst->setBlock("g_SpotLights", a_pScene->getSpotLightContainer()->getMaterialBlock());

	m_pCopyMatInst->setTexture("m_SrcTex", m_pFrameBuffer);

	m_DrawCommand.resize(EngineSetting::singleton().m_NumRenderCommandList);
	for( unsigned int i=0 ; i<m_DrawCommand.size() ; ++i ) m_DrawCommand[i] = GDEVICE()->commanderFactory();

	m_GBufferCache.resize(GBUFFER_COUNT - 1);
	for( unsigned int i=GBUFFER_NORMAL ; i<=GBUFFER_MOTIONBLUR ; ++i ) m_GBufferCache[i] = m_pGBuffer[i]->getComponent<TextureAsset>()->getTextureID();
	m_FBufferCache.resize(1, m_pFrameBuffer->getComponent<TextureAsset>()->getTextureID());
}

DeferredRenderer::~DeferredRenderer()
{
	AssetManager::singleton().removeData(FRAMEBUFFER_ASSET_NAME);
	AssetManager::singleton().removeData(DEPTHMINMAX_ASSET_NAME);
	m_pDepthMinmax = nullptr;
	m_pFrameBuffer = nullptr;
	for( unsigned int i=0 ; i<GBUFFER_COUNT ; ++i )
	{
		AssetManager::singleton().removeData(c_GBufferDef[i].first);
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
	std::vector<std::shared_ptr<RenderableComponent>> l_Lights, l_StaticMeshes, l_Meshes;

	std::shared_ptr<Asset> l_pLightmap = getScene()->getLightmap();
	if( nullptr != l_pLightmap )
	{
		LightmapAsset *l_pLightmapInst = l_pLightmap->getComponent<LightmapAsset>();
		l_pLightmapInst->stepBake(m_pCmdInit);
	}

	if( !setupVisibleList(a_pCamera, l_Lights, l_StaticMeshes, l_Meshes) )
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
	
	EngineCore::singleton().addJob([=, &l_Lights, &l_Meshes](){ this->setupIndexUav(l_Lights);});
	
	// sort meshes
	{
		EngineCore::singleton().addJob([=]() -> void
		{
			for( unsigned int i=0 ; i<l_StaticMeshes.size() ; ++i )
			{
				
			}
		});
	}

	// shadow map render
	ShadowMapRenderer *l_pShadowMap = reinterpret_cast<ShadowMapRenderer *>(getScene()->getShadowMapBaker());
	l_pShadowMap->bake(l_Lights, l_StaticMeshes, l_Meshes, m_pCmdInit, m_DrawCommand);

	{// graphic step, divide by stage
		//bind gbuffer
		m_pCmdInit->begin(false);

		glm::viewport l_Viewport(0.0f, 0.0f, EngineSetting::singleton().m_DefaultSize.x, EngineSetting::singleton().m_DefaultSize.y, 0.0f, 1.0f);
		m_pCmdInit->setViewPort(1, l_Viewport);
		m_pCmdInit->setScissor(1, glm::ivec4(0, 0, EngineSetting::singleton().m_DefaultSize.x, EngineSetting::singleton().m_DefaultSize.y));
		m_pCmdInit->setRenderTarget(m_pGBuffer[GBUFFER_DEPTH]->getComponent<TextureAsset>()->getTextureID(), 6,
									m_pGBuffer[GBUFFER_NORMAL]->getComponent<TextureAsset>()->getTextureID(),
									m_pGBuffer[GBUFFER_MATERIAL]->getComponent<TextureAsset>()->getTextureID(),
									m_pGBuffer[GBUFFER_BASECOLOR]->getComponent<TextureAsset>()->getTextureID(),
									m_pGBuffer[GBUFFER_MASK]->getComponent<TextureAsset>()->getTextureID(),
									m_pGBuffer[GBUFFER_FACTOR]->getComponent<TextureAsset>()->getTextureID(),
									m_pGBuffer[GBUFFER_MOTIONBLUR]->getComponent<TextureAsset>()->getTextureID());

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

				for( unsigned int j=0 ; j<l_Meshes.size() ; j+=l_NumCommand )
				{
					if( j+i >= l_Meshes.size() ) break;


				}

				m_DrawCommand[i]->end();
			});
		}
		EngineCore::singleton().join();
		
		{// copy depth data
			m_pCmdInit->useProgram(DefaultPrograms::CopyDepth);
			m_pCmdInit->setRenderTarget(-1, 1, m_pDepthMinmax->getComponent<TextureAsset>()->getTextureID());

			glm::ivec3 l_DepthSize(m_pDepthMinmax->getComponent<TextureAsset>()->getDimension());
			glm::viewport l_DepthView(0.0f, 0.0f, l_DepthSize.x, l_DepthSize.y, 0.0f, 1.0f);
			m_pCmdInit->setViewPort(1, l_DepthView);
			m_pCmdInit->setScissor(1, glm::ivec4(0, 0, l_DepthSize.x, l_DepthSize.y));

			m_pCmdInit->bindVertex(EngineCore::singleton().getQuadBuffer().get());
			m_pCmdInit->bindTexture(m_pGBuffer[GBUFFER_DEPTH]->getComponent<TextureAsset>()->getTextureID(), 0, GraphicCommander::BIND_RENDER_TARGET);
			m_pCmdInit->bindSampler(m_pGBuffer[GBUFFER_DEPTH]->getComponent<TextureAsset>()->getSamplerID(), 0);
			m_pCmdInit->setTopology(Topology::triangle_strip);
			m_pCmdInit->drawVertex(4, 0);

			m_pCmdInit->end();
		}
		// do fence ?
		m_pDepthMinmax->getComponent<TextureAsset>()->generateMipmap(m_MinmaxStepCount, ProgramManager::singleton().getData(DefaultPrograms::GenerateMinmaxDepth));

		m_pCmdInit->begin(true);
		
		m_pCmdInit->useProgram(DefaultPrograms::TiledLightIntersection);
		m_pLightIndexMatInst->setBlock("g_Camera", a_pCamera->getMaterialBlock());
		m_pLightIndexMatInst->setParam<int>("c_NumLight", 0, (int)l_Lights.size());
		m_pLightIndexMatInst->bindAll(m_pCmdInit);
		m_pCmdInit->compute(m_TileDim.x / 8, m_TileDim.y / 8);

		m_pCmdInit->end();

		// draw frame buffer
		m_pCmdInit->begin(false);
		
		m_pCmdInit->useProgram(m_pDeferredLightMat->getComponent<MaterialAsset>()->getProgram());
		m_pCmdInit->setRenderTarget(m_pGBuffer[GBUFFER_DEPTH]->getComponent<TextureAsset>()->getTextureID(), 1, m_pFrameBuffer->getComponent<TextureAsset>()->getTextureID());
		m_pCmdInit->clearRenderTarget(m_pFrameBuffer->getComponent<TextureAsset>()->getTextureID(), glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));

		glm::ivec3 l_FrameSize(m_pFrameBuffer->getComponent<TextureAsset>()->getDimension());
		glm::viewport l_FrameView(0.0f, 0.0f, l_FrameSize.x, l_FrameSize.y, 0.0f, 1.0f);
		m_pCmdInit->setViewPort(1, l_FrameView);
		m_pCmdInit->setScissor(1, glm::ivec4(0, 0, l_FrameSize.x, l_FrameSize.y));

		m_pCmdInit->bindVertex(EngineCore::singleton().getQuadBuffer().get());
		m_pDeferredLightMatInst->bindAll(m_pCmdInit);
		m_pCmdInit->setTopology(Topology::triangle_strip);
		m_pCmdInit->drawVertex(4, 0);

		m_pCmdInit->end();

		// draw extra object
		//drawOpaqueMesh(a_pCamera, m_pGBuffer[GBUFFER_DEPTH]->getTextureID(), m_FBufferCache, l_Meshes, l_CurrIdx);

		// draw transparent objects
		//if( l_CurrIdx < l_Meshes.size() ) drawMesh(a_pCamera, m_pGBuffer[GBUFFER_DEPTH]->getTextureID(), m_FBufferCache, l_Meshes, l_CurrIdx, l_Meshes.size() - 1);
		
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

			m_pCmdInit->bindVertex(EngineCore::singleton().getQuadBuffer().get());
			m_pCopyMatInst->bindAll(m_pCmdInit);
			m_pCmdInit->setTopology(Topology::triangle_strip);
			m_pCmdInit->drawVertex(4, 0);

			m_pCmdInit->end();

			a_pCanvas->present();
		}
	}
}

bool DeferredRenderer::setupVisibleList(std::shared_ptr<Camera> a_pCamera
		, std::vector<std::shared_ptr<RenderableComponent>> &a_Light
		, std::vector<std::shared_ptr<RenderableComponent>> &a_StaticMesh, std::vector<std::shared_ptr<RenderableComponent>> &a_Mesh)
{
	getScene()->getSceneGraph(GRAPH_MESH)->getVisibleList(a_pCamera, a_Mesh);
	getScene()->getSceneGraph(GRAPH_STATIC_MESH)->getVisibleList(a_pCamera, a_StaticMesh);
	if( a_Mesh.empty() && a_StaticMesh.empty() ) return false;

	getScene()->getSceneGraph(GRAPH_LIGHT)->getVisibleList(a_pCamera, a_Light);

	if( (int)a_Light.size() >= m_LightIdx->getBlockSize() / (sizeof(unsigned int) * 2) )
	{
		m_LightIdx->extend(m_ExtendSize / 2);
	}

	return true;
}

void DeferredRenderer::setupIndexUav(std::vector< std::shared_ptr<RenderableComponent> > &a_Light)
{
	struct LightInfo
	{
		unsigned int m_Index;
		unsigned int m_Type;
	};

	char *l_pBuff = m_LightIdx->getBlockPtr(0);
	for( unsigned int i=0 ; i<a_Light.size() ; ++i )
	{
		LightInfo *l_pTarget = reinterpret_cast<LightInfo *>(l_pBuff + sizeof(LightInfo) * i);
		Light *l_pLight = reinterpret_cast<Light *>(a_Light[i].get());
		l_pTarget->m_Index = l_pLight->getID();
		l_pTarget->m_Type = l_pLight->typeID();
	}
	m_LightIdx->sync(true);
}
#pragma endregion

}