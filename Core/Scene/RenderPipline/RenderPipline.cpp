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
	, m_DrawFlag(0)
{
}

RenderPipeline::~RenderPipeline()
{
	m_pRefScene = nullptr;
}

void RenderPipeline::setDrawFlag(unsigned int a_Flag)
{
	drawFlagChanged(a_Flag);
	m_DrawFlag = a_Flag;
}
#pragma endregion

}