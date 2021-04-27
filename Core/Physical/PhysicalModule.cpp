// PhysicalModule.cpp
//
// 2014/06/16 Ian Wu/Real0000
//

#include "CommonUtil.h"
#include "PhysicalModule.h"


namespace R
{

class DefaultFilter : public btOverlapFilterCallback
{
public:
	virtual bool needBroadphaseCollision(btBroadphaseProxy *a_pProxy0, btBroadphaseProxy *a_pProxy1) const
	{
		return 0 != (a_pProxy0->m_collisionFilterGroup & a_pProxy1->m_collisionFilterMask)
			&& 0 != (a_pProxy1->m_collisionFilterGroup & a_pProxy0->m_collisionFilterMask);
	}
};
static DefaultFilter g_Filter;

class DefaultCallback : public btOverlappingPairCallback
{
public:
	DefaultCallback()
		: m_NumOverlapping(0)
	{
	}
	
	virtual ~DefaultCallback()
	{
	}

	virtual btBroadphasePair* addOverlappingPair(btBroadphaseProxy *a_pProxy0, btBroadphaseProxy *a_pProxy1)
	{
		processOverlapping(a_pProxy0, a_pProxy1, PhysicalListener::ENTER);
		return nullptr;
	}

	virtual void* removeOverlappingPair(btBroadphaseProxy *a_pProxy0, btBroadphaseProxy *a_pProxy1, btDispatcher *a_pDispatcher)
	{
		processOverlapping(a_pProxy0, a_pProxy1, PhysicalListener::LEAVE);
		return nullptr;
	}

	virtual void removeOverlappingPairsContainingProxy(btBroadphaseProxy *a_pProxy0, btDispatcher *a_pDispatcher)
	{
	}

private:
	void processOverlapping(btBroadphaseProxy *a_pProxy0, btBroadphaseProxy *a_pProxy1, PhysicalListener::Event a_Type)
	{
		btCollisionObject *l_pObj0Base = reinterpret_cast<btCollisionObject*>(a_pProxy0->m_clientObject);
		btCollisionObject *l_pObj1Base = reinterpret_cast<btCollisionObject*>(a_pProxy1->m_clientObject);
		PhysicalListener::Type l_FuncType = (btCollisionObject::CO_GHOST_OBJECT == l_pObj0Base->getInternalType() ||
											btCollisionObject:: btCollisionObject::CO_GHOST_OBJECT) ? PhysicalListener::TRIGGER : PhysicalListener::COLLISION;
		PhysicalListener *l_pListener0 = reinterpret_cast<PhysicalListener*>(l_pObj0Base->getUserPointer());
		PhysicalListener *l_pListener1 = reinterpret_cast<PhysicalListener*>(l_pObj1Base->getUserPointer());
		if( nullptr == l_pListener0 || nullptr == l_pListener1 ) return ;

		auto l_pFunc = l_pListener0->getEvent(a_Type);
		if( nullptr != l_pFunc ) l_pFunc(l_pListener1->getOwner(), l_FuncType);
		l_pFunc = l_pListener1->getEvent(a_Type);
		if( nullptr != l_pFunc ) l_pFunc(l_pListener0->getOwner(), l_FuncType);
	}

