// PhysicalModule.cpp
//
// 2014/06/16 Ian Wu/Real0000
//

#include "CommonUtil.h"
#include "PhysicalModule.h"

namespace R
{

#pragma region PhysicalModule
//
// PhysicalModule
//
PhysicalModule::PhysicalModule()
	: m_pCollisionConfiguration(nullptr)
{
}

PhysicalModule::~PhysicalModule()
{
}

void PhysicalModule::init()
{
	m_pCollisionConfiguration = new btDefaultCollisionConfiguration();
}

void PhysicalModule::uini()
{
	SAFE_DELETE(m_pCollisionConfiguration)
}
#pragma endregion

}