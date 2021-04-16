// PhysicalModule.h
//
// 2014/06/16 Ian Wu/Real0000
//

#ifndef _PHYSICAL_MODULE_H_
#define _PHYSICAL_MODULE_H_

#include "btBulletDynamicsCommon.h"

namespace R
{

class SceneNode;

class TriggerListener
{
public:
	virtual void onTriggerEnter(std::shared_ptr<SceneNode> a_pOther){}
	virtual void onTriggerStay(std::shared_ptr<SceneNode> a_pOther){}
	virtual void onTriggerLeave(std::shared_ptr<SceneNode> a_pOther){}
};

class PhysicalModule
{
public:
	static PhysicalModule& singleton(){ static PhysicalModule l_Inst; return l_Inst; }

	void init();
	void uini();

private:
	PhysicalModule();
	virtual ~PhysicalModule();

	btDefaultCollisionConfiguration* m_pCollisionConfiguration;
};

}

#endif