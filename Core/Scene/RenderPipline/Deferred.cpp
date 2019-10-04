// Deferred.cpp
//
// 2017/08/14 Ian Wu/Real0000
//

#include "CommonUtil.h"
#include "RGDeviceWrapper.h"

#include "Core.h"
#include "RenderObject/Material.h"
#include "RenderObject/Mesh.h"
#include "RenderObject/Light.h"
#include "Scene/Camera.h"
#include "Scene/Scene.h"
#include "Scene/Graph/ScenePartition.h"
#include "Texture/Texture.h"
#include "Texture/TextureAtlas.h"

#include "Deferred.h"

namespace R
{

#pragma region DeferredRenderer
//
// DeferredRenderer
//
DeferredRenderer::DeferredRenderer(SharedSceneMember *a_pSharedMember)
	: RenderPipeline(a_pSharedMember)
	, m_pCmdInit(nullptr)
	, m_LightIdx(nullptr), m_MeshIdx(nullptr)
	, m_ExtendSize(INIT_CONTAINER_SIZE)
	, m_pShadowMap(new RenderTextureAtlas(glm::ivec2(EngineSetting::singleton().m_ShadowMapSize, EngineSetting::singleton().m_ShadowMapSize), PixelFormat::r32_float))
	, m_ShadowCmdIdx(0)
	, m_TileDim(0, 0)
	, m_pFrameBuffer(nullptr)
	, m_pDepthMinmax(nullptr)
	, m_MinmaxStepCount(1)
	, m_pLightIndexMat(Material::create(ProgramManager::singleton().getData(DefaultPrograms::TiledLightIntersection)))
	, m_pDeferredLightMat(Material::create(ProgramManager::singleton().getData(DefaultPrograms::TiledDeferredLighting)))
	, m_pCopyMat(Material::create(ProgramManager::singleton().getData(DefaultPrograms::Copy)))
	, m_ThreadPool(std::thread::hardware_concurrency())
{
	m_pShadowMapDepth = TextureManager::singleton().createRenderTarget(wxT("RenderTextureAtlasDepth"), glm::ivec2(EngineSetting::singleton().m_ShadowMapSize), PixelFormat::d32_float);

	const std::pair<wxString, PixelFormat::Key> c_GBufferDef[] = {
		{wxT("DefferredGBufferNormal"), PixelFormat::rgba8_snorm},
		{wxT("DefferredGBufferMaterial"), PixelFormat::rgba8_uint},
		{wxT("DefferredGBufferDiffuse"), PixelFormat::rgba8_unorm},
		{wxT("DefferredGBufferMask"), PixelFormat::rgba8_unorm},
		{wxT("DefferredGBufferFactor"), PixelFormat::rgba8_unorm},
		{wxT("DefferredGBufferMotionBlur"), PixelFormat::rg16_float},
		{wxT("DefferredGBufferDepth"), PixelFormat::d24_unorm_s8_uint}};
	for( unsigned int i=0 ; i<GBUFFER_COUNT ; ++i )
	{
		m_pGBuffer[i] = TextureManager::singleton().createRenderTarget(c_GBufferDef[i].first, EngineSetting::singleton().m_DefaultSize, c_GBufferDef[i].second);

		char l_Buff[8];
		snprintf(l_Buff, 8, "GBuff%d", i);
		m_pDeferredLightMat->setTexture(l_Buff, m_pGBuffer[i]);
	}
	m_pFrameBuffer = TextureManager::singleton().createRenderTarget(wxT("DefferredFrame"), EngineSetting::singleton().m_DefaultSize, PixelFormat::rgba16_float);

	m_LightIdx = m_pLightIndexMat->createExternalBlock(ShaderRegType::UavBuffer, "g_SrcLights", m_ExtendSize / 2); // {index, type}
	m_MeshIdx = m_pLightIndexMat->createExternalBlock(ShaderRegType::UavBuffer, "g_SrcLights", m_ExtendSize / 4);// use packed int

	m_pCmdInit = GDEVICE()->commanderFactory();

	m_TileDim.x = std::ceil(EngineSetting::singleton().m_DefaultSize.x / EngineSetting::singleton().m_TileSize);
	m_TileDim.y = std::ceil(EngineSetting::singleton().m_DefaultSize.y / EngineSetting::singleton().m_TileSize);
	m_pDepthMinmax = TextureManager::singleton().createRenderTarget(wxT("DefferredDepthMinmax"), EngineSetting::singleton().m_DefaultSize, PixelFormat::rg16_float);
	m_MinmaxStepCount = std::ceill(log2f(EngineSetting::singleton().m_TileSize));
	m_TiledValidLightIdx = m_pLightIndexMat->createExternalBlock(ShaderRegType::UavBuffer, "g_DstLights", m_TileDim.x * m_TileDim.y * INIT_LIGHT_SIZE / 2); // {index, type}

	m_pLightIndexMat->setParam<glm::vec2>("c_PixelSize", 0, glm::vec2(1.0f / EngineSetting::singleton().m_DefaultSize.x, 1.0f / EngineSetting::singleton().m_DefaultSize.y));
	m_pLightIndexMat->setParam<int>("c_MipmapLevel", 0, (int)m_MinmaxStepCount);
	m_pLightIndexMat->setParam<glm::ivec2>("c_TileCount", 0, m_TileDim);
	
	m_pLightIndexMat->setBlock("g_SrcLights", m_LightIdx);
	m_pLightIndexMat->setBlock("g_DstLights", m_TiledValidLightIdx);
	m_pLightIndexMat->setBlock("g_OmniLights", a_pSharedMember->m_pOmniLights->getMaterialBlock());
	m_pLightIndexMat->setBlock("g_SpotLights", a_pSharedMember->m_pSpotLights->getMaterialBlock());

	m_pCopyMat->setTexture("m_SrcTex", m_pFrameBuffer);

	m_DrawCommand.resize(EngineSetting::singleton().m_NumRenderCommandList);
	for( unsigned int i=0 ; i<m_DrawCommand.size() ; ++i ) m_DrawCommand[i] = GDEVICE()->commanderFactory();

	m_GBufferCache.resize(GBUFFER_COUNT - 1);
	for( unsigned int i=GBUFFER_NORMAL ; i<=GBUFFER_MOTIONBLUR ; ++i ) m_GBufferCache[i] = m_pGBuffer[i]->getTextureID();
	m_FBufferCache.resize(1, m_pFrameBuffer->getTextureID());
}

DeferredRenderer::~DeferredRenderer()
{
	m_pDepthMinmax->release();
	m_pDepthMinmax = nullptr;
	m_pFrameBuffer->release();
	m_pFrameBuffer = nullptr;
	for( unsigned int i=0 ; i<GBUFFER_COUNT ; ++i )
	{
		m_pGBuffer[i]->release();
		m_pGBuffer[i] = nullptr;
	}

	m_pShadowMapDepth->release();
	m_pShadowMapDepth = nullptr;
	SAFE_DELETE(m_pShadowMap)

	for( unsigned int i=0 ; i<m_ShadowCommands.size() ; ++i ) delete m_ShadowCommands[i];
	m_ShadowCommands.clear();

	SAFE_DELETE(m_pCmdInit);
	for( unsigned int i=0 ; i<m_DrawCommand.size() ; ++i ) SAFE_DELETE(m_DrawCommand[i])
	m_DrawCommand.clear();
	// add memset ?

	m_pDeferredLightMat = nullptr;
	m_pLightIndexMat = nullptr;
	m_LightIdx = nullptr;
	m_MeshIdx = nullptr;
	m_TiledValidLightIdx = nullptr;
}

void DeferredRenderer::render(std::shared_ptr<CameraComponent> a_pCamera, GraphicCanvas *a_pCanvas)
{
	std::vector< std::shared_ptr<RenderableComponent> > l_Lights, l_Meshes;
	if( !setupVisibleList(a_pCamera, l_Lights, l_Meshes) )
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
		unsigned int l_ShadowedLights = 0;
		m_ThreadPool.addJob([=, &l_Lights, &l_Meshes](){ this->setupIndexUav(l_Lights, l_Meshes);});
		
		for( unsigned int i=0 ; i<l_Lights.size() ; ++i )
		{
			auto l_pLight = l_Lights[i]->shared_from_base<Light>();
			if( l_pLight->getShadowed() )
			{
				m_ThreadPool.addJob([=, &l_pLight, &l_Meshes](){ this->setupShadow(l_pLight->getShadowCamera(), l_pLight, l_Meshes);});
				++l_ShadowedLights;
			}
		}
		
		m_ThreadPool.join();

		getSharedMember()->m_pDirLights->flush();
		getSharedMember()->m_pOmniLights->flush();
		getSharedMember()->m_pSpotLights->flush();
		if( l_CurrShadowMapSize != m_pShadowMap->getArraySize() )
		{
			m_pShadowMapDepth->release();
			m_pShadowMapDepth = TextureManager::singleton().createRenderTarget(wxT("RenderTextureAtlasDepth"), m_pShadowMap->getMaxSize(), PixelFormat::d32_float, m_pShadowMap->getArraySize());
		}

		// add required command execute thread
		while( m_ShadowCommands.size() <= l_ShadowedLights ) m_ShadowCommands.push_back(GDEVICE()->commanderFactory());

		m_pCmdInit->begin(false);

		glm::viewport l_Viewport(0.0f, 0.0f, EngineSetting::singleton().m_ShadowMapSize, EngineSetting::singleton().m_ShadowMapSize, 0.0f, 1.0f);
		m_pCmdInit->setRenderTarget(m_pShadowMapDepth->getTextureID(), 1, m_pShadowMap->getTexture()->getTextureID());
		m_pCmdInit->setViewPort(1, l_Viewport);
		m_pCmdInit->setScissor(1, glm::ivec4(0, 0, EngineSetting::singleton().m_ShadowMapSize, EngineSetting::singleton().m_ShadowMapSize));

		m_pCmdInit->clearRenderTarget(m_pShadowMap->getTexture()->getTextureID(), glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
		m_pCmdInit->clearDepthTarget(m_pShadowMapDepth->getTextureID(), true, 1.0f, false, 0);

		m_pCmdInit->end();

		m_ShadowCmdIdx = 0;
		for( unsigned int i=0 ; i<l_Lights.size() ; ++i )
		{
			auto l_pLight = l_Lights[i]->shared_from_base<Light>();
			if( l_pLight->getShadowed() ) m_ThreadPool.addJob([=, &l_pLight, &l_Meshes](){ this->drawShadow(l_pLight, l_Meshes);});
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
			m_pCmdInit->clearRenderTarget(m_pGBuffer[i]->getTextureID(), glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
		}
		m_pCmdInit->clearDepthTarget(m_pGBuffer[GBUFFER_DEPTH]->getTextureID(), true, 1.0f, false, 0);

		m_pCmdInit->end();

		// draw gbuffer
		unsigned int l_CurrIdx = 0;
		drawOpaqueMesh(a_pCamera, m_pGBuffer[GBUFFER_DEPTH]->getTextureID(), m_GBufferCache, l_Meshes, l_CurrIdx);
		
		{// copy depth data
			m_pCmdInit->begin(false);
			
			m_pCmdInit->useProgram(DefaultPrograms::CopyDepth);
			m_pCmdInit->setRenderTarget(-1, 1, m_pDepthMinmax->getTextureID());

			glm::ivec3 l_DepthSize(m_pDepthMinmax->getDimension());
			glm::viewport l_DepthView(0.0f, 0.0f, l_DepthSize.x, l_DepthSize.y, 0.0f, 1.0f);
			m_pCmdInit->setViewPort(1, l_DepthView);
			m_pCmdInit->setScissor(1, glm::ivec4(0, 0, l_DepthSize.x, l_DepthSize.y));

			m_pCmdInit->bindVertex(EngineCore::singleton().getQuadBuffer().get());
			m_pCmdInit->bindTexture(m_pGBuffer[GBUFFER_DEPTH]->getTextureID(), 0, true);
			m_pCmdInit->setTopology(Topology::triangle_strip);
			m_pCmdInit->drawVertex(4, 0);

			m_pCmdInit->end();
		}
		// do fence ?
		m_pDepthMinmax->generateMipmap(m_MinmaxStepCount, ProgramManager::singleton().getData(DefaultPrograms::GenerateMinmaxDepth));

		m_pCmdInit->begin(true);
		
		m_pCmdInit->useProgram(DefaultPrograms::TiledLightIntersection);
		m_pLightIndexMat->setBlock("g_Camera", a_pCamera->getMaterialBlock());
		m_pLightIndexMat->setParam<int>("c_NumLight", 0, (int)l_Lights.size());
		m_pLightIndexMat->bindAll(m_pCmdInit);
		m_pCmdInit->compute(m_TileDim.x, m_TileDim.y);

		m_pCmdInit->end();

		// draw frame buffer
		m_pCmdInit->begin(false);
		
		m_pCmdInit->useProgram(m_pDeferredLightMat->getProgram());
		m_pCmdInit->setRenderTarget(m_pGBuffer[GBUFFER_DEPTH]->getTextureID(), 1, m_pFrameBuffer->getTextureID());
		m_pCmdInit->clearRenderTarget(m_pFrameBuffer->getTextureID(), glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));

		glm::ivec3 l_FrameSize(m_pFrameBuffer->getDimension());
		glm::viewport l_FrameView(0.0f, 0.0f, l_FrameSize.x, l_FrameSize.y, 0.0f, 1.0f);
		m_pCmdInit->setViewPort(1, l_FrameView);
		m_pCmdInit->setScissor(1, glm::ivec4(0, 0, l_FrameSize.x, l_FrameSize.y));

		m_pCmdInit->bindVertex(EngineCore::singleton().getQuadBuffer().get());
		m_pDeferredLightMat->bindAll(m_pCmdInit);
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

			m_pCmdInit->useProgram(m_pCopyMat->getProgram());
			m_pCmdInit->setRenderTargetWithBackBuffer(-1, a_pCanvas);
			m_pCmdInit->clearBackBuffer(a_pCanvas, glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
			
			wxSize l_CanvasSize(a_pCanvas->GetClientSize());
			glm::viewport l_CanvasView(0.0f, 0.0f, l_CanvasSize.x, l_CanvasSize.y, 0.0f, 1.0f);
			m_pCmdInit->setViewPort(1, l_CanvasView);
			m_pCmdInit->setScissor(1, glm::ivec4(0, 0, l_CanvasSize.x, l_CanvasSize.y));

			m_pCmdInit->bindVertex(EngineCore::singleton().getQuadBuffer().get());
			m_pCopyMat->bindAll(m_pCmdInit);
			m_pCmdInit->setTopology(Topology::triangle_strip);
			m_pCmdInit->drawVertex(4, 0);

			m_pCmdInit->end();

			a_pCanvas->present();
		}
	}
}

bool DeferredRenderer::setupVisibleList(std::shared_ptr<CameraComponent> a_pCamera, std::vector< std::shared_ptr<RenderableComponent> > &a_Light, std::vector< std::shared_ptr<RenderableComponent> > &a_Mesh)
{
	getSharedMember()->m_pGraphs[SharedSceneMember::GRAPH_MESH]->getVisibleList(a_pCamera, a_Mesh);
	getSharedMember()->m_pGraphs[SharedSceneMember::GRAPH_LIGHT]->getVisibleList(a_pCamera, a_Light);
	if( a_Mesh.empty() ) return false;

	std::sort(a_Mesh.begin(), a_Mesh.end(), 
		[](const std::shared_ptr<RenderableComponent> &a_This, const std::shared_ptr<RenderableComponent> &a_Other) -> bool
		{
			RenderableMesh *l_pThis = reinterpret_cast<RenderableMesh *>(a_This.get());
			RenderableMesh *l_pOther = reinterpret_cast<RenderableMesh *>(a_Other.get());
			return l_pThis->getStage() > l_pOther->getStage();
		});
	
	if( (int)a_Light.size() >= m_LightIdx->getBlockSize() / (sizeof(unsigned int) * 2) )
	{
		m_LightIdx->extend(m_ExtendSize / 2);
	}

	unsigned int l_NumMesh = 0;
	for( unsigned int i=0 ; i<a_Mesh.size() ; ++i )
	{
		RenderableMesh *l_pThis = reinterpret_cast<RenderableMesh *>(a_Mesh[i].get());
		l_NumMesh += l_pThis->getNumSubMesh();
	}

	if( (int)l_NumMesh >= m_MeshIdx->getBlockSize() / sizeof(unsigned int) )
	{
		unsigned int l_ExtentUnit = m_ExtendSize / sizeof(unsigned int);
		l_ExtentUnit = (l_NumMesh / l_ExtentUnit + 1) * l_ExtentUnit;
		m_MeshIdx->extend(l_ExtentUnit);
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

	l_pBuff = m_MeshIdx->getBlockPtr(0);
	unsigned int l_Offset = 0;
	for( unsigned int i=0 ; i<a_Mesh.size() ; ++i )
	{
		RenderableMesh *l_pMesh = reinterpret_cast<RenderableMesh *>(a_Mesh[i].get());
		unsigned int *l_pTarget = reinterpret_cast<unsigned int*>(l_pBuff + sizeof(unsigned int) * l_Offset);
		for( unsigned int j=0 ; j<l_pMesh->getNumSubMesh() ; ++j )
		{
			l_pTarget[j] = l_pMesh->getCommandID(j);
		}
	}
	m_MeshIdx->sync(true);
}

void DeferredRenderer::setupShadow(std::shared_ptr<CameraComponent> a_pCamera, std::shared_ptr<Light> &a_Light, std::vector< std::shared_ptr<RenderableComponent> > &a_Mesh)
{
	if( !a_Light->getShadowed() ) return;

	auto l_IdxContainer = a_Light->getShadowMapIndex();
	l_IdxContainer.clear();

	unsigned int l_Size = calculateShadowMapRegion(a_pCamera, a_Light);
	requestShadowMapRegion(l_Size, a_Light);
	for( unsigned int i=0 ; i<a_Mesh.size() ; ++i )
	{
		if( a_pCamera->boundingBox().intersect(a_Mesh[i]->boundingBox()) ) l_IdxContainer.push_back(i);
	}
}

void DeferredRenderer::drawShadow(std::shared_ptr<Light> &a_Light, std::vector< std::shared_ptr<RenderableComponent> > &a_Mesh)
{
	GraphicCommander *l_pCmdList = nullptr;
	{
		std::lock_guard<std::mutex> l_IdxGuard(m_ShadowMapLock);
		l_pCmdList = m_ShadowCommands[m_ShadowCmdIdx];
		++m_ShadowCmdIdx;
	}

	l_pCmdList->begin(false);

	unsigned int l_ShadowMapSize = EngineSetting::singleton().m_ShadowMapSize;
	switch( a_Light->typeID() )
	{
		case COMPONENT_DIR_LIGHT:{
			glm::viewport l_Viewport(0.0f, 0.0f, l_ShadowMapSize, l_ShadowMapSize, 0.0f, 1.0f);
			l_pCmdList->setViewPort(1, l_Viewport);
			l_pCmdList->setScissor(1, glm::ivec4(0, 0, l_Viewport.m_Size.x, l_Viewport.m_Size.y));
			}break;

		case COMPONENT_OMNI_LIGHT:{
			OmniLight *l_pOmni = reinterpret_cast<OmniLight *>(a_Light.get());
			glm::vec4 l_ShadowMapUV(l_pOmni->getShadowMapUV());
			glm::viewport l_Viewport(l_ShadowMapSize, l_ShadowMapSize, l_ShadowMapSize, l_ShadowMapSize, 0.0f, 1.0f);
			l_Viewport.m_Start.x *= l_ShadowMapUV.x;
			l_Viewport.m_Start.y *= l_ShadowMapUV.y;
			l_Viewport.m_Size.x *= l_ShadowMapUV.z;
			l_Viewport.m_Size.y *= l_ShadowMapUV.w;
			l_pCmdList->setViewPort(1, l_Viewport);
			l_pCmdList->setScissor(1, glm::ivec4(l_Viewport.m_Start.x, l_Viewport.m_Start.y, l_Viewport.m_Start.x + l_Viewport.m_Size.x, l_Viewport.m_Start.y + l_Viewport.m_Size.y));
			}break;

		case COMPONENT_SPOT_LIGHT:{
			SpotLight *l_pSpot = reinterpret_cast<SpotLight *>(a_Light.get());
			glm::vec4 l_ShadowMapUV(l_pSpot->getShadowMapUV());
			glm::viewport l_Viewport(l_ShadowMapSize, l_ShadowMapSize, l_ShadowMapSize, l_ShadowMapSize, 0.0f, 1.0f);
			l_Viewport.m_Start.x *= l_ShadowMapUV.x;
			l_Viewport.m_Start.y *= l_ShadowMapUV.y;
			l_Viewport.m_Size.x *= l_ShadowMapUV.z;
			l_Viewport.m_Size.y *= l_ShadowMapUV.w;
			l_pCmdList->setViewPort(1, l_Viewport);
			l_pCmdList->setScissor(1, glm::ivec4(l_Viewport.m_Start.x, l_Viewport.m_Start.y, l_Viewport.m_Start.x + l_Viewport.m_Size.x, l_Viewport.m_Start.y + l_Viewport.m_Size.y));
			}break;
	}

	l_pCmdList->end();
}

unsigned int DeferredRenderer::calculateShadowMapRegion(std::shared_ptr<CameraComponent> a_pCamera, std::shared_ptr<Light> &a_Light)
{
	if( COMPONENT_DIR_LIGHT == a_Light->typeID() ) return m_pShadowMap->getMaxSize().x;
	assert(a_pCamera->getCameraType() == CameraComponent::ORTHO || a_pCamera->getCameraType() == CameraComponent::PERSPECTIVE);

	glm::sphere l_Sphere(glm::vec3(a_Light->boundingBox().m_Center), glm::length(a_Light->boundingBox().m_Size) * 0.5);
	if( l_Sphere.m_Range <= glm::epsilon<float>() ) return 32;

	float l_SliceLength = 0.0f;
	if( a_pCamera->getCameraType() == CameraComponent::ORTHO ) l_SliceLength = std::min(a_pCamera->getViewParam().x, a_pCamera->getViewParam().y);
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
	unsigned int l_RegionID = 0;
	{
		std::lock_guard<std::mutex> l_Guard(m_ShadowMapLock);
		l_RegionID = m_pShadowMap->allocate(glm::ivec2(a_Size));
	}
	ImageAtlas::NodeInfo &l_Info = m_pShadowMap->getInfo(l_RegionID);

	glm::vec2 l_TexutureSize(m_pShadowMap->getMaxSize());
	glm::vec4 l_UV(l_Info.m_Offset.x / l_TexutureSize.x, l_Info.m_Offset.y / l_TexutureSize.y, l_Info.m_Size.x / l_TexutureSize.x, l_Info.m_Size.y / l_TexutureSize.y);
	switch( a_Light->typeID() )
	{
		case COMPONENT_OMNI_LIGHT:{
			OmniLight *l_pCasted = reinterpret_cast<OmniLight *>(a_Light.get());
			l_pCasted->setShadowMapUV(l_UV, l_Info.m_Index);
			}return;

		case COMPONENT_SPOT_LIGHT:{
			SpotLight *l_pCasted = reinterpret_cast<SpotLight *>(a_Light.get());
			l_pCasted->setShadowMapUV(l_UV, l_Info.m_Index);
			}return;

		case COMPONENT_DIR_LIGHT:{
			DirLight *l_pCasted = reinterpret_cast<DirLight *>(a_Light.get());
			l_pCasted->setShadowMapLayer(l_Info.m_Index);
			}return;

		default: break;
	}
	assert(false && "invalid light type");
}

void DeferredRenderer::drawOpaqueMesh(std::shared_ptr<CameraComponent> a_pCamera, int a_DepthTexture, std::vector<int> &a_RenderTargets, std::vector< std::shared_ptr<RenderableComponent> > &a_Mesh, unsigned int &a_OpaqueEnd)
{
	unsigned int l_StageID = static_cast<RenderableMesh *>(a_Mesh.front().get())->getStage();
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
	}
}

void DeferredRenderer::drawMesh(std::shared_ptr<CameraComponent> a_pCamera, int a_DepthTexture, std::vector<int> &a_RenderTargets, std::vector< std::shared_ptr<RenderableComponent> > &a_Mesh, unsigned int a_Start, unsigned int a_End)
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
			RenderableMesh *l_pMesh = static_cast<RenderableMesh *>(a_Mesh[j].get());
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
			}
		}
		l_pCommandList->end();
	}
}
#pragma endregion

}