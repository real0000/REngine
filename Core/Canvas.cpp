// Canvas.cpp
//
// 2017/07/20 Ian Wu/Real0000
//

#include "RGDeviceWrapper.h"
#include "Core.h"
#include "Canvas.h"

namespace R
{

#pragma region EngineCanvas
//
// EngineCanvas
//
EngineCanvas::EngineCanvas(wxWindow *a_pParent)
	: GraphicCanvas(a_pParent, wxID_ANY)
{
	setResizeCallback(std::bind(&EngineCanvas::resize, this, std::placeholders::_1));
}

EngineCanvas::~EngineCanvas()
{
}

void EngineCanvas::processInput()
{

}

void EngineCanvas::resize(glm::ivec2 a_NewSize)
{
}
#pragma endregion

}