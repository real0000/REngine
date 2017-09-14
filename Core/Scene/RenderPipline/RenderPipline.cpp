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
RenderPipeline::RenderPipeline(SharedSceneMember *a_pSharedMember)
	: m_pSharedMember(new SharedSceneMember())
{
	*m_pSharedMember = *a_pSharedMember;
}

RenderPipeline::~RenderPipeline()
{
	SAFE_DELETE(m_pSharedMember)
}
#pragma endregion

}