// PhysicalModule.h
//
// 2014/06/16 Ian Wu/Real0000
//

#ifndef _PHYSICAL_MODULE_H_
#define _PHYSICAL_MODULE_H_

#include "BulletCollision/CollisionDispatch/btCollisionDispatcherMt.h"
#include "BulletCollision/CollisionDispatch/btGhostObject.h"
#include "BulletDynamics/Dynamics/btDiscreteDynamicsWorldMt.h"
#include "BulletDynamics/Featherstone/btMultiBodyConstraintSolver.h"
#include "btBulletDynamicsCommon.h"

namespace R
{

class EngineComponent;
class PhysicalModule;
class PhysicalWorld;

class PhysicalListener
{
	friend class PhysicalWorld;
public:
	enum Type
	{
		TRIGGER = 0,
		COLLISION,
	};
	enum Event
	{
		ENTER = 0,
		LEAVE,

		COUNT
	};
	typedef std::function<void(std::shared_ptr<EngineComponent>, Type)> PhysicalFunc;

public:
	void setEvent(Event a_Type, PhysicalFunc a_pFunc){ m_pFunc[a_Type] = a_pFunc; }
	PhysicalFunc getEvent(Event a_Type){ return m_pFunc[a_Type]; }

	std::shared_ptr<EngineComponent> getOwner(){ return m_pOwner; }

private:
	PhysicalListener(std::shared_ptr<EngineComponent> a_pOwner);
	virtual ~PhysicalListener();

	std::shared_ptr<EngineComponent> m_pOwner;
	PhysicalFunc m_pFunc[COUNT];
};

class PhysicalWorld
{
	friend class PhysicalModule;
public:
	void setGravity(glm::vec3 a_Dir);

	void update(float a_Delta);
	int createTrigger(PhysicalListener *a_pListener, glm::sphere &a_Sphere, int a_GroupMask, int a_CollideMask);
	int createTrigger(PhysicalListener *a_pListener, glm::obb &a_Box, int a_GroupMask, int a_CollideMask);
	int createTrigger(PhysicalListener *a_pListener, float a_Radius, float a_Height, glm::mat4x4 &a_Tansform, int a_GroupMask, int a_CollideMask);// cone
	void updateTrigger(int a_ID, glm::mat4x4 &a_Tansform);
	void removeTrigger(int a_ID);

	PhysicalListener* createListener(std::shared_ptr<EngineComponent> a_pOwner);
	PhysicalListener* getListener(int a_ID);

private:
	PhysicalWorld(btDispatcher *a_pDispatcher
				, btBroadphaseInterface *a_pBroadPhase
				, btConstraintSolverPoolMt *a_pSolverPool
				, btConstraintSolver *a_pConstraintSolverMt
				, btCollisionConfiguration *a_pCollisionConfiguration);
	virtual ~PhysicalWorld();

	int createTrigger(PhysicalListener *a_pListener, btCollisionShape *a_pShape, glm::mat4x4 &a_Transform, int a_GroupMask, int a_CollideMask);
	inline btTransform convertTransform(glm::mat4x4 &a_Src);

	struct Triggers
	{
		Triggers();
		~Triggers();

		btCollisionShape *m_pShape;
		btCollisionObject *m_pObj;
	};
	SerializedObjectPool<Triggers> m_TriggersPool;
	btDiscreteDynamicsWorld *m_pWorld;
};

class PhysicalModule
{
public:
	static PhysicalModule& singleton(){ static PhysicalModule l_Inst; return l_Inst; }

	void init();
	void update(float a_Delta);
	void uini();

	PhysicalWorld* createWorld();
	void removeWorld(PhysicalWorld *a_pWorld);

private:
	PhysicalModule();
	virtual ~PhysicalModule();

	btDefaultCollisionConfiguration *m_pCollisionConfiguration;
	btCollisionDispatcherMt *m_pDispatcher;
	btDbvtBroadphase *m_pBroadphase;
	btConstraintSolver *m_pSolver;

	btITaskScheduler *m_pDefScheduler;
	std::set<PhysicalWorld*> m_Worlds;
};

}

#endif