	unsigned int m_NumOverlapping;
};
static DefaultCallback g_Callback;

#pragma region PhysicalListener
//
// PhysicalListener
//
PhysicalListener::PhysicalListener(std::shared_ptr<EngineComponent> a_pOwner)
	: m_pOwner(a_pOwner)
	, m_pFunc{nullptr, nullptr}
{
	assert(nullptr != m_pOwner);
}

PhysicalListener::~PhysicalListener()
{
	m_pOwner = nullptr;
}
#pragma endregion

#pragma region PhysicalWorld
#pragma region PhysicalWorld::Triggers
//
// PhysicalWorld::Triggers
//
PhysicalWorld::Triggers::Triggers()
	: m_pShape(nullptr)
	, m_pObj(nullptr)
{
}

PhysicalWorld::Triggers::~Triggers()
{
	PhysicalListener *l_pListenerBase = reinterpret_cast<PhysicalListener *>(m_pObj->getUserPointer());
	SAFE_DELETE(l_pListenerBase)
	m_pObj->setUserPointer(nullptr);

	SAFE_DELETE(m_pObj)
	SAFE_DELETE(m_pShape)
}
#pragma endregion
//
// PhysicalWorld
//
PhysicalWorld::PhysicalWorld(btDispatcher *a_pDispatcher
							, btBroadphaseInterface *a_pBroadPhase
							, btConstraintSolverPoolMt *a_pSolverPool
							, btConstraintSolver *a_pConstraintSolverMt
							, btCollisionConfiguration *a_pCollisionConfiguration)
	: m_TriggersPool()
{
	m_TriggersPool.setNewFunc(std::bind(&defaultNewFunc<Triggers>));
	m_TriggersPool.setBeforeDelFunc([=](std::shared_ptr<Triggers> a_pTargetPtr) -> void
	{
		m_pWorld->removeCollisionObject(a_pTargetPtr->m_pObj);
	});

	m_pWorld = new btDiscreteDynamicsWorldMt(a_pDispatcher, a_pBroadPhase, a_pSolverPool, a_pConstraintSolverMt, a_pCollisionConfiguration);
	m_pWorld->setGravity(btVector3(0.0f, -9.8f, 0.0f));
}

PhysicalWorld::~PhysicalWorld()
{
	m_TriggersPool.clear();
	SAFE_DELETE(m_pWorld)
}

void PhysicalWorld::setGravity(glm::vec3 a_Dir)
{
	m_pWorld->setGravity(btVector3(a_Dir.x, a_Dir.y, a_Dir.z));
}

void PhysicalWorld::update(float a_Delta)
{
	m_pWorld->stepSimulation(a_Delta);
}

int PhysicalWorld::createTrigger(PhysicalListener *a_pListener, glm::sphere &a_Sphere, int a_GroupMask, int a_CollideMask)
{
	btSphereShape *l_pShape = new btSphereShape(a_Sphere.m_Range);
	glm::mat4x4 l_Trans(glm::identity<glm::mat4x4>());
	l_Trans[3][0] = a_Sphere.m_Center.x;
	l_Trans[3][1] = a_Sphere.m_Center.y;
	l_Trans[3][2] = a_Sphere.m_Center.z;
	return createTrigger(a_pListener, l_pShape, l_Trans, a_GroupMask, a_CollideMask);
}

int PhysicalWorld::createTrigger(PhysicalListener *a_pListener, glm::obb &a_Box, int a_GroupMask, int a_CollideMask)
{
	glm::vec3 l_Half(a_Box.m_Size * 0.5f);
	btBoxShape *l_pShape = new btBoxShape(btVector3(l_Half.x, l_Half.y, l_Half.z));
	return createTrigger(a_pListener, l_pShape, a_Box.m_Transition, a_GroupMask, a_CollideMask);
}

int PhysicalWorld::createTrigger(PhysicalListener *a_pListener, float a_Radius, float a_Height, glm::mat4x4 &a_Tansform, int a_GroupMask, int a_CollideMask)
{
	btConeShape *l_pShape = new btConeShape(a_Radius, a_Height);
	return createTrigger(a_pListener, l_pShape, a_Tansform, a_GroupMask, a_CollideMask);
}

void PhysicalWorld::updateTrigger(int a_ID, glm::mat4x4 &a_Tansform)
{
	m_TriggersPool[a_ID]->m_pObj->setWorldTransform(convertTransform(a_Tansform));
}

void PhysicalWorld::removeTrigger(int a_ID)
{
	if( a_ID < 0 ) return;
	m_TriggersPool.release(a_ID);
}

PhysicalListener* PhysicalWorld::createListener(std::shared_ptr<EngineComponent> a_pOwner)
{
	return new PhysicalListener(a_pOwner);
}

PhysicalListener* PhysicalWorld::getListener(int a_ID)
{
	assert(nullptr != m_TriggersPool[a_ID]->m_pObj->getUserPointer());
	return reinterpret_cast<PhysicalListener*>(m_TriggersPool[a_ID]->m_pObj->getUserPointer());
}

int PhysicalWorld::createTrigger(PhysicalListener *a_pListener, btCollisionShape *a_pShape, glm::mat4x4 &a_Transform, int a_GroupMask, int a_CollideMask)
{
	std::shared_ptr<Triggers> l_pObj = nullptr;
	int l_Res = m_TriggersPool.retain(&l_pObj);

	l_pObj->m_pShape = a_pShape;

	btCollisionObject *l_pCollider = new btGhostObject();
	l_pCollider->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | btCollisionObject::CF_KINEMATIC_OBJECT);
	l_pCollider->setCollisionShape(l_pObj->m_pShape);
	l_pCollider->setUserPointer(a_pListener);
	l_pObj->m_pObj = l_pCollider;

