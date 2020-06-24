// REngine.h
//
// 2014/03/12 Ian Wu/Real0000
//

#ifndef _RENGINE_H_
#define _RENGINE_H_

//#include "PxPhysicsAPI.h"

#include "CommonUtil.h"
#include "RImporters.h"
#include "RGDeviceWrapper.h"

#include "Core.h"

#include "PhysicalModule.h"
#include "StateMachine.h"

#include "Asset/AssetBase.h"
#include "Asset/MaterialAsset.h"
#include "Asset/MeshAsset.h"
#include "Asset/SceneAsset.h"
#include "Asset/TextureAsset.h"

#include "Input/InputMediator.h"
#include "Input/KeyboardInput.h"
#include "Input/MouseInput.h"

#include "RenderObject/Mesh.h"
#include "RenderObject/RenderObject.h"

#include "Scene/Camera.h"
#include "Scene/Scene.h"
#include "Scene/Graph/Octree.h"
#include "Scene/Graph/ScenePartition.h"
#include "Scene/RenderPipline/Deferred.h"
#include "Scene/RenderPipline/RenderPipline.h"

#endif