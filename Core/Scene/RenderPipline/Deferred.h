// Deferred.h
//
// 2017/08/14 Ian Wu/Real0000
//

#ifndef __DEFERRED_H_
#define __DEFERRED_H_

#include "RenderPipline.h"

namespace R
{

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
	DeferredRenderer(SharedSceneMember *a_pSharedMember);
	virtual ~DeferredRenderer();

	virtual void render(std::shared_ptr<CameraComponent> a_pCamera, GraphicCanvas *a_pCanvas);

private:
	bool setupVisibleList(std::shared_ptr<CameraComponent> a_pCamera, std::vector< std::shared_ptr<RenderableComponent> > &a_Light, std::vector< std::shared_ptr<RenderableComponent> > &a_Mesh);
	void setupIndexUav(std::vector< std::shared_ptr<RenderableComponent> > &a_Light, std::vector< std::shared_ptr<RenderableComponent> > &a_Mesh);
	void setupShadow(std::shared_ptr<CameraComponent> a_pCamera, std::shared_ptr<Light> &a_Light, std::vector< std::shared_ptr<RenderableComponent> > &a_Mesh);
	void drawShadow(std::shared_ptr<Light> &a_Light, std::vector< std::shared_ptr<RenderableComponent> > &a_Mesh);

	unsigned int calculateShadowMapRegion(std::shared_ptr<CameraComponent> a_pCamera, std::shared_ptr<Light> &a_Light);
	void requestShadowMapRegion(unsigned int a_Size, std::shared_ptr<Light> &a_Light);

	void drawOpaqueMesh(std::shared_ptr<CameraComponent> a_pCamera, int a_DepthTexture, std::vector<int> &a_RenderTargets, std::vector< std::shared_ptr<RenderableComponent> > &a_Mesh, unsigned int &a_OpaqueEnd);
	void drawMesh(std::shared_ptr<CameraComponent> a_pCamera, int a_DepthTexture, std::vector<int> &a_RenderTargets, std::vector< std::shared_ptr<RenderableComponent> > &a_Mesh, unsigned int a_Start, unsigned int a_End);

	GraphicCommander *m_pCmdInit;
	std::shared_ptr<MaterialBlock> m_LightIdx, m_MeshIdx;
	unsigned int m_ExtendSize;//maybe add to engine setting?

	// shadow map variable
	RenderTextureAtlas *m_pShadowMap;
	std::shared_ptr<Asset> m_pShadowMapDepth;
	unsigned int m_ShadowCmdIdx;
	std::vector<GraphicCommander *> m_ShadowCommands;
	std::vector< std::shared_ptr<MaterialBlock> > m_ShadowMapIndirectBuffer;
	std::mutex m_ShadowMapLock;
	
	// gbuffer varibles
	glm::ivec2 m_TileDim;
	std::shared_ptr<Asset> m_pGBuffer[GBUFFER_COUNT];
	std::shared_ptr<Asset> m_pFrameBuffer;
	std::shared_ptr<Asset> m_pDepthMinmax;
	unsigned int m_MinmaxStepCount;
	std::shared_ptr<MaterialBlock> m_TiledValidLightIdx;
	std::vector<GraphicCommander *> m_DrawCommand;
	std::shared_ptr<Material> m_pLightIndexMat, m_pDeferredLightMat, m_pCopyMat;
	ThreadPool m_ThreadPool;

	std::vector<int> m_GBufferCache, m_FBufferCache;
};

}

#endif