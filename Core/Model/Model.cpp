// Model.cpp
//
// 2017/07/19 Ian Wu/Real0000
//
#include "CommonUtil.h"
#include "RGDeviceWrapper.h"

#include "Core.h"
#include "Model.h"
#include "Material.h"

#include "Scene/Scene.h"

namespace R
{

#pragma region ModelComponent
//
// ModelComponent
//
ModelComponent::ModelComponent(std::shared_ptr<SceneNode> a_pOwner)
	: EngineComponent(a_pOwner)
{
}

ModelComponent::~ModelComponent()
{
}
#pragma endregion

}