	l_pCollider->setWorldTransform(convertTransform(a_Transform));
	
	m_pWorld->addCollisionObject(l_pCollider, a_GroupMask, a_CollideMask);

	return l_Res;
}

btTransform PhysicalWorld::convertTransform(glm::mat4x4 &a_Src)
{
	btTransform l_Trans;
	btMatrix3x3 &l_Basis = l_Trans.getBasis();
	for( unsigned int i=0 ; i<3 ; ++i )
	{
		for( unsigned int j=0 ; j<3 ; ++j ) l_Basis[i][j] = a_Src[i][j];
	}
	l_Trans.setOrigin(btVector3(a_Src[3][0], a_Src[3][1], a_Src[3][2]));
	return l_Trans;
}
#pragma endregion

#pragma region PhysicalModule

//
// PhysicalModule
//
PhysicalModule::PhysicalModule()
	: m_pCollisionConfiguration(nullptr)
	, m_pDispatcher(nullptr)
	, m_pBroadphase(nullptr)
	, m_pSolver(nullptr)
	, m_pDefScheduler(nullptr)
{
	m_pDefScheduler = btCreateDefaultTaskScheduler();
	m_pDefScheduler->setNumThreads(m_pDefScheduler->getMaxNumThreads());
	btSetTaskScheduler(m_pDefScheduler);
}

PhysicalModule::~PhysicalModule()
{
	SAFE_DELETE(m_pDefScheduler)
}

void PhysicalModule::init()
{
	btDefaultCollisionConstructionInfo l_DefCCInfo;
	l_DefCCInfo.m_defaultMaxPersistentManifoldPoolSize = 80000;
	l_DefCCInfo.m_defaultMaxCollisionAlgorithmPoolSize = 80000;
	m_pCollisionConfiguration = new btDefaultCollisionConfiguration(l_DefCCInfo);
	
	m_pDispatcher = new btCollisionDispatcherMt(m_pCollisionConfiguration, 40);
	m_pBroadphase = new btDbvtBroadphase();
	m_pBroadphase->getOverlappingPairCache()->setOverlapFilterCallback(&g_Filter);
	m_pBroadphase->getOverlappingPairCache()->setInternalGhostPairCallback(&g_Callback);

	btConstraintSolver *l_pSolverUnits[BT_MAX_THREAD_COUNT];
	for( unsigned int i=0 ; i<BT_MAX_THREAD_COUNT ; ++i ) l_pSolverUnits[i] = new btSequentialImpulseConstraintSolver();
	m_pSolver = new btConstraintSolverPoolMt(l_pSolverUnits, BT_MAX_THREAD_COUNT);
}

void PhysicalModule::update(float a_Delta)
{
	for( auto it=m_Worlds.begin() ; it!=m_Worlds.end() ; ++it ) (*it)->update(a_Delta);
}

void PhysicalModule::uini()
{
	for( auto it=m_Worlds.begin() ; it!=m_Worlds.end() ; ++it ) delete *it;
	m_Worlds.clear();

	SAFE_DELETE(m_pSolver)
	SAFE_DELETE(m_pBroadphase)
	SAFE_DELETE(m_pDispatcher)
	SAFE_DELETE(m_pCollisionConfiguration)
}

PhysicalWorld* PhysicalModule::createWorld()
{
	PhysicalWorld *l_pNewWorld = new PhysicalWorld(m_pDispatcher, m_pBroadphase, reinterpret_cast<btConstraintSolverPoolMt*>(m_pSolver), nullptr, m_pCollisionConfiguration);
	m_Worlds.insert(l_pNewWorld);
	return l_pNewWorld;
}

void PhysicalModule::removeWorld(PhysicalWorld *a_pWorld)
{
	m_Worlds.erase(a_pWorld);
	SAFE_DELETE(a_pWorld)
}
#pragma endregion

}