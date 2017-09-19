// Deferred.cpp
//
// 2017/08/14 Ian Wu/Real0000
//

#include "CommonUtil.h"
#include "Core.h"

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
	
void DeferredRenderer::add(std::shared_ptr<CameraComponent> a_pCamera)
{
	
}

void DeferredRenderer::remove(std::shared_ptr<CameraComponent> a_pCamera)
{
}

void DeferredRenderer::clear()
{
}

void DeferredRenderer::buildStaticCommand()
{
}

void DeferredRenderer::render()
{
}
#pragma endregion

}