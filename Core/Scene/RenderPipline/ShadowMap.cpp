// ShadowMap.cpp
//
// 2020/11/16 Ian Wu/Real0000
//

#include "CommonUtil.h"
#include "RGDeviceWrapper.h"

#include "Core.h"
#include "ShadowMap.h"

#include "Asset/MaterialAsset.h"
#include "Asset/MeshAsset.h"
#include "Asset/TextureAsset.h"
#include "RenderObject/Mesh.h"
#include "RenderObject/Light.h"
#include "RenderObject/TextureAtlas.h"
#include "Scene/Camera.h"
#include "Scene/Scene.h"


namespace R
{

#pragma region ShadowMapRenderer
//
// ShadowMapRenderer
//
ShadowMapRenderer* ShadowMapRenderer::create(boost::property_tree::ptree &a_Src, std::shared_ptr<Scene> a_pScene)
{
	return new ShadowMapRenderer(a_pScene);
}

ShadowMapRenderer::ShadowMapRenderer(std::shared_ptr<Scene> a_pScene)
	: RenderPipeline(a_pScene)
	, m_pShadowMap(new RenderTextureAtlas(glm::ivec2(EngineSetting::singleton().m_ShadowMapSize, EngineSetting::singleton().m_ShadowMapSize), PixelFormat::r32_uint))
	, m_pClearMat(AssetManager::singleton().createAsset(CLEAR_MAT_ASSET_NAME))
	, m_pClearMatInst(nullptr)
{
	m_pClearMatInst = m_pClearMat->getComponent<MaterialAsset>();
	m_pClearMatInst->init(ProgramManager::singleton().getData(DefaultPrograms::ClearShadowMap));
	m_pClearMatInst->setTexture("ShadowMap", m_pShadowMap->getTexture());
}

ShadowMapRenderer::~ShadowMapRenderer()
{
	AssetManager::singleton().removeData(SHADOWMAP_ASSET_NAME);
	m_pClearMat = nullptr;
	m_pClearMatInst = nullptr;

	SAFE_DELETE(m_pShadowMap)

	for( unsigned int i=0 ; i<m_ShadowCommands.size() ; ++i ) delete m_ShadowCommands[i];
	m_ShadowCommands.clear();
}

void ShadowMapRenderer::saveSetting(boost::property_tree::ptree &a_Dst)
{
	boost::property_tree::ptree l_Attr;
	l_Attr.add("type", ShadowMapRenderer::typeName());

	a_Dst.add_child("<xmlattr>", l_Attr);
}

void ShadowMapRenderer::render(std::shared_ptr<Camera> a_pCamera, GraphicCanvas *a_pCanvas)
{
}

void ShadowMapRenderer::drawFlagChanged(unsigned int a_Flag)
{
	
}

void ShadowMapRenderer::bake(std::vector<std::shared_ptr<RenderableComponent>> &a_Lights
							, std::vector<RenderableMesh*> &a_SortedDir
							, std::vector<RenderableMesh*> &a_SortedOmni
							, std::vector<RenderableMesh*> &a_SortedSpot
							, GraphicCommander *a_pMiscCmd
							, std::vector<GraphicCommander *> &a_DrawCommand)
{
	m_pShadowMap->releaseAll();

	unsigned int l_CurrShadowMapSize = m_pShadowMap->getArraySize();
		
	std::vector<std::shared_ptr<DirLight>> l_DirLights;
	std::vector<std::shared_ptr<OmniLight>> l_OmniLights;
	std::vector<std::shared_ptr<SpotLight>> l_SpotLights;
	for( unsigned int i=0 ; i<a_Lights.size() ; ++i )
	{
		auto l_pLight = a_Lights[i]->shared_from_base<Light>();
		if( l_pLight->getShadowed() )
		{
			EngineCore::singleton().addJob([=, &l_pLight]()
			{
				unsigned int l_Size = calculateShadowMapRegion(l_pLight->getShadowCamera(), l_pLight);
				requestShadowMapRegion(l_Size, l_pLight);
			});
			switch( l_pLight->getID() )
			{
				case COMPONENT_DirLight:	l_DirLights.push_back(a_Lights[i]->shared_from_base<DirLight>());	break;
				case COMPONENT_OmniLight:	l_OmniLights.push_back(a_Lights[i]->shared_from_base<OmniLight>());	break;
				case COMPONENT_SpotLight:	l_SpotLights.push_back(a_Lights[i]->shared_from_base<SpotLight>());	break;
				default:break;
			}
		}
	}
		
	EngineCore::singleton().join();

	a_pMiscCmd->begin(true);
	a_pMiscCmd->useProgram(m_pClearMatInst->getProgram());
	a_pMiscCmd->compute(m_pShadowMap->getMaxSize().x / 8, m_pShadowMap->getMaxSize().y / 8, m_pShadowMap->getArraySize());
	a_pMiscCmd->end();

	// add required command execute thread
	while( m_ShadowCommands.size() <= m_pShadowMap->getArraySize() ) m_ShadowCommands.push_back(GDEVICE()->commanderFactory());

	a_pMiscCmd->begin(false);

	glm::viewport l_Viewport(0.0f, 0.0f, m_pShadowMap->getMaxSize().y, m_pShadowMap->getMaxSize().y, 0.0f, 1.0f);
	a_pMiscCmd->setRenderTarget(-1, 0);
	a_pMiscCmd->setViewPort(1, l_Viewport);
	a_pMiscCmd->setScissor(1, glm::ivec4(0, 0, l_Viewport.m_Size.x, l_Viewport.m_Size.y));

	a_pMiscCmd->end();

	unsigned int l_NumCommand = a_DrawCommand.size();
	for( unsigned int i=0 ; i<l_NumCommand ; ++i )
	{
		EngineCore::singleton().addJob([=, &l_DirLights, &l_OmniLights, &l_SpotLights, &a_SortedDir, &a_SortedOmni, &a_SortedSpot]()
		{
			a_DrawCommand[i]->begin(false);
			
			//dir
			getScene()->getRenderBatcher()->drawSortedMeshes(a_DrawCommand[i]
				, a_SortedDir, i, l_NumCommand, MATSLOT_DIR_SHADOWMAP
				, [=](MaterialAsset *a_pMat) -> void
				{
					a_pMat->bindTexture(a_DrawCommand[i], "ShadowMap", m_pShadowMap->getTexture());
					a_pMat->bindBlock(a_DrawCommand[i], "Lights", getScene()->getDirLightContainer()->getMaterialBlock());
				}
				, [=](std::vector<glm::ivec4> &a_Instance, unsigned int a_Idx) -> unsigned int
				{
					for( unsigned int k=0 ; k<l_DirLights.size() ; ++k )
					{	
						for( unsigned int l=0 ; l<4 ; ++l )
						{
							glm::ivec4 l_NewInst;
							l_NewInst.x = a_SortedDir[a_Idx]->getWorldOffset();
							l_NewInst.y = l_DirLights[k]->getID();
							l_NewInst.z = l;
							a_Instance.push_back(l_NewInst);
						}
					}
					return l_DirLights.size() * 4;
				});
				
			//omni
			getScene()->getRenderBatcher()->drawSortedMeshes(a_DrawCommand[i]
				, a_SortedOmni, i, l_NumCommand, MATSLOT_OMNI_SHADOWMAP
				, [=](MaterialAsset *a_pMat) -> void
				{
					a_pMat->bindTexture(a_DrawCommand[i], "ShadowMap", m_pShadowMap->getTexture());
					a_pMat->bindBlock(a_DrawCommand[i], "Lights", getScene()->getOmniLightContainer()->getMaterialBlock());
				}
				, [=](std::vector<glm::ivec4> &a_Instance, unsigned int a_Idx) -> unsigned int
				{
					unsigned int l_NumInstanceAdd = 0;
					for( unsigned int k=0 ; k<l_OmniLights.size() ; ++k )
					{
						bool l_Temp = false;
						if( !reinterpret_cast<OmniLight *>(l_OmniLights[k].get())->getShadowCamera()->getFrustum().intersect(a_SortedOmni[a_Idx]->boundingBox(), l_Temp) ) continue;

						glm::ivec4 l_NewInst;
						l_NewInst.x = a_SortedOmni[a_Idx]->getWorldOffset();
						l_NewInst.y = l_OmniLights[k]->getID();
						a_Instance.push_back(l_NewInst);
						++l_NumInstanceAdd;
					}
					return l_NumInstanceAdd;
				});

			//spot
			getScene()->getRenderBatcher()->drawSortedMeshes(a_DrawCommand[i]
				, a_SortedSpot, i, l_NumCommand, MATSLOT_SPOT_SHADOWMAP
				, [=](MaterialAsset *a_pMat) -> void
				{
					a_pMat->bindTexture(a_DrawCommand[i], "ShadowMap", m_pShadowMap->getTexture());
					a_pMat->bindBlock(a_DrawCommand[i], "Lights", getScene()->getSpotLightContainer()->getMaterialBlock());
				}
				, [=](std::vector<glm::ivec4> &a_Instance, unsigned int a_Idx) -> unsigned int
				{
					unsigned int l_NumInstanceAdd = 0;
					for( unsigned int k=0 ; k<l_SpotLights.size() ; ++k )
					{
						bool l_Temp = false;
						if( !reinterpret_cast<SpotLight *>(l_SpotLights[k].get())->getShadowCamera()->getFrustum().intersect(a_SortedSpot[a_Idx]->boundingBox(), l_Temp) ) continue;

						glm::ivec4 l_NewInst;
						l_NewInst.x = a_SortedSpot[a_Idx]->getWorldOffset();
						l_NewInst.y = l_SpotLights[k]->getID();
						a_Instance.push_back(l_NewInst);
						++l_NumInstanceAdd;
					}
					return l_NumInstanceAdd;
				});

			a_DrawCommand[i]->end();
		});
	}
	EngineCore::singleton().join();
}

std::shared_ptr<Asset> ShadowMapRenderer::getShadowMap()
{
	return m_pShadowMap->getTexture();
}

unsigned int ShadowMapRenderer::calculateShadowMapRegion(std::shared_ptr<Camera> a_pCamera, std::shared_ptr<Light> &a_Light)
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

void ShadowMapRenderer::requestShadowMapRegion(unsigned int a_Size, std::shared_ptr<Light> &a_Light)
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

void ShadowMapRenderer::drawLightShadow(GraphicCommander *a_pCmd
		, MaterialAsset *a_pMat, std::shared_ptr<MaterialBlock> a_pLightBlock
		, std::vector<glm::ivec4> &a_InstanceData, VertexBuffer *a_pVtxBuff, IndexBuffer *a_pIdxBuff
		, unsigned int a_IndirectData, int a_BuffID)
{
	a_pCmd->useProgram(a_pMat->getProgram());
	int l_InstanceBuffer = getScene()->getRenderBatcher()->requestInstanceVtxBuffer();
	GDEVICE()->updateVertexBuffer(l_InstanceBuffer, a_InstanceData.data(), sizeof(glm::ivec4) * a_InstanceData.size());
	
	a_pCmd->bindVertex(a_pVtxBuff, l_InstanceBuffer);
	a_pCmd->bindIndex(a_pIdxBuff);
	a_pMat->bindAll(a_pCmd);
	a_pCmd->drawIndirect(a_IndirectData, a_BuffID);
	getScene()->getRenderBatcher()->recycleInstanceVtxBuffer(l_InstanceBuffer);
}
#pragma endregion

}