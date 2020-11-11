// Deferred.cpp
//
// 2017/08/14 Ian Wu/Real0000
//

#include "CommonUtil.h"
#include "RGDeviceWrapper.h"

#include "Core.h"
#include "Asset/AssetBase.h"
#include "Asset/MaterialAsset.h"
#include "Asset/MeshAsset.h"
#include "Asset/TextureAsset.h"
#include "RenderObject/Mesh.h"
#include "RenderObject/Light.h"
#include "RenderObject/TextureAtlas.h"
#include "Scene/Camera.h"
#include "Scene/Scene.h"
#include "Scene/Graph/ScenePartition.h"

#include "Deferred.h"

namespace R
{

#define SHADOWMAP_ASSET_NAME wxT("DefferredRenderTextureAtlasDepth.Image")
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
DeferredRenderer::DeferredRenderer(std::shared_ptr<Scene> a_pScene)
	: RenderPipeline(a_pScene)
	, m_pCmdInit(nullptr)
	, m_LightIdx(nullptr)
	, m_ExtendSize(INIT_CONTAINER_SIZE)
	, m_pShadowMap(new RenderTextureAtlas(glm::ivec2(EngineSetting::singleton().m_ShadowMapSize, EngineSetting::singleton().m_ShadowMapSize), PixelFormat::d32_float))
	, m_pDirShadowMat(nullptr), m_pOmniShadowMat(nullptr), m_pSpotShadowMat(nullptr)
	, m_TileDim(0, 0)
	, m_pFrameBuffer(nullptr)
	, m_pDepthMinmax(nullptr)
	, m_MinmaxStepCount(1)
	, m_pLightIndexMat(AssetManager::singleton().createAsset(LIGHTINDEX_ASSET_NAME).second)
	, m_pDeferredLightMat(AssetManager::singleton().createAsset(LIGHTING_ASSET_NAME).second)
	, m_pCopyMat(AssetManager::singleton().createAsset(COPY_ASSET_NAME).second)
	, m_pLightIndexMatInst(nullptr), m_pDeferredLightMatInst(nullptr), m_pCopyMatInst(nullptr)
	, m_ThreadPool(std::thread::hardware_concurrency())
{
	std::tuple<std::shared_ptr<Asset>, std::shared_ptr<MaterialBlock>, wxString> l_ShadowMaps[] = {
		std::make_tuple(m_pDirShadowMat, a_pScene->getDirLightContainer()->getMaterialBlock(), EngineSetting::singleton().m_DirMaterial),
		std::make_tuple(m_pOmniShadowMat, a_pScene->getOmniLightContainer()->getMaterialBlock(), EngineSetting::singleton().m_OmniMaterial),
		std::make_tuple(m_pSpotShadowMat, a_pScene->getSpotLightContainer()->getMaterialBlock(), EngineSetting::singleton().m_SpotMaterial)};
	const unsigned int c_NumShadowMap = sizeof(l_ShadowMaps) / sizeof(std::pair<std::shared_ptr<Asset>&, wxString>);
	for( unsigned int i=0 ; i<c_NumShadowMap ; ++i )
	{
		std::get<0>(l_ShadowMaps[i]) = AssetManager::singleton().getAsset(std::get<2>(l_ShadowMaps[i])).second;

		MaterialAsset *l_pMatInst = std::get<0>(l_ShadowMaps[i])->getComponent<MaterialAsset>();
		l_pMatInst->setBlock("m_SkinTransition", a_pScene->getRenderBatcher()->getSkinMatrixBlock());
		l_pMatInst->setBlock("m_NormalTransition", a_pScene->getRenderBatcher()->getWorldMatrixBlock());
		l_pMatInst->setBlock("m_Lights", std::get<1>(l_ShadowMaps[i]));
	}

	m_pLightIndexMatInst = m_pLightIndexMat->getComponent<MaterialAsset>();
	m_pDeferredLightMatInst = m_pDeferredLightMat->getComponent<MaterialAsset>();
	m_pCopyMatInst = m_pCopyMat->getComponent<MaterialAsset>();

	m_pLightIndexMatInst->init(ProgramManager::singleton().getData(DefaultPrograms::TiledLightIntersection));
	m_pDeferredLightMatInst->init(ProgramManager::singleton().getData(DefaultPrograms::TiledDeferredLighting));
	m_pCopyMatInst->init(ProgramManager::singleton().getData(DefaultPrograms::Copy));

	for( unsigned int i=0 ; i<GBUFFER_COUNT ; ++i )
	{
		m_pGBuffer[i] = AssetManager::singleton().createAsset(c_GBufferDef[i].first).second;
		m_pGBuffer[i]->getComponent<TextureAsset>()->initRenderTarget(EngineSetting::singleton().m_DefaultSize, c_GBufferDef[i].second);

		char l_Buff[8];
		snprintf(l_Buff, 8, "GBuff%d", i);
		m_pDeferredLightMatInst->setTexture(l_Buff, m_pGBuffer[i]);
	}
	m_pFrameBuffer = AssetManager::singleton().createAsset(FRAMEBUFFER_ASSET_NAME).second;
	m_pFrameBuffer->getComponent<TextureAsset>()->initRenderTarget(EngineSetting::singleton().m_DefaultSize, PixelFormat::rgba16_float);

	m_LightIdx = m_pLightIndexMatInst->createExternalBlock(ShaderRegType::UavBuffer, "g_SrcLights", m_ExtendSize / 2); // {index, type}

	m_pCmdInit = GDEVICE()->commanderFactory();

	m_TileDim.x = std::ceil(EngineSetting::singleton().m_DefaultSize.x / EngineSetting::singleton().m_TileSize);
	m_TileDim.y = std::ceil(EngineSetting::singleton().m_DefaultSize.y / EngineSetting::singleton().m_TileSize);
	
	m_pDepthMinmax = AssetManager::singleton().createAsset(DEPTHMINMAX_ASSET_NAME).second;
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
	AssetManager::singleton().removeData(SHADOWMAP_ASSET_NAME);
	AssetManager::singleton().removeData(FRAMEBUFFER_ASSET_NAME);
	AssetManager::singleton().removeData(DEPTHMINMAX_ASSET_NAME);
	m_pDepthMinmax = nullptr;
	m_pFrameBuffer = nullptr;
	for( unsigned int i=0 ; i<GBUFFER_COUNT ; ++i )
	{
		AssetManager::singleton().removeData(c_GBufferDef[i].first);
		m_pGBuffer[i] = nullptr;
	}
	
	m_pDirShadowMat = nullptr;
	m_pOmniShadowMat = nullptr;
	m_pSpotShadowMat = nullptr;
	SAFE_DELETE(m_pShadowMap)

	for( unsigned int i=0 ; i<m_ShadowCommands.size() ; ++i ) delete m_ShadowCommands[i];
	m_ShadowCommands.clear();

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

void DeferredRenderer::render(std::shared_ptr<Camera> a_pCamera, GraphicCanvas *a_pCanvas)
{
	std::vector<std::shared_ptr<RenderableComponent>> l_StaticLights, l_Lights, l_StaticMeshes, l_Meshes;
	if( !setupVisibleList(a_pCamera, l_StaticLights, l_Lights, l_StaticMeshes, l_Meshes) )
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
	
	m_pShadowMap->releaseAll();

	{// shadow step
		unsigned int l_CurrShadowMapSize = m_pShadowMap->getArraySize();
		m_ThreadPool.addJob([=, &l_Lights, &l_Meshes](){ this->setupIndexUav(l_Lights, l_Meshes);});
		
		std::vector<std::shared_ptr<DirLight>> l_DirLights;
		std::vector<std::shared_ptr<OmniLight>> l_OmniLights;
		std::vector<std::shared_ptr<SpotLight>> l_SpotLights;
		for( unsigned int i=0 ; i<l_Lights.size() ; ++i )
		{
			auto l_pLight = l_Lights[i]->shared_from_base<Light>();
			if( l_pLight->getShadowed() )
			{
				m_ThreadPool.addJob([=, &l_pLight]()
				{
					unsigned int l_Size = calculateShadowMapRegion(l_pLight->getShadowCamera(), l_pLight);
					requestShadowMapRegion(l_Size, l_pLight);
				});
				switch( l_pLight->getID() )
				{
					case COMPONENT_DirLight:	l_DirLights.push_back(l_Lights[i]->shared_from_base<DirLight>());	break;
					case COMPONENT_OmniLight:	l_OmniLights.push_back(l_Lights[i]->shared_from_base<OmniLight>());	break;
					case COMPONENT_SpotLight:	l_SpotLights.push_back(l_Lights[i]->shared_from_base<SpotLight>());	break;
					default:break;
				}
			}
		}
		
		m_ThreadPool.join();

		getScene()->getDirLightContainer()->flush();
		getScene()->getOmniLightContainer()->flush();
		getScene()->getSpotLightContainer()->flush();

		// add required command execute thread
		while( m_ShadowCommands.size() <= m_pShadowMap->getArraySize() ) m_ShadowCommands.push_back(GDEVICE()->commanderFactory());

		m_pCmdInit->begin(false);

		glm::viewport l_Viewport(0.0f, 0.0f, EngineSetting::singleton().m_ShadowMapSize, EngineSetting::singleton().m_ShadowMapSize, 0.0f, 1.0f);
		m_pCmdInit->setRenderTarget(m_pShadowMap->getTexture()->getComponent<TextureAsset>()->getTextureID(), 0);
		m_pCmdInit->setViewPort(1, l_Viewport);
		m_pCmdInit->setScissor(1, glm::ivec4(0, 0, EngineSetting::singleton().m_ShadowMapSize, EngineSetting::singleton().m_ShadowMapSize));
		m_pCmdInit->clearDepthTarget(m_pShadowMap->getTexture()->getComponent<TextureAsset>()->getTextureID(), true, 1.0f, false, 0);

		m_pCmdInit->end();

		unsigned int l_NumCommand = std::min(l_Meshes.size(), m_DrawCommand.size());
		for( unsigned int i=0 ; i<l_NumCommand ; ++i )
		{
			m_ThreadPool.addJob([=, &i, &l_NumCommand, &l_DirLights, &l_OmniLights, &l_SpotLights, &l_Meshes]()
			{
				m_DrawCommand[i]->begin(false);

				for( unsigned int j=0 ; j<l_Meshes.size() ; j+=l_NumCommand )
				{
					if( j+i >= l_Meshes.size() ) break;

					RenderableMesh *l_pInst = reinterpret_cast<RenderableMesh *>(l_Meshes[j+i].get());
					MeshAsset *l_pMeshIst = l_pInst->getMesh()->getComponent<MeshAsset>();
					MeshAsset::Instance *l_pSubMeshInst = l_pMeshIst->getMeshes()[l_pInst->getMeshIdx()];

					std::shared_ptr<Asset> l_pTexture = l_pInst->getMaterial(0)->getComponent<MaterialAsset>()->getFirstTexture();
					if( nullptr == l_pTexture ) l_pTexture = EngineCore::singleton().getWhiteTexture();
					
					// draw dir shadow map
					{
						m_DrawCommand[i]->useProgram(m_pDirShadowMat->getComponent<MaterialAsset>()->getProgram());
						m_pDirShadowMat->getComponent<MaterialAsset>()->setTexture("m_DiffTex", l_pTexture);

						std::vector<glm::ivec4> l_InstanceData(l_DirLights.size() * 4);
						for( unsigned int k=0 ; k<l_DirLights.size() ; ++k )
						{
							for( unsigned int l=0 ; l<4 ; ++l )
							{
								l_InstanceData[k*4 + l].x = l_pInst->getWorldOffset();
								l_InstanceData[k*4 + l].y = l_DirLights[k]->getID();
								l_InstanceData[k*4 + l].z = l;
							}
						}
						int l_InstanceBuffer = getScene()->getRenderBatcher()->requestInstanceVtxBuffer();
						GDEVICE()->updateVertexBuffer(l_InstanceBuffer, l_InstanceData.data(), sizeof(glm::ivec4) * l_InstanceData.size());

						m_DrawCommand[i]->bindVertex(l_pMeshIst->getVertexBuffer().get(), l_InstanceBuffer);
						m_DrawCommand[i]->bindIndex(l_pMeshIst->getIndexBuffer().get());
						m_pDirShadowMat->getComponent<MaterialAsset>()->bindAll(m_DrawCommand[i]);
						m_DrawCommand[i]->drawElement(l_pSubMeshInst->m_StartIndex, l_pSubMeshInst->m_IndexCount, 0, l_InstanceData.size(), 0);
						getScene()->getRenderBatcher()->recycleInstanceVtxBuffer(l_InstanceBuffer);
					}

					// draw omni shadow map
					{
						m_DrawCommand[i]->useProgram(m_pOmniShadowMat->getComponent<MaterialAsset>()->getProgram());
						m_pOmniShadowMat->getComponent<MaterialAsset>()->setTexture("m_DiffTex", l_pTexture);

						std::vector<glm::ivec4> l_InstanceData;
						for( unsigned int k=0 ; k<l_OmniLights.size() ; ++k )
						{
							bool l_Temp = false;
							if( !reinterpret_cast<OmniLight *>(l_OmniLights[k].get())->getShadowCamera()->getFrustum().intersect(l_pInst->boundingBox(), l_Temp) ) continue;

							glm::ivec4 l_NewInst;
							l_NewInst.x = l_pInst->getWorldOffset();
							l_NewInst.y = l_OmniLights[k]->getID();
							l_InstanceData.push_back(l_NewInst);
						}

						if( !l_InstanceData.empty() )
						{
							int l_InstanceBuffer = getScene()->getRenderBatcher()->requestInstanceVtxBuffer();
							GDEVICE()->updateVertexBuffer(l_InstanceBuffer, l_InstanceData.data(), sizeof(glm::ivec4) * l_InstanceData.size());
						
							m_DrawCommand[i]->bindVertex(l_pMeshIst->getVertexBuffer().get(), l_InstanceBuffer);
							m_DrawCommand[i]->bindIndex(l_pMeshIst->getIndexBuffer().get());
							m_pOmniShadowMat->getComponent<MaterialAsset>()->bindAll(m_DrawCommand[i]);

							m_DrawCommand[i]->drawElement(l_pSubMeshInst->m_StartIndex, l_pSubMeshInst->m_IndexCount, 0, l_InstanceData.size(), 0);
							getScene()->getRenderBatcher()->recycleInstanceVtxBuffer(l_InstanceBuffer);
						}
					}

					// draw spot shadow map
					{
						m_DrawCommand[i]->useProgram(m_pSpotShadowMat->getComponent<MaterialAsset>()->getProgram());
						m_pSpotShadowMat->getComponent<MaterialAsset>()->setTexture("m_DiffTex", l_pTexture);

						std::vector<glm::ivec4> l_InstanceData;
						for( unsigned int k=0 ; k<l_SpotLights.size() ; ++k )
						{
							bool l_Temp = false;
							if( !reinterpret_cast<OmniLight *>(l_SpotLights[k].get())->getShadowCamera()->getFrustum().intersect(l_pInst->boundingBox(), l_Temp) ) continue;

							glm::ivec4 l_NewInst;
							l_NewInst.x = l_pInst->getWorldOffset();
							l_NewInst.y = l_SpotLights[k]->getID();
							l_InstanceData.push_back(l_NewInst);
						}

						if( !l_InstanceData.empty() )
						{
							int l_InstanceBuffer = getScene()->getRenderBatcher()->requestInstanceVtxBuffer();
							GDEVICE()->updateVertexBuffer(l_InstanceBuffer, l_InstanceData.data(), sizeof(glm::ivec4) * l_InstanceData.size());
						
							m_DrawCommand[i]->bindVertex(l_pMeshIst->getVertexBuffer().get(), l_InstanceBuffer);
							m_DrawCommand[i]->bindIndex(l_pMeshIst->getIndexBuffer().get());
							m_pSpotShadowMat->getComponent<MaterialAsset>()->bindAll(m_DrawCommand[i]);

							m_DrawCommand[i]->drawElement(l_pSubMeshInst->m_StartIndex, l_pSubMeshInst->m_IndexCount, 0, l_InstanceData.size(), 0);
							getScene()->getRenderBatcher()->recycleInstanceVtxBuffer(l_InstanceBuffer);
						}
					}
				}
				
				m_DrawCommand[i]->end();
			});
		}
		m_ThreadPool.join();
	}

	{// graphic step, divide by stage
		//bind gbuffer
		m_pCmdInit->begin(false);

		glm::viewport l_Viewport(0.0f, 0.0f, EngineSetting::singleton().m_DefaultSize.x, EngineSetting::singleton().m_DefaultSize.y, 0.0f, 1.0f);
		m_pCmdInit->setViewPort(1, l_Viewport);
		m_pCmdInit->setScissor(1, glm::ivec4(0, 0, EngineSetting::singleton().m_DefaultSize.x, EngineSetting::singleton().m_DefaultSize.y));

		for( unsigned int i=0 ; i<GBUFFER_COUNT - 1 ; ++i )
		{
			m_pCmdInit->clearRenderTarget(m_pGBuffer[i]->getComponent<TextureAsset>()->getTextureID(), glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
		}
		m_pCmdInit->clearDepthTarget(m_pGBuffer[GBUFFER_DEPTH]->getComponent<TextureAsset>()->getTextureID(), true, 1.0f, false, 0);

		m_pCmdInit->end();

		// draw gbuffer
		unsigned int l_CurrIdx = 0;
		drawOpaqueMesh(a_pCamera, m_pGBuffer[GBUFFER_DEPTH]->getComponent<TextureAsset>()->getTextureID(), m_GBufferCache, l_Meshes, l_CurrIdx);
		
		{// copy depth data
			m_pCmdInit->begin(false);
			
			m_pCmdInit->useProgram(DefaultPrograms::CopyDepth);
			m_pCmdInit->setRenderTarget(-1, 1, m_pDepthMinmax->getComponent<TextureAsset>()->getTextureID());

			glm::ivec3 l_DepthSize(m_pDepthMinmax->getComponent<TextureAsset>()->getDimension());
			glm::viewport l_DepthView(0.0f, 0.0f, l_DepthSize.x, l_DepthSize.y, 0.0f, 1.0f);
			m_pCmdInit->setViewPort(1, l_DepthView);
			m_pCmdInit->setScissor(1, glm::ivec4(0, 0, l_DepthSize.x, l_DepthSize.y));

			m_pCmdInit->bindVertex(EngineCore::singleton().getQuadBuffer().get());
			m_pCmdInit->bindTexture(m_pGBuffer[GBUFFER_DEPTH]->getComponent<TextureAsset>()->getTextureID(), 0, true);
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
		, std::vector<std::shared_ptr<RenderableComponent>> &a_StaticLight, std::vector<std::shared_ptr<RenderableComponent>> &a_Light
		, std::vector<std::shared_ptr<RenderableComponent>> &a_StaticMesh, std::vector<std::shared_ptr<RenderableComponent>> &a_Mesh)
{
	getScene()->getSceneGraph(GRAPH_MESH)->getVisibleList(a_pCamera, a_Mesh);
	getScene()->getSceneGraph(GRAPH_STATIC_MESH)->getVisibleList(a_pCamera, a_StaticMesh);
	if( a_Mesh.empty() && a_StaticMesh.empty() ) return false;

	getScene()->getSceneGraph(GRAPH_STATIC_LIGHT)->getVisibleList(a_pCamera, a_StaticLight);
	getScene()->getSceneGraph(GRAPH_LIGHT)->getVisibleList(a_pCamera, a_Light);

	if( (int)a_Light.size() >= m_LightIdx->getBlockSize() / (sizeof(unsigned int) * 2) )
	{
		m_LightIdx->extend(m_ExtendSize / 2);
	}

	return true;
}

void DeferredRenderer::setupIndexUav(std::vector< std::shared_ptr<RenderableComponent> > &a_Light, std::vector< std::shared_ptr<RenderableComponent> > &a_Mesh)
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

unsigned int DeferredRenderer::calculateShadowMapRegion(std::shared_ptr<Camera> a_pCamera, std::shared_ptr<Light> &a_Light)
{
	if( COMPONENT_DirLight == a_Light->typeID() ) return 1024;
	assert(a_pCamera->getCameraType() == Camera::ORTHO || a_pCamera->getCameraType() == Camera::PERSPECTIVE);

	glm::sphere l_Sphere(glm::vec3(a_Light->boundingBox().m_Center), glm::length(a_Light->boundingBox().m_Size) * 0.5);
	if( l_Sphere.m_Range <= glm::epsilon<float>() ) return 32;

	float l_SliceLength = 0.0f;
	if( a_pCamera->getCameraType() == Camera::ORTHO ) l_SliceLength = std::min(a_pCamera->getViewParam().x, a_pCamera->getViewParam().y);
	else
	{
		glm::vec3 l_Eye, l_Dir, l_Up;
		a_pCamera->getCameraParam(l_Eye, l_Dir, l_Up);
		l_SliceLength = glm::dot(l_Sphere.m_Center - l_Eye, l_Dir) * tan(a_pCamera->getViewParam().x * 0.5f);
		if( a_pCamera->getViewParam().y < 1.0f ) l_SliceLength *= a_pCamera->getViewParam().y;
		l_SliceLength *= 2.0f;
	}

	unsigned int l_Res = EngineSetting::singleton().m_ShadowMapSize;
	while( l_Sphere.m_Range < l_SliceLength * 0.5f )
	{
		l_Res = l_Res << 1;
		l_SliceLength *= 0.5f;
		if( l_Res <= 32 )
		{
			l_Res = 32;
			break;
		}
	}

	return l_Res;
}

void DeferredRenderer::requestShadowMapRegion(unsigned int a_Size, std::shared_ptr<Light> &a_Light)
{
	glm::vec2 l_TexutureSize(m_pShadowMap->getMaxSize());
	glm::vec4 l_UV[4];
	unsigned int l_LayerID[4];

	unsigned int l_Loop = COMPONENT_DirLight == a_Light->typeID() ? 4 : 1;
	{
		std::lock_guard<std::mutex> l_Guard(m_ShadowMapLock);
		for( unsigned int i=0 ; i<l_Loop ; ++i )
		{
			unsigned int l_RegionID = m_pShadowMap->allocate(glm::ivec2(a_Size));
			ImageAtlas::NodeInfo &l_Info = m_pShadowMap->getInfo(l_RegionID);
			l_LayerID[i] = l_Info.m_Index;
			l_UV[i] = glm::vec4(l_Info.m_Offset.x / l_TexutureSize.x, l_Info.m_Offset.y / l_TexutureSize.y, l_Info.m_Size.x / l_TexutureSize.x, l_Info.m_Size.y / l_TexutureSize.y);
		}
	}
	
	switch( a_Light->typeID() )
	{
		case COMPONENT_OmniLight:{
			OmniLight *l_pCasted = reinterpret_cast<OmniLight *>(a_Light.get());
			l_pCasted->setShadowMapUV(l_UV[0], l_LayerID[0]);
			}return;

		case COMPONENT_SpotLight:{
			SpotLight *l_pCasted = reinterpret_cast<SpotLight *>(a_Light.get());
			l_pCasted->setShadowMapUV(l_UV[0], l_LayerID[0]);
			}return;

		case COMPONENT_DirLight:{
			DirLight *l_pCasted = reinterpret_cast<DirLight *>(a_Light.get());
			for( unsigned int i=0 ; i<4 ; ++i ) l_pCasted->setShadowMapLayer(i, l_UV[i], l_LayerID[i]);
			}return;

		default: break;
	}
	assert(false && "invalid light type");
}

void DeferredRenderer::drawOpaqueMesh(std::shared_ptr<Camera> a_pCamera, int a_DepthTexture, std::vector<int> &a_RenderTargets, std::vector< std::shared_ptr<RenderableComponent> > &a_Mesh, unsigned int &a_OpaqueEnd)
{
	/*unsigned int l_StageID = static_cast<RenderableMesh *>(a_Mesh.front().get())->getStage();
	for( unsigned int i=0 ; i<a_Mesh.size() ; ++i )
	{
		unsigned int l_CurrStageID = static_cast<RenderableMesh *>(a_Mesh[i].get())->getStage();
		if( l_StageID != l_CurrStageID )
		{
			drawMesh(a_pCamera, a_DepthTexture, a_RenderTargets, a_Mesh, a_OpaqueEnd, i);
			l_StageID = l_CurrStageID;
			a_OpaqueEnd = i;
		}
		else if( i + 1 == a_Mesh.size() )
		{
			drawMesh(a_pCamera, a_DepthTexture, a_RenderTargets, a_Mesh, a_OpaqueEnd, i + 1);
			a_OpaqueEnd = i;
		}
			
		if( l_CurrStageID > OPAQUE_STAGE_END ) return ;
	}*/
}

void DeferredRenderer::drawMesh(std::shared_ptr<Camera> a_pCamera, int a_DepthTexture, std::vector<int> &a_RenderTargets, std::vector< std::shared_ptr<RenderableComponent> > &a_Mesh, unsigned int a_Start, unsigned int a_End)
{
	unsigned int l_CommandCount = m_DrawCommand.size();

	glm::viewport l_Viewport(0.0f, 0.0f, EngineSetting::singleton().m_DefaultSize.x, EngineSetting::singleton().m_DefaultSize.y, 0.0f, 1.0f);

	//#pragma omp parallel for
	for( int i=0 ; i<(int)l_CommandCount ; ++i )
	{
		GraphicCommander *l_pCommandList = m_DrawCommand[i];
		unsigned int l_Start = a_Start+i;
		if( l_Start > a_End ) continue;

		l_pCommandList->begin(false);
		for( unsigned int j=a_Start+i ; j<a_End ; j+=l_CommandCount )
		{
			/*RenderableMesh *l_pMesh = static_cast<RenderableMesh *>(a_Mesh[j].get());
			for( unsigned int k=0 ; k<l_pMesh->getNumSubMesh() ; ++k )
			{
				std::shared_ptr<Material> l_pMaterial = l_pMesh->getMaterial(k);
				l_pMaterial->setBlock("g_Camera", a_pCamera->getMaterialBlock());

				l_pCommandList->useProgram(l_pMaterial->getProgram());
				l_pCommandList->setRenderTarget(a_DepthTexture, a_RenderTargets);
				l_pCommandList->setViewPort(1, l_Viewport);
				l_pCommandList->setScissor(1, glm::ivec4(0, 0, EngineSetting::singleton().m_DefaultSize.x, EngineSetting::singleton().m_DefaultSize.y));
				
				l_pCommandList->setTopology(Topology::triangle_list);
				l_pCommandList->bindVertex(l_pMesh->getVtxBuffer().get());
				l_pCommandList->bindIndex(l_pMesh->getIndexBuffer().get());
				l_pMaterial->bindAll(l_pCommandList);
				l_pCommandList->drawElement(0, l_pMesh->getIndexBuffer()->getNumIndicies(), 0);
			}*/
		}
		l_pCommandList->end();
	}
}
#pragma endregion

}