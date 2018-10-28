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
	, m_ExtendSize(INIT_CONTAINER_SIZE)
	, m_pShadowMap(new RenderTextureAtlas(glm::ivec2(EngineSetting::singleton().m_ShadowMapSize, EngineSetting::singleton().m_ShadowMapSize), PixelFormat::r32_float))
	, m_ShadowCmdIdx(0)
{
	m_pShadowMapDepth = TextureManager::singleton().createRenderTarget(wxT("RenderTextureAtlasDepth"), glm::ivec2(EngineSetting::singleton().m_ShadowMapSize), PixelFormat::d32_float);

	m_LightIdxUav.m_UavId = GDEVICE()->requestUavBuffer(m_LightIdxUav.m_pBuffer, sizeof(LightInfo), INIT_CONTAINER_SIZE);
	m_LightIdxUav.m_UavSize = INIT_CONTAINER_SIZE;
	
	m_MeshIdxUav.m_UavId = GDEVICE()->requestUavBuffer(m_MeshIdxUav.m_pBuffer, sizeof(MeshInfo), INIT_CONTAINER_SIZE);
	m_MeshIdxUav.m_UavSize = INIT_CONTAINER_SIZE;

	m_pCmdInit = GDEVICE()->commanderFactory();
}

DeferredRenderer::~DeferredRenderer()
{
	m_pShadowMapDepth->release();
	SAFE_DELETE(m_pShadowMap)

	for( unsigned int i=0 ; i<m_ShadowCommands.size() ; ++i ) delete m_ShadowCommands[i];
	m_ShadowCommands.clear();

	SAFE_DELETE(m_pCmdInit);
	// add memset ?

	GDEVICE()->freeUavBuffer(m_LightIdxUav.m_UavId);
	GDEVICE()->freeUavBuffer(m_MeshIdxUav.m_UavId);
}

void DeferredRenderer::render(std::shared_ptr<CameraComponent> a_pCamera)
{
	std::vector< std::shared_ptr<RenderableComponent> > l_Lights, l_Meshes;
	if( !setupVisibleList(a_pCamera, l_Lights, l_Meshes) ) return;
	
	m_pShadowMap->releaseAll();

	{// shadow step
		unsigned int l_CurrShadowMapSize = m_pShadowMap->getArraySize();
		unsigned int l_ShadowedLights = 0;
		std::thread l_UavThread(&DeferredRenderer::setupIndexUav, this, l_Lights, l_Meshes);
		std::vector<std::thread> l_LightCmdThread;

		for( unsigned int i=0 ; i<l_Lights.size() ; ++i )
		{
			auto l_pLight = l_Lights[i]->shared_from_base<Light>();
			if( l_pLight->getShadowed() )
			{
				l_LightCmdThread.emplace_back(std::thread(&DeferredRenderer::setupShadow, this, l_pLight->getShadowCamera(), l_pLight, l_Meshes));
				++l_ShadowedLights;
			}
		}
		
		l_UavThread.join();
		for( unsigned int i=0 ; i<l_LightCmdThread.size() ; ++i ) l_LightCmdThread[i].join();
		l_LightCmdThread.clear();

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
			if( l_pLight->getShadowed() ) l_LightCmdThread.emplace_back(std::thread(&DeferredRenderer::drawShadow, this, l_pLight, l_Meshes));
		}
		for( unsigned int i=0 ; i<l_LightCmdThread.size() ; ++i ) l_LightCmdThread[i].join();
	}

	{// graphic step, divide by stage
		
		
		unsigned int l_CurrIdx = 0;
		unsigned int l_StageID = static_cast<RenderableMesh *>(l_Meshes.front().get())->getMaterial()->getStage();
		for( unsigned int i=0 ; i<l_Meshes.size() ; ++i )
		{
			if( l_StageID != static_cast<RenderableMesh *>(l_Meshes[i].get())->getMaterial()->getStage() )
			{
				drawMesh(l_Meshes, l_CurrIdx, i);
				l_CurrIdx = i;
			}
			else if( i + 1 == l_Meshes.size() )
			{
				drawMesh(l_Meshes, l_CurrIdx, i + 1);
				l_CurrIdx = i;
			}
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
			return l_pThis->getMaterial()->getStage() > l_pOther->getMaterial()->getStage();
		});
	
	if( (int)a_Light.size() >= m_LightIdxUav.m_UavSize )
	{
		m_LightIdxUav.m_UavSize = ((a_Light.size() / m_ExtendSize) + 1) * m_ExtendSize;
		GDEVICE()->resizeUavBuffer(m_LightIdxUav.m_UavId, m_LightIdxUav.m_pBuffer, m_LightIdxUav.m_UavSize);
	}

	if( (int)a_Mesh.size() >= m_MeshIdxUav.m_UavSize )
	{
		m_MeshIdxUav.m_UavSize = ((a_Mesh.size() / m_ExtendSize) + 1) * m_ExtendSize;
		GDEVICE()->resizeUavBuffer(m_MeshIdxUav.m_UavId, m_MeshIdxUav.m_pBuffer, m_MeshIdxUav.m_UavSize);
	}
	return true;
}

void DeferredRenderer::setupIndexUav(std::vector< std::shared_ptr<RenderableComponent> > &a_Light, std::vector< std::shared_ptr<RenderableComponent> > &a_Mesh)
{
	for( unsigned int i=0 ; i<a_Light.size() ; ++i )
	{
		LightInfo *l_pTarget = reinterpret_cast<LightInfo *>(m_LightIdxUav.m_pBuffer + sizeof(LightInfo) * i);
		Light *l_pLight = reinterpret_cast<Light *>(a_Light[i].get());
		l_pTarget->m_Index = l_pLight->getID();
		l_pTarget->m_Type = l_pLight->typeID();
	}
	GDEVICE()->setUavBufferCounter(m_LightIdxUav.m_UavId, a_Light.size());

	for( unsigned int i=0 ; i<a_Mesh.size() ; ++i )
	{
		MeshInfo *l_pTarget = reinterpret_cast<MeshInfo *>(m_MeshIdxUav.m_pBuffer + sizeof(MeshInfo) * i);
		RenderableMesh *l_pMesh = reinterpret_cast<RenderableMesh *>(a_Mesh[i].get());
		l_pTarget->m_Index = l_pMesh->getCommandID();
	}
	GDEVICE()->setUavBufferCounter(m_MeshIdxUav.m_UavId, a_Mesh.size());

	GDEVICE()->syncUavBuffer(true, 2, m_MeshIdxUav.m_UavId, m_LightIdxUav.m_UavId);
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

void DeferredRenderer::drawMesh(std::vector< std::shared_ptr<RenderableComponent> > &a_Mesh, unsigned int a_Start, unsigned int a_End)
{
	for( unsigned int i=a_Start ; i<a_End ; ++i )
	{
	}
}
#pragma endregion

}