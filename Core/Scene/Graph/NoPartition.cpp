// NoPartition.cpp
//
// 2017/08/24 Ian Wu/Real0000
//


#include "CommonUtil.h"
#include "Core.h"

#include "NoPartition.h"

namespace R
{


#pragma region NoPartition
//
// NoPartition
//
NoPartition::NoPartition()
{
}

NoPartition::~NoPartition()
{
	clear();
}

void NoPartition::add(std::shared_ptr<RenderableComponent> a_pComponent)
{
	m_Container.insert(a_pComponent);
}

void NoPartition::update(std::shared_ptr<RenderableComponent> a_pComponent)
{
}

void NoPartition::remove(std::shared_ptr<RenderableComponent> a_pComponent)
{
	m_Container.erase(a_pComponent);
}

void NoPartition::clear()
{
	m_Container.clear();
}

void NoPartition::getVisibleList(std::shared_ptr<CameraComponent> a_pTargetCamera, std::vector< std::shared_ptr<RenderableComponent> > &a_Output)
{
	a_Output.resize(m_Container.size());
	std::copy(m_Container.begin(), m_Container.end(), a_Output.begin());
}

}