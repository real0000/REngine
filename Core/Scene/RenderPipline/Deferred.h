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
class TextureUnit;
class Light;

class DeferredRenderer : public RenderPipeline
{
private:
	enum
	{
		GBUFFER_COLOR = 0,
		GBUFFER_NORMAL,

		GBUFFER_DEPTH,

		GBUFFER_COUNT, 
	};
public:
	DeferredRenderer(SharedSceneMember *a_pSharedMember);
	virtual ~DeferredRenderer();

	virtual void render(std::shared_ptr<CameraComponent> a_pCamera);

private:
	bool setupVisibleList(std::shared_ptr<CameraComponent> a_pCamera, std::vector< std::shared_ptr<RenderableComponent> > &a_Light, std::vector< std::shared_ptr<RenderableComponent> > &a_Mesh);
	void setupIndexUav(std::vector< std::shared_ptr<RenderableComponent> > &a_Light, std::vector< std::shared_ptr<RenderableComponent> > &a_Mesh);
	void setupShadow(std::shared_ptr<CameraComponent> a_pCamera, std::shared_ptr<Light> &a_Light, std::vector< std::shared_ptr<RenderableComponent> > &a_Mesh);
	void drawShadow(std::shared_ptr<Light> &a_Light, std::vector< std::shared_ptr<RenderableComponent> > &a_Mesh);

	unsigned int calculateShadowMapRegion(std::shared_ptr<CameraComponent> a_pCamera, std::shared_ptr<Light> &a_Light);
	void requestShadowMapRegion(unsigned int a_Size, std::shared_ptr<Light> &a_Light);

	void drawMesh(std::vector< std::shared_ptr<RenderableComponent> > &a_Mesh, unsigned int a_Start, unsigned int a_End);

	struct ObjectIndexBuffer
	{
		int m_UavId;
		char *m_pBuffer;
		int m_UavSize;
	};
	struct LightInfo
	{
		unsigned int m_Index;
		unsigned int m_Type;
	};
	struct MeshInfo
	{
		unsigned int m_Index;
	};
	GraphicCommander *m_pCmdInit;
	ObjectIndexBuffer m_LightIdxUav, m_MeshIdxUav;
	unsigned int m_ExtendSize;//maybe add to engine setting?

	// shadow map variable
	RenderTextureAtlas *m_pShadowMap;
	std::shared_ptr<TextureUnit> m_pShadowMapDepth;
	std::shared_ptr<TextureUnit> m_pGBuffer[GBUFFER_COUNT];
	unsigned int m_ShadowCmdIdx;
	std::vector<GraphicCommander *> m_ShadowCommands;
	std::vector<ObjectIndexBuffer> m_ShadowMapIndirectBuffer;
	std::mutex m_ShadowMapLock;
};

}

#endif