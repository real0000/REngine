// IntersectHelper.h
//
// 2021/04/26 Ian Wu/Real0000
//

#ifndef _INTERSECT_HELPER_H_
#define _INTERSECT_HELPER_H_

namespace R
{

class Camera;
class EngineComponent;
class Light;
class PhysicalListener;
class PhysicalWorld;
class RenderableMesh;

class IntersectHelper
{
public:
	// use TRIGGER_* for a_RecordFlag
	IntersectHelper(std::shared_ptr<EngineComponent> a_pOwner, std::function<int(PhysicalListener*)> a_pCreateFunc, std::function<glm::mat4x4()> a_pUpdateFunc, unsigned int a_RecordFlag);
	virtual ~IntersectHelper();

	void updateTrigger();
	void setupTrigger(bool a_bCreate);
	void removeTrigger();
	const std::set<std::shared_ptr<RenderableMesh>>& getMeshes(){ return m_Meshes; }
	const std::set<std::shared_ptr<Light>>& getLights(){ return m_Lights; }
	const std::set<std::shared_ptr<Camera>>& getCameras(){ return m_Cameras; }

private:
	int m_TriggerID;
	std::shared_ptr<EngineComponent> m_pOwner;
	PhysicalWorld *m_pRefWorld;
	std::function<int(PhysicalListener*)> m_pCreateFunc;
	std::function<glm::mat4x4()> m_pUpdateFunc;
	std::set<std::shared_ptr<RenderableMesh>> m_Meshes;
	std::set<std::shared_ptr<Light>> m_Lights;
	std::set<std::shared_ptr<Camera>> m_Cameras;
	unsigned int m_RecordFlag;
};

}

#endif