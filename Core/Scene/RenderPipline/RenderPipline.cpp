// RenderPipline.cpp
//
// 2017/08/14 Ian Wu/Real0000
//

#include "CommonUtil.h"
#include "Core.h"
#include "RenderPipline.h"

#include "Scene/Scene.h"

namespace R
{

#pragma region RenderPipeline
//
// RenderPipeline
//
RenderPipeline::RenderPipeline(std::shared_ptr<Scene> a_pScene)
	: m_pRefScene(a_pScene)
{
}

RenderPipeline::~RenderPipeline()
{
	m_pRefScene = nullptr;
}
#pragma endregion

}