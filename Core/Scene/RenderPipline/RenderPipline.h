// RenderPipline.h
//
// 2017/08/14 Ian Wu/Real0000
//

#ifndef _RENDER_PIPELINE_H_
#define _RENDER_PIPELINE_H_

namespace R
{

class Camera;
class RenderableComponent;

#define INIT_CONTAINER_SIZE 1024// general setting
#define INIT_LIGHT_SIZE 1024
#define OPAQUE_STAGE_END 1000

// draw flags
#define DRAW_WIREFRAME		0x01
#define DRAW_DEBUG_TEXTURE	0x02

class RenderPipeline
{
public:
	RenderPipeline(std::shared_ptr<Scene> a_pScene);
	virtual ~RenderPipeline();

	void setDrawFlag(unsigned int a_Flag);

	virtual void saveSetting(boost::property_tree::ptree &a_Dst) = 0;
	virtual void render(std::shared_ptr<Camera> a_pCamera, GraphicCanvas *a_pCanvas) = 0;
	virtual void canvasResize(glm::ivec2 a_Size) = 0;
	virtual void drawFlagChanged(unsigned int a_Flag) = 0;
	
protected:
	 std::shared_ptr<Scene> getScene(){ return m_pRefScene; }
	 unsigned int getDrawFlag(){ return m_DrawFlag; }

private:
	std::shared_ptr<Scene> m_pRefScene;// scene node will be null
	unsigned int m_DrawFlag;
};

}

#endif