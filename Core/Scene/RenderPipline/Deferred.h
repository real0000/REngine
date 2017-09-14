// Deferred.h
//
// 2017/08/14 Ian Wu/Real0000
//

#ifndef __DEFERRED_H_
#define __DEFERRED_H_

#include "RenderPipline.h"

namespace R
{

class DeferredRenderer : public RenderPipeline
{
public:
	DeferredRenderer(SharedSceneMember *a_pSharedMember);
	virtual ~DeferredRenderer();
	
	virtual void add(std::shared_ptr<CameraComponent> a_pCamera);
	virtual void remove(std::shared_ptr<CameraComponent> a_pCamera);
	virtual void clear();
	virtual void buildStaticCommand();
	virtual void render();

private:
	//std::map< std::shared_ptr<CameraComponent>,  > m_CameraCache;
};

}

#endif