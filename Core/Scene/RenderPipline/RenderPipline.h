// RenderPipline.h
//
// 2017/08/14 Ian Wu/Real0000
//

#ifndef _RENDER_PIPELINE_H_
#define _RENDER_PIPELINE_H_

namespace R
{

struct SharedSceneMember;
class CameraComponent;
class Material;
class RenderableComponent;

#define INIT_CONTAINER_SIZE 1024// general setting
#define OPAQUE_STAGE_END 1000

class RenderPipeline
{
public:
	RenderPipeline(SharedSceneMember *a_pSharedMember);
	virtual ~RenderPipeline();

	virtual void render(std::shared_ptr<CameraComponent> a_pCamera) = 0;

protected:
	SharedSceneMember* getSharedMember(){ return m_pSharedMember; }

private:
	SharedSceneMember *m_pSharedMember;// scene node will be null
};

}

#endif