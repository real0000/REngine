// Deferred.h
//
// 2017/08/14 Ian Wu/Real0000
//

#ifndef __DEFERRED_H_
#define __DEFERRED_H_

#include "RenderPipline.h"

namespace R
{

class Asset;
class RenderTextureAtlas;
class TextureAsset;
class Light;

class DeferredRenderer : public RenderPipeline
{
private:
	enum
	{
		GBUFFER_NORMAL = 0,
		GBUFFER_MATERIAL,
		GBUFFER_DIFFUSE,
		GBUFFER_MASK,
		GBUFFER_FACTOR,
		GBUFFER_MOTIONBLUR,

		GBUFFER_DEPTH,

		GBUFFER_COUNT, 
	};
public:
	DeferredRenderer(std::shared_ptr<Scene> a_pScene);
	virtual ~DeferredRenderer();

	virtual void render(std::shared_ptr<Camera> a_pCamera, GraphicCanvas *a_pCanvas);

private:
	bool setupVisibleList(std::shared_ptr<Camera> a_pCamera
		, std::vector<std::shared_ptr<RenderableComponent>> &a_StaticLight, std::vector<std::shared_ptr<RenderableComponent>> &a_Light
		, std::vector<std::shared_ptr<RenderableComponent>> &a_StaticMesh, std::vector<std::shared_ptr<RenderableComponent>> &a_Mesh);
	void setupIndexUav(std::vector< std::shared_ptr<RenderableComponent> > &a_Light, std::vector< std::shared_ptr<RenderableComponent> > &a_Mesh);

	unsigned int calculateShadowMapRegion(std::shared_ptr<Camera> a_pCamera, std::shared_ptr<Light> &a_Light);
	void requestShadowMapRegion(unsigned int a_Size, std::shared_ptr<Light> &a_Light);

	void drawOpaqueMesh(std::shared_ptr<Camera> a_pCamera, int a_DepthTexture, std::vector<int> &a_RenderTargets, std::vector< std::shared_ptr<RenderableComponent> > &a_Mesh, unsigned int &a_OpaqueEnd);
	void drawMesh(std::shared_ptr<Camera> a_pCamera, int a_DepthTexture, std::vector<int> &a_RenderTargets, std::vector< std::shared_ptr<RenderableComponent> > &a_Mesh, unsigned int a_Start, unsigned int a_End);

	GraphicCommander *m_pCmdInit;
	std::shared_ptr<MaterialBlock> m_LightIdx;
	unsigned int m_ExtendSize;//maybe add to engine setting?

	// shadow map variable
	RenderTextureAtlas *m_pShadowMap;
	std::vector<GraphicCommander *> m_ShadowCommands;
	std::shared_ptr<Asset> m_pDirShadowMat, m_pOmniShadowMat, m_pSpotShadowMat;
	std::mutex m_ShadowMapLock;
	
	// gbuffer varibles
	glm::ivec2 m_TileDim;
	std::shared_ptr<Asset> m_pGBuffer[GBUFFER_COUNT];
	std::shared_ptr<Asset> m_pFrameBuffer;
	std::shared_ptr<Asset> m_pDepthMinmax;
	unsigned int m_MinmaxStepCount;
	std::shared_ptr<MaterialBlock> m_TiledValidLightIdx;
	std::vector<GraphicCommander *> m_DrawCommand;
	std::shared_ptr<Asset> m_pLightIndexMat, m_pDeferredLightMat, m_pCopyMat;
	MaterialAsset *m_pLightIndexMatInst, *m_pDeferredLightMatInst, *m_pCopyMatInst;
	ThreadPool m_ThreadPool;

	std::vector<int> m_GBufferCache, m_FBufferCache;
};

}

#endif