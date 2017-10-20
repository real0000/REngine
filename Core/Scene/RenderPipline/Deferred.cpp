// Deferred.cpp
//
// 2017/08/14 Ian Wu/Real0000
//

#include "CommonUtil.h"
#include "Core.h"
#include "Scene/Scene.h"
#include "Scene/Graph/ScenePartition.h"

#include "Deferred.h"

namespace R
{

#pragma region DeferredRenderer
//
// DeferredRenderer
//
DeferredRenderer::DeferredRenderer(SharedSceneMember *a_pSharedMember)
	: RenderPipeline(a_pSharedMember)
{
}

DeferredRenderer::~DeferredRenderer()
{
}

void DeferredRenderer::render(std::shared_ptr<CameraComponent> a_pCamera)
{
	std::vector< std::shared_ptr<RenderableComponent> > l_Lights, l_Meshes;
	getSharedMember()->m_pGraphs[SharedSceneMember::GRAPH_MESH]->getVisibleList(a_pCamera, l_Meshes);
}
#pragma endregion

}