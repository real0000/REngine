// RenderObject.cpp
//
// 2017/07/19 Ian Wu/Real0000
//
#include "CommonUtil.h"
#include "RGDeviceWrapper.h"

#include "Core.h"
#include "RenderObject.h"
#include "Material.h"

#include "Scene/Scene.h"
#include "Scene/Graph/ScenePartition.h"

namespace R
{

#pragma region RenderableComponent
//
// RenderableComponent
//
RenderableComponent::RenderableComponent(SharedSceneMember *a_pSharedMember, std::shared_ptr<SceneNode> a_pOwner)
	: EngineComponent(a_pSharedMember, a_pOwner)
{
	addTransformListener();
}

RenderableComponent::~RenderableComponent()
{
}
#pragma endregion

}