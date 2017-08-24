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

enum
{
	RENDER_STAGE_Z_PREPASS = 0,
	RENDER_STAGE_GBUFFER = 1000,
	RENDER_STAGE_LIGHTUP = 2000,
	RENDER_STAGE_FOWARD = 2000,
	RENDER_STAGE_TRANSPARENT = 3000,
};

class RenderPipeline
{
public:
	RenderPipeline(SharedSceneMember *a_pMember);
	virtual ~RenderPipeline();

	void setupVisibleList(std::shared_ptr<CameraComponent> a_pTargetCamera);

	virtual void render(unsigned int a_Stage) = 0;

protected:
	SharedSceneMember* getSceneMember(){ return m_pMember; }


private:
	SharedSceneMember *m_pMember;

};

}

#endif