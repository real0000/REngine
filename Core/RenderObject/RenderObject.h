// RenderObject.h
//
// 2017/07/19 Ian Wu/Real0000
//

#ifndef _RENDER_OBJECT_H_
#define _RENDER_OBJECT_H_

namespace R
{

class RenderableComponent : public EngineComponent
{
	friend class RenderableComponent;
public:
	RenderableComponent(std::shared_ptr<Scene> a_pRefScene, std::shared_ptr<SceneNode> a_pOwner);
	virtual ~RenderableComponent(); // don't call this method directly
	
	virtual void setShadowed(bool a_bShadow) = 0;
	virtual bool getShadowed() = 0;
	glm::daabb& boundingBox(){ return m_BoundingBox; }
	
private:
	glm::daabb m_BoundingBox;
};

}

#endif