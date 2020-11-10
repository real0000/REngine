// RenderObject.cpp
//
// 2017/07/19 Ian Wu/Real0000
//
#include "CommonUtil.h"
#include "RGDeviceWrapper.h"
#include "RImporters.h"

#include "Core.h"
#include "RenderObject.h"
#include "Mesh.h"

#include "Scene/Scene.h"
#include "Scene/Graph/ScenePartition.h"

namespace R
{

#pragma region RenderableComponent
//
// RenderableComponent
//
RenderableComponent::RenderableComponent(std::shared_ptr<Scene> a_pRefScene, std::shared_ptr<SceneNode> a_pOwner)
	: EngineComponent(a_pRefScene, a_pOwner)
{
}

RenderableComponent::~RenderableComponent()
{
}
#pragma endregion

}