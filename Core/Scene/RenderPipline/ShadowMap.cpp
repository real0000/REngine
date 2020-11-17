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
	, m_pShadowMap(new RenderTextureAtlas(glm::ivec2(EngineSetting::singleton().m_ShadowMapSize, EngineSetting::singleton().m_ShadowMapSize), PixelFormat::d32_float))
{
}

ShadowMapRenderer::~ShadowMapRenderer()
{
	AssetManager::singleton().removeData(SHADOWMAP_ASSET_NAME);

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

	std::vector<RenderableMesh*> l_SortedMesh[3];// dir, omni, spot
	{
		l_SortedMesh[0].resize(a_StaticMesh.size());
		for( unsigned int i=0 ; i<a_StaticMesh.size() ; ++i ) l_SortedMesh[0][i] = reinterpret_cast<RenderableMesh *>(a_StaticMesh[i].get());
		memcpy(l_SortedMesh[1].data(), l_SortedMesh[0].data(), sizeof(RenderableMesh*) * a_StaticMesh.size());
		memcpy(l_SortedMesh[2].data(), l_SortedMesh[0].data(), sizeof(RenderableMesh*) * a_StaticMesh.size());
		std::sort(l_SortedMesh[0].begin(), l_SortedMesh[0].end(), [=](RenderableMesh *a_pLeft, RenderableMesh *a_pRight) -> bool
		{
			return a_pLeft->getMaterial(MATSLOT_DIR_SHADOWMAP) >= a_pRight->getMaterial(MATSLOT_DIR_SHADOWMAP);
		});
		std::sort(l_SortedMesh[1].begin(), l_SortedMesh[1].end(), [=](RenderableMesh *a_pLeft, RenderableMesh *a_pRight) -> bool
		{
			return a_pLeft->getMaterial(MATSLOT_OMNI_SHADOWMAP) >= a_pRight->getMaterial(MATSLOT_OMNI_SHADOWMAP);
		});
		std::sort(l_SortedMesh[2].begin(), l_SortedMesh[2].end(), [=](RenderableMesh *a_pLeft, RenderableMesh *a_pRight) -> bool
		{
			return a_pLeft->getMaterial(MATSLOT_SPOT_SHADOWMAP) >= a_pRight->getMaterial(MATSLOT_SPOT_SHADOWMAP);
		});
	}
		
	EngineCore::singleton().join();

	getScene()->getDirLightContainer()->flush();
	getScene()->getOmniLightContainer()->flush();
	getScene()->getSpotLightContainer()->flush();

	// add required command execute thread
	while( m_ShadowCommands.size() <= m_pShadowMap->getArraySize() ) m_ShadowCommands.push_back(GDEVICE()->commanderFactory());

	a_pMiscCmd->begin(false);

	glm::viewport l_Viewport(0.0f, 0.0f, EngineSetting::singleton().m_ShadowMapSize, EngineSetting::singleton().m_ShadowMapSize, 0.0f, 1.0f);
	a_pMiscCmd->setRenderTarget(m_pShadowMap->getTexture()->getComponent<TextureAsset>()->getTextureID(), 0);
	a_pMiscCmd->setViewPort(1, l_Viewport);
	a_pMiscCmd->setScissor(1, glm::ivec4(0, 0, EngineSetting::singleton().m_ShadowMapSize, EngineSetting::singleton().m_ShadowMapSize));
	a_pMiscCmd->clearDepthTarget(m_pShadowMap->getTexture()->getComponent<TextureAsset>()->getTextureID(), true, 1.0f, false, 0);

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
					MaterialAsset *l_pDirShadowMatInst = l_pDirShadowMat->getComponent<MaterialAsset>();
					a_DrawCommand[i]->useProgram(l_pDirShadowMatInst->getProgram());

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

					a_DrawCommand[i]->bindVertex(l_pMeshIst->getVertexBuffer().get(), l_InstanceBuffer);
					a_DrawCommand[i]->bindIndex(l_pMeshIst->getIndexBuffer().get());
					l_pDirShadowMatInst->bindBlock(a_DrawCommand[i], STANDARD_TRANSFORM_NORMAL, getScene()->getRenderBatcher()->getWorldMatrixBlock());
					l_pDirShadowMatInst->bindBlock(a_DrawCommand[i], STANDARD_TRANSFORM_SKIN, getScene()->getRenderBatcher()->getSkinMatrixBlock());
					l_pDirShadowMatInst->bindBlock(a_DrawCommand[i], "m_Lights", getScene()->getDirLightContainer()->getMaterialBlock());
					l_pDirShadowMatInst->bindAll(a_DrawCommand[i]);
					a_DrawCommand[i]->drawElement(l_pSubMeshInst->m_StartIndex, l_pSubMeshInst->m_IndexCount, 0, l_InstanceData.size(), 0);
					getScene()->getRenderBatcher()->recycleInstanceVtxBuffer(l_InstanceBuffer);
				}

				// draw omni shadow map
				std::shared_ptr<Asset> l_pOmniShadowMat = l_pInst->getMaterial(MATSLOT_OMNI_SHADOWMAP);
				if( nullptr != l_pOmniShadowMat )
				{
					MaterialAsset *l_pOmniShadowMatInst = l_pOmniShadowMat->getComponent<MaterialAsset>();
					a_DrawCommand[i]->useProgram(l_pOmniShadowMatInst->getProgram());

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
						
						a_DrawCommand[i]->bindVertex(l_pMeshIst->getVertexBuffer().get(), l_InstanceBuffer);
						a_DrawCommand[i]->bindIndex(l_pMeshIst->getIndexBuffer().get());
						l_pOmniShadowMatInst->bindBlock(a_DrawCommand[i], STANDARD_TRANSFORM_NORMAL, getScene()->getRenderBatcher()->getWorldMatrixBlock());
						l_pOmniShadowMatInst->bindBlock(a_DrawCommand[i], STANDARD_TRANSFORM_SKIN, getScene()->getRenderBatcher()->getSkinMatrixBlock());
						l_pOmniShadowMatInst->bindBlock(a_DrawCommand[i], "m_Lights", getScene()->getOmniLightContainer()->getMaterialBlock());
						l_pOmniShadowMatInst->bindAll(a_DrawCommand[i]);

						a_DrawCommand[i]->drawElement(l_pSubMeshInst->m_StartIndex, l_pSubMeshInst->m_IndexCount, 0, l_InstanceData.size(), 0);
						getScene()->getRenderBatcher()->recycleInstanceVtxBuffer(l_InstanceBuffer);
					}
				}

				// draw spot shadow map
				std::shared_ptr<Asset> l_pSpotShadowMat = l_pInst->getMaterial(MATSLOT_SPOT_SHADOWMAP);
				if( nullptr != l_pSpotShadowMat )
				{
					MaterialAsset *l_pSpotShadowMatInst = l_pSpotShadowMat->getComponent<MaterialAsset>();
					a_DrawCommand[i]->useProgram(l_pSpotShadowMatInst->getProgram());

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
						
						a_DrawCommand[i]->bindVertex(l_pMeshIst->getVertexBuffer().get(), l_InstanceBuffer);
						a_DrawCommand[i]->bindIndex(l_pMeshIst->getIndexBuffer().get());
						l_pSpotShadowMatInst->bindBlock(a_DrawCommand[i], STANDARD_TRANSFORM_NORMAL, getScene()->getRenderBatcher()->getWorldMatrixBlock());
						l_pSpotShadowMatInst->bindBlock(a_DrawCommand[i], STANDARD_TRANSFORM_SKIN, getScene()->getRenderBatcher()->getSkinMatrixBlock());
						l_pSpotShadowMatInst->bindBlock(a_DrawCommand[i], "m_Lights", getScene()->getSpotLightContainer()->getMaterialBlock());
						l_pSpotShadowMatInst->bindAll(a_DrawCommand[i]);

						a_DrawCommand[i]->drawElement(l_pSubMeshInst->m_StartIndex, l_pSubMeshInst->m_IndexCount, 0, l_InstanceData.size(), 0);
						getScene()->getRenderBatcher()->recycleInstanceVtxBuffer(l_InstanceBuffer);
					}
				}
			}
				
			for( unsigned int j=0 ; j<a_StaticMesh.size() ; j+=l_NumCommand )
			{
				if( j+i >= a_StaticMesh.size() ) break;
					
				RenderableMesh *l_pInst = reinterpret_cast<RenderableMesh *>(a_StaticMesh[j+i].get());
				SceneBatcher::MeshCache *l_pBatchInfo = nullptr;
				if( getScene()->getRenderBatcher()->getBatchParam(l_pInst->getMesh(), &l_pBatchInfo) )
				{
					
				}
			}

			a_DrawCommand[i]->end();
		});
	}
	EngineCore::singleton().join();
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
#pragma endregion

}