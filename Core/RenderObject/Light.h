// Light.h
//
// 2017/09/19 Ian Wu/Real0000
//

#ifndef _LIGHT_H_
#define _LIGHT_H_

#include "RenderObject.h"

namespace R
{

class SceneNode;

class Light : public RenderableComponent
{
	friend class EngineComponent;
public:
	Light(SharedSceneMember *a_pSharedMember, std::shared_ptr<SceneNode> a_pOwner);
	virtual ~Light();
	
	virtual void start();
	virtual void end();
	virtual void staticFlagChanged();
	virtual void transformListener(glm::mat4x4 &a_NewTransform);
	
	virtual bool isHidden(){ return m_bHidden; }
	void setHidden(bool a_bHidden);

private:
	bool m_bHidden;
};

}

#endif