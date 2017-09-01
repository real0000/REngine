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

#define MATERIAL_INIT_CONTAINER_SIZE 1024

class RenderPipeline
{
public:
	RenderPipeline(SharedSceneMember *a_pSharedMember);
	virtual ~RenderPipeline();

	void clear();
	void addStage(unsigned int a_Stage);
	void removeStage(unsigned int a_Stage);
	void render(std::shared_ptr<CameraComponent> a_pTargetCamera);

	virtual void buildStaticCommand() = 0;
	virtual void render(std::vector< std::shared_ptr<Material> > &a_Materials) = 0;

protected:
	SharedSceneMember* getSharedMember(){ return m_pSharedMember; }

private:
	SharedSceneMember *m_pSharedMember;
	std::set<unsigned int> m_Stages;
};

}

#endif