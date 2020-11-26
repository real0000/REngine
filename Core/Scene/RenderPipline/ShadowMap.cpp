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

#define SHADOWMAP_ASSET_NAME wxT("DefferredRenderTextureAtlasDepth.Image")
#define CLEAR_MAT_ASSET_NAME wxT("ShadowMapClear.Material")

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
	, m_pClearMat(AssetManager::singleton().createAsset(CLEAR_MAT_ASSET_NAME).second)
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

	recycleAllIndirectBuffer();
	while( !m_IndirectBufferPool.empty() )
	{
		delete m_IndirectBufferPool.front();
		m_IndirectBufferPool.pop_front();
	}
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

void ShadowMapRenderer::bake(std::vector<std::shared_ptr<RenderableComponent>> &a_Lights
							, std::vector<std::shared_ptr<RenderableComponent>> &a_StaticMesh
							, std::vector<std::shared_ptr<RenderableComponent>> &a_Mesh
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

	std::map<MaterialAsset*, std::map<SceneBatcher::MeshVtxCache, std::vector<RenderableMesh*>>> l_SortedMesh[3];// dir, omni, spot
	{
		auto l_InsertFunc = [=, &l_SortedMesh](unsigned int a_SortedIdx, unsigned int a_MatSlot, RenderableMesh *a_pMesh, SceneBatcher::MeshCache *a_pBatchInfo) -> void
		{
			std::shared_ptr<Asset> l_pMat = a_pMesh->getMaterial(a_MatSlot);
			if( nullptr != l_pMat )
			{
				MaterialAsset *l_pMatInst = l_pMat->getComponent<MaterialAsset>();

				std::map<SceneBatcher::MeshVtxCache, std::vector<RenderableMesh*>> *l_pTargetContainer = nullptr;
				auto it = l_SortedMesh[a_SortedIdx].find(l_pMatInst);
				if( it == l_SortedMesh[a_SortedIdx].end() ) l_pTargetContainer = &(l_SortedMesh[a_SortedIdx][l_pMatInst] = std::map<SceneBatcher::MeshVtxCache, std::vector<RenderableMesh*>>());
				else l_pTargetContainer = &(it->second);

				SceneBatcher::MeshVtxCache &l_VtxCache = (*a_pBatchInfo)[a_pMesh->getMeshIdx()];
				std::vector<RenderableMesh*> *l_pMeshInstContainer = nullptr;
				auto vecIt = l_pTargetContainer->find(l_VtxCache);
				if( l_pTargetContainer->end() == vecIt) l_pMeshInstContainer = &((*l_pTargetContainer)[l_VtxCache] = std::vector<RenderableMesh*>());
				else l_pMeshInstContainer = &(vecIt->second);
				l_pMeshInstContainer->push_back(a_pMesh);
			}
		};

		EngineCore::singleton().addJob([=]() -> void
		{
			for( unsigned int i=0 ; i<a_StaticMesh.size() ; ++i )
			{
				RenderableMesh *l_pMeshInst = reinterpret_cast<RenderableMesh *>(a_StaticMesh[i].get());
				SceneBatcher::MeshCache *l_pBatchInfo = nullptr;
				if( !getScene()->getRenderBatcher()->getBatchParam(l_pMeshInst->getMesh(), &l_pBatchInfo) ) continue;

				l_InsertFunc(0, MATSLOT_DIR_SHADOWMAP, l_pMeshInst, l_pBatchInfo);
				l_InsertFunc(1, MATSLOT_OMNI_SHADOWMAP, l_pMeshInst, l_pBatchInfo);
				l_InsertFunc(2, MATSLOT_SPOT_SHADOWMAP, l_pMeshInst, l_pBatchInfo);
			}
		});
	}
		
	EngineCore::singleton().join();

	a_pMiscCmd->begin(true);
	a_pMiscCmd->useProgram(m_pClearMatInst->getProgram());
	a_pMiscCmd->compute(m_pShadowMap->getMaxSize().x / 8, m_pShadowMap->getMaxSize().y / 8, m_pShadowMap->getArraySize());
	a_pMiscCmd->end();

	getScene()->getDirLightContainer()->flush();
	getScene()->getOmniLightContainer()->flush();
	getScene()->getSpotLightContainer()->flush();

	// add required command execute thread
	while( m_ShadowCommands.size() <= m_pShadowMap->getArraySize() ) m_ShadowCommands.push_back(GDEVICE()->commanderFactory());

	a_pMiscCmd->begin(false);

	glm::viewport l_Viewport(0.0f, 0.0f, m_pShadowMap->getMaxSize().y, m_pShadowMap->getMaxSize().y, 0.0f, 1.0f);
	a_pMiscCmd->setRenderTarget(-1, 0);
	a_pMiscCmd->setViewPort(1, l_Viewport);
	a_pMiscCmd->setScissor(1, glm::ivec4(0, 0, l_Viewport.m_Size.x, l_Viewport.m_Size.y));

	a_pMiscCmd->end();

	unsigned int l_NumCommand = std::min(a_Mesh.size(), a_DrawCommand.size());
	for( unsigned int i=0 ; i<l_NumCommand ; ++i )
	{
		EngineCore::singleton().addJob([=, &i, &l_NumCommand, &l_DirLights, &l_OmniLights, &l_SpotLights, &a_Mesh, &a_StaticMesh]()
		{
			a_DrawCommand[i]->begin(false);

			for( unsigned int j=0 ; j<a_Mesh.size() ; j+=l_NumCommand )
			{
				if( j+i >= a_Mesh.size() ) break;

				RenderableMesh *l_pInst = reinterpret_cast<RenderableMesh *>(a_Mesh[j+i].get());
				if( !l_pInst->getShadowed() ) continue;

				MeshAsset *l_pMeshIst = l_pInst->getMesh()->getComponent<MeshAsset>();
				MeshAsset::Instance *l_pSubMeshInst = l_pMeshIst->getMeshes()[l_pInst->getMeshIdx()];

				// draw dir shadow map
				std::shared_ptr<Asset> l_pDirShadowMat = l_pInst->getMaterial(MATSLOT_DIR_SHADOWMAP);
				if( nullptr != l_pDirShadowMat )
				{
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
					
					drawLightShadow(a_DrawCommand[i]
						, l_pDirShadowMat->getComponent<MaterialAsset>(), getScene()->getDirLightContainer()->getMaterialBlock()
						, l_InstanceData, l_pMeshIst->getVertexBuffer().get(), l_pMeshIst->getIndexBuffer().get()
						, [=]() -> void
						{
							a_DrawCommand[i]->drawElement(l_pSubMeshInst->m_StartIndex, l_pSubMeshInst->m_IndexCount, 0, l_InstanceData.size(), 0);
						});
				}

				// draw omni shadow map
				std::shared_ptr<Asset> l_pOmniShadowMat = l_pInst->getMaterial(MATSLOT_OMNI_SHADOWMAP);
				if( nullptr != l_pOmniShadowMat )
				{
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
						drawLightShadow(a_DrawCommand[i]
							, l_pOmniShadowMat->getComponent<MaterialAsset>(), getScene()->getOmniLightContainer()->getMaterialBlock()
							, l_InstanceData, l_pMeshIst->getVertexBuffer().get(), l_pMeshIst->getIndexBuffer().get()
							, [=]() -> void
							{
								a_DrawCommand[i]->drawElement(l_pSubMeshInst->m_StartIndex, l_pSubMeshInst->m_IndexCount, 0, l_InstanceData.size(), 0);
							});
					}
				}

				// draw spot shadow map
				std::shared_ptr<Asset> l_pSpotShadowMat = l_pInst->getMaterial(MATSLOT_SPOT_SHADOWMAP);
				if( nullptr != l_pSpotShadowMat )
				{
					std::vector<glm::ivec4> l_InstanceData;
					for( unsigned int k=0 ; k<l_SpotLights.size() ; ++k )
					{
						bool l_Temp = false;
						if( !reinterpret_cast<SpotLight *>(l_SpotLights[k].get())->getShadowCamera()->getFrustum().intersect(l_pInst->boundingBox(), l_Temp) ) continue;

						glm::ivec4 l_NewInst;
						l_NewInst.x = l_pInst->getWorldOffset();
						l_NewInst.y = l_SpotLights[k]->getID();
						l_InstanceData.push_back(l_NewInst);
					}
					
					if( !l_InstanceData.empty() )
					{
						drawLightShadow(a_DrawCommand[i]
							, l_pSpotShadowMat->getComponent<MaterialAsset>(), getScene()->getSpotLightContainer()->getMaterialBlock()
							, l_InstanceData, l_pMeshIst->getVertexBuffer().get(), l_pMeshIst->getIndexBuffer().get()
							, [=]() -> void
							{
								a_DrawCommand[i]->drawElement(l_pSubMeshInst->m_StartIndex, l_pSubMeshInst->m_IndexCount, 0, l_InstanceData.size(), 0);
							});
					}
				}
			}
			
			//dir
			for( auto it=l_SortedMesh[0].begin() ; it!=l_SortedMesh[0].end() ; ++it )
			{
				std::vector<glm::ivec4> l_Instance;
				IndirectDrawBuffer *l_pIndirectBuffer = requestIndirectBuffer();
				for( auto cmdIt=it->second.begin() ; cmdIt!=it->second.end() ; ++cmdIt )
				{
					IndirectDrawData l_TempData;
					l_TempData.m_BaseVertex = cmdIt->first.m_VertexStart;
					l_TempData.m_StartIndex = cmdIt->first.m_IndexStart;
					l_TempData.m_IndexCount = cmdIt->first.m_IndexCount;
					l_TempData.m_StartInstance = l_Instance.size();
					l_TempData.m_InstanceCount = 0;
					for( unsigned int j=0 ; j<cmdIt->second.size() ; j+=l_NumCommand )
					{
						RenderableMesh *l_pMeshInst = cmdIt->second[j];
						for( unsigned int k=0 ; k<l_DirLights.size() ; ++k )
						{	
							for( unsigned int l=0 ; l<4 ; ++l )
							{
								glm::ivec4 l_NewInst;
								l_NewInst.x = l_pMeshInst->getWorldOffset();
								l_NewInst.y = l_DirLights[k]->getID();
								l_NewInst.z = l;
								l_Instance.push_back(l_NewInst);
							}
							l_TempData.m_InstanceCount += 4;
						}
					}
					l_pIndirectBuffer->assign(l_TempData);
				}
				drawLightShadow(a_DrawCommand[i]
					, it->first, getScene()->getDirLightContainer()->getMaterialBlock()
					, l_Instance, getScene()->getRenderBatcher()->getVertexBuffer().get(), getScene()->getRenderBatcher()->getIndexBuffer().get()
					, [=]() -> void
					{
						a_DrawCommand[i]->drawIndirect(l_pIndirectBuffer->getCurrCount(), l_pIndirectBuffer->getID());
					});
				l_Instance.clear();
			}

			//omni
			for( auto it=l_SortedMesh[1].begin() ; it!=l_SortedMesh[1].end() ; ++it )
			{
				std::vector<glm::ivec4> l_Instance;
				IndirectDrawBuffer *l_pIndirectBuffer = requestIndirectBuffer();
				for( auto cmdIt=it->second.begin() ; cmdIt!=it->second.end() ; ++cmdIt )
				{
					IndirectDrawData l_TempData;
					l_TempData.m_BaseVertex = cmdIt->first.m_VertexStart;
					l_TempData.m_StartIndex = cmdIt->first.m_IndexStart;
					l_TempData.m_IndexCount = cmdIt->first.m_IndexCount;
					l_TempData.m_StartInstance = l_Instance.size();
					l_TempData.m_InstanceCount = 0;
					for( unsigned int j=0 ; j<cmdIt->second.size() ; j+=l_NumCommand )
					{
						RenderableMesh *l_pMeshInst = cmdIt->second[j];
						for( unsigned int k=0 ; k<l_OmniLights.size() ; ++k )
						{
							bool l_Temp = false;
							if( !reinterpret_cast<OmniLight *>(l_OmniLights[k].get())->getShadowCamera()->getFrustum().intersect(l_pMeshInst->boundingBox(), l_Temp) ) continue;

							glm::ivec4 l_NewInst;
							l_NewInst.x = l_pMeshInst->getWorldOffset();
							l_NewInst.y = l_OmniLights[k]->getID();
							l_Instance.push_back(l_NewInst);
							++l_TempData.m_InstanceCount;
						}
					}
					l_pIndirectBuffer->assign(l_TempData);
				}
				drawLightShadow(a_DrawCommand[i]
					, it->first, getScene()->getOmniLightContainer()->getMaterialBlock()
					, l_Instance, getScene()->getRenderBatcher()->getVertexBuffer().get(), getScene()->getRenderBatcher()->getIndexBuffer().get()
					, [=]() -> void
					{
						a_DrawCommand[i]->drawIndirect(l_pIndirectBuffer->getCurrCount(), l_pIndirectBuffer->getID());
					});
				l_Instance.clear();
			}

			//spot
			for( auto it=l_SortedMesh[2].begin() ; it!=l_SortedMesh[2].end() ; ++it )
			{
				std::vector<glm::ivec4> l_Instance;
				IndirectDrawBuffer *l_pIndirectBuffer = requestIndirectBuffer();
				for( auto cmdIt=it->second.begin() ; cmdIt!=it->second.end() ; ++cmdIt )
				{
					IndirectDrawData l_TempData;
					l_TempData.m_BaseVertex = cmdIt->first.m_VertexStart;
					l_TempData.m_StartIndex = cmdIt->first.m_IndexStart;
					l_TempData.m_IndexCount = cmdIt->first.m_IndexCount;
					l_TempData.m_StartInstance = l_Instance.size();
					l_TempData.m_InstanceCount = 0;
					for( unsigned int j=0 ; j<cmdIt->second.size() ; j+=l_NumCommand )
					{
						RenderableMesh *l_pMeshInst = cmdIt->second[j];
						for( unsigned int k=0 ; k<l_SpotLights.size() ; ++k )
						{
							bool l_Temp = false;
							if( !reinterpret_cast<SpotLight *>(l_SpotLights[k].get())->getShadowCamera()->getFrustum().intersect(l_pMeshInst->boundingBox(), l_Temp) ) continue;

							glm::ivec4 l_NewInst;
							l_NewInst.x = l_pMeshInst->getWorldOffset();
							l_NewInst.y = l_SpotLights[k]->getID();
							l_Instance.push_back(l_NewInst);
						}
					}
					l_pIndirectBuffer->assign(l_TempData);
				}
				drawLightShadow(a_DrawCommand[i]
					, it->first, getScene()->getSpotLightContainer()->getMaterialBlock()
					, l_Instance, getScene()->getRenderBatcher()->getVertexBuffer().get(), getScene()->getRenderBatcher()->getIndexBuffer().get()
					, [=]() -> void
					{
						a_DrawCommand[i]->drawIndirect(l_pIndirectBuffer->getCurrCount(), l_pIndirectBuffer->getID());
					});
				l_Instance.clear();
			}

			a_DrawCommand[i]->end();
		});
	}
	EngineCore::singleton().join();
	recycleAllIndirectBuffer();
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
		, std::function<void()> a_DrawFunc)
{
	a_pCmd->useProgram(a_pMat->getProgram());
	int l_InstanceBuffer = getScene()->getRenderBatcher()->requestInstanceVtxBuffer();
	GDEVICE()->updateVertexBuffer(l_InstanceBuffer, a_InstanceData.data(), sizeof(glm::ivec4) * a_InstanceData.size());
						
	a_pMat->setTexture("ShadowMap", m_pShadowMap->getTexture());
	a_pCmd->bindVertex(a_pVtxBuff, l_InstanceBuffer);
	a_pCmd->bindIndex(a_pIdxBuff);
	a_pMat->bindBlock(a_pCmd, STANDARD_TRANSFORM_NORMAL, getScene()->getRenderBatcher()->getWorldMatrixBlock());
	a_pMat->bindBlock(a_pCmd, STANDARD_TRANSFORM_SKIN, getScene()->getRenderBatcher()->getSkinMatrixBlock());
	a_pMat->bindBlock(a_pCmd, "Lights", a_pLightBlock);
	a_pMat->bindAll(a_pCmd);
	a_DrawFunc();
	getScene()->getRenderBatcher()->recycleInstanceVtxBuffer(l_InstanceBuffer);
}

IndirectDrawBuffer* ShadowMapRenderer::requestIndirectBuffer()
{
	std::lock_guard<std::mutex> l_Guard(m_BufferLock);
	IndirectDrawBuffer *l_pRes = nullptr;
	if( m_IndirectBufferPool.empty() ) l_pRes = new IndirectDrawBuffer();
	else
	{
		l_pRes = m_IndirectBufferPool.front();
		m_IndirectBufferPool.pop_front();
	}
	m_IndirectBufferInUse.push_back(l_pRes);
	return l_pRes;
}

void ShadowMapRenderer::recycleAllIndirectBuffer()
{
	for( auto it=m_IndirectBufferInUse.begin() ; it!=m_IndirectBufferInUse.end() ; ++it )
	{
		(*it)->reset();
		m_IndirectBufferPool.push_back(*it);
	}
	m_IndirectBufferInUse.clear();
}
#pragma endregion

}