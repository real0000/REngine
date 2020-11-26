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
class MaterialAsset;
class RenderTextureAtlas;
class TextureAsset;
class Light;

class DeferredRenderer : public RenderPipeline
{
	friend class DeferredRenderer;
private:
	enum
	{
		GBUFFER_NORMAL = 0,
		GBUFFER_MATERIAL,
		GBUFFER_BASECOLOR,
		GBUFFER_MASK,
		GBUFFER_FACTOR,
		GBUFFER_MOTIONBLUR,

		GBUFFER_DEPTH,

		GBUFFER_COUNT, 
	};
public:
	static DeferredRenderer* create(boost::property_tree::ptree &a_Src, std::shared_ptr<Scene> a_pScene);
	virtual ~DeferredRenderer();

	static std::string typeName(){ return "DeferredRenderer"; }
	virtual void saveSetting(boost::property_tree::ptree &a_Dst);
	virtual void render(std::shared_ptr<Camera> a_pCamera, GraphicCanvas *a_pCanvas);

private:
	DeferredRenderer(std::shared_ptr<Scene> a_pScene);

	bool setupVisibleList(std::shared_ptr<Camera> a_pCamera
		, std::vector<std::shared_ptr<RenderableComponent>> &a_Light
		, std::vector<std::shared_ptr<RenderableComponent>> &a_StaticMesh, std::vector<std::shared_ptr<RenderableComponent>> &a_Mesh);
	void setupIndexUav(std::vector< std::shared_ptr<RenderableComponent> > &a_Light);

	GraphicCommander *m_pCmdInit;
	std::shared_ptr<MaterialBlock> m_LightIdx;
	unsigned int m_ExtendSize;//maybe add to engine setting?
	
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

	std::vector<int> m_GBufferCache, m_FBufferCache;
};

}

#endif