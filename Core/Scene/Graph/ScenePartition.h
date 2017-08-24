// ScenePartition.h
//
// 2017/08/14 Ian Wu/Real0000
//

#ifndef _SCENE_PARTITION_H_
#define _SCENE_PARTITION_H_

namespace R
{

class CameraComponent;
class RenderableComponent;

STRING_ENUM_CLASS(ScenePartitionType,
	None,
	Octree)

class ScenePartition
{
public:
	ScenePartition(){}
	virtual ~ScenePartition(){}
	
	virtual void add(std::shared_ptr<RenderableComponent> a_pComponent) = 0;
	virtual void update(std::shared_ptr<RenderableComponent> a_pComponent) = 0;
	virtual void remove(std::shared_ptr<RenderableComponent> a_pComponent) = 0;
	virtual void clear() = 0;
	
	virtual void getVisibleList(std::shared_ptr<CameraComponent> a_pTargetCamera, std::vector< std::shared_ptr<RenderableComponent> > &a_Output) = 0;
};

}

#endif