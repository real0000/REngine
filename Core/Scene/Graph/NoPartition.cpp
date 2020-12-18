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
NoPartition* NoPartition::create(boost::property_tree::ptree &a_Src)
{
	return new NoPartition();
}

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

void NoPartition::saveSetting(boost::property_tree::ptree &a_Dst)
{
	boost::property_tree::ptree l_Attr;
	l_Attr.add("type", NoPartition::typeName());

	a_Dst.add_child("<xmlattr>", l_Attr);
}

void NoPartition::getVisibleList(std::shared_ptr<Camera> a_pTargetCamera, std::vector<std::shared_ptr<RenderableComponent>> &a_Output)
{
	getAllComponent(a_Output);
}

void NoPartition::getAllComponent(std::vector<std::shared_ptr<RenderableComponent>> &a_Output)
{
	a_Output.reserve(m_Container.size());
	for( auto it=m_Container.begin() ; it!=m_Container.end() ; ++it ) a_Output.push_back(*it);
}
#pragma endregion

}