// ShadowMap.h
//
// 2020/11/16 Ian Wu/Real0000
//

#ifndef _SHADOW_MAP_H_
#define _SHADOW_MAP_H_

#include "RenderPipline.h"

namespace R
{

class Light;
class RenderTextureAtlas;

class ShadowMapRenderer : public RenderPipeline
{
public:
	static ShadowMapRenderer* create(boost::property_tree::ptree &a_Src, std::shared_ptr<Scene> a_pScene);
	virtual ~ShadowMapRenderer();

	static std::string typeName(){ return "ShadowMapRenderer"; }
	virtual void saveSetting(boost::property_tree::ptree &a_Dst);
	virtual void render(std::shared_ptr<Camera> a_pCamera, GraphicCanvas *a_pCanvas);

	void bake(std::vector<std::shared_ptr<RenderableComponent>> &a_Lights,
				std::vector<std::shared_ptr<RenderableComponent>> &a_StaticMesh,
				std::vector<std::shared_ptr<RenderableComponent>> &a_Mesh,
				GraphicCommander *a_pMiscCmd,
				std::vector<GraphicCommander *> &a_DrawCommand);

private:
	ShadowMapRenderer(std::shared_ptr<Scene> a_pScene);
	
	unsigned int calculateShadowMapRegion(std::shared_ptr<Camera> a_pCamera, std::shared_ptr<Light> &a_Light);
	void requestShadowMapRegion(unsigned int a_Size, std::shared_ptr<Light> &a_Light);
	
	RenderTextureAtlas *m_pShadowMap;
	std::vector<GraphicCommander *> m_ShadowCommands;
	std::mutex m_ShadowMapLock;
};

}

#endif