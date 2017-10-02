// Deferred.h
//
// 2017/08/14 Ian Wu/Real0000
//

#ifndef __DEFERRED_H_
#define __DEFERRED_H_

#include "RenderPipline.h"

namespace R
{
class TextureUnit;

class DeferredRenderer : public RenderPipeline
{
public:
	DeferredRenderer(SharedSceneMember *a_pSharedMember);
	virtual ~DeferredRenderer();

	virtual void render(std::shared_ptr<CameraComponent> a_pCamera);

private:
	std::set< std::shared_ptr<CameraComponent> > m_CameraCmdCache;
};

}

#endif