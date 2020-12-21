// REditor.h
//
// 2020/12/21 Ian Wu/Real0000
//

#ifndef _REDITOR_H_
#define _REDITOR_H_

#include "Core.h"

namespace R
{

enum EditorComponent
{
	COMPONENT_CameraController = CUSTOM_COMPONENT,

	COMPONENT_EDITOR_END,
};

void registEditorComponents();

}

#include "Components/CameraController.h"

#endif