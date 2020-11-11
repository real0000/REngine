// NoPartition.h
//
// 2017/08/24 Ian Wu/Real0000
//

#ifndef _NO_PARTITION_H_
#define _NO_PARTITION_H_

#include "ScenePartition.h"

namespace R
{
	
class NoPartition : public ScenePartition
{
	friend class NoPartition;
public:
	static NoPartition* create(boost::property_tree::ptree &a_Src);
	static std::string typeName(){ return "NoPartition"; }
	virtual ~NoPartition();

	virtual void add(std::shared_ptr<RenderableComponent> a_pComponent);
	virtual void update(std::shared_ptr<RenderableComponent> a_pComponent);
	virtual void remove(std::shared_ptr<RenderableComponent> a_pComponent);
	virtual void clear();

	virtual void getVisibleList(std::shared_ptr<Camera> a_pTargetCamera, std::vector<std::shared_ptr<RenderableComponent>> &a_Output);
	virtual void getAllComponent(std::vector<std::shared_ptr<RenderableComponent>> &a_Output);

private:
	NoPartition();

	std::set< std::shared_ptr<RenderableComponent> > m_Container;
};

}

#endif