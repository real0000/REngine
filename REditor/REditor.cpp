// REditor.cpp
//
// 2020/12/21 Ian Wu/Real0000
//

#include "REngine.h"
#include "REditor.h"

namespace R
{

void registEditorComponents()
{
	EngineCore::singleton().registComponentReflector<CameraController>();
}

}
