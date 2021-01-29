// GridLine.cpp
//
// 2020/01/29 Ian Wu/Real0000
//

#include "REngine.h"

#include "REditor.h"
#include "GridLine.h"

namespace R
{

#pragma region GridLine
//
// GridLine
//
GridLine::GridLine(std::shared_ptr<Scene> a_pRefScene, std::shared_ptr<SceneNode> a_pNode)
	: EngineComponent(a_pRefScene, a_pNode)
{
}

GridLine::~GridLine()
{
	m_VertexBuffer = nullptr;
	m_IndexBuffer = nullptr;
}

void GridLine::setup(GridPlane a_Plane, float a_Length, float a_Unit)
{
	
}
#pragma endregion

}