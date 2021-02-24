// ShadowMap.h
//
// 2020/11/16 Ian Wu/Real0000
//

#ifndef _SHADOW_MAP_H_
#define _SHADOW_MAP_H_

#include "RenderPipline.h"

namespace R
{

class IndirectDrawBuffer;
class Light;
class MaterialAsset;
class MaterialBlock;
class RenderableMesh;
class RenderTextureAtlas;

class ShadowMapRenderer : public RenderPipeline
{
public:
	static ShadowMapRenderer* create(boost::property_tree::ptree &a_Src, std::shared_ptr<Scene> a_pScene);
	virtual ~ShadowMapRenderer();

	static std::string typeName(){ return "ShadowMapRenderer"; }
	virtual void saveSetting(boost::property_tree::ptree &a_Dst);
	virtual void render(std::shared_ptr<Camera> a_pCamera, GraphicCanvas *a_pCanvas);
	virtual void canvasResize(glm::ivec2 a_Size){}
	virtual void drawFlagChanged(unsigned int a_Flag);

	void bake(std::vector<std::shared_ptr<RenderableComponent>> &a_Lights
			, std::vector<RenderableMesh*> &a_SortedDir
			, std::vector<RenderableMesh*> &a_SortedOmni
			, std::vector<RenderableMesh*> &a_SortedSpot
			, GraphicCommander *a_pMiscCmd
			, std::vector<GraphicCommander *> &a_DrawCommand);
	std::shared_ptr<Asset> getShadowMap();

private:
	ShadowMapRenderer(std::shared_ptr<Scene> a_pScene);
	
	unsigned int calculateShadowMapRegion(std::shared_ptr<Camera> a_pCamera, std::shared_ptr<Light> &a_Light);
	void requestShadowMapRegion(unsigned int a_Size, std::shared_ptr<Light> &a_Light);
	void drawLightShadow(GraphicCommander *a_pCmd
		, MaterialAsset *a_pMat, std::shared_ptr<MaterialBlock> a_pLightBlock
		, std::vector<glm::ivec4> &a_InstanceData, VertexBuffer *a_pVtxBuff, IndexBuffer *a_pIdxBuff
		, unsigned int a_IndirectData, int a_BuffID);
	
	RenderTextureAtlas *m_pShadowMap;
	std::shared_ptr<Asset> m_pClearMat;
	MaterialAsset *m_pClearMatInst;
	std::vector<GraphicCommander *> m_ShadowCommands;
	std::mutex m_ShadowMapLock;
};

}

#endif