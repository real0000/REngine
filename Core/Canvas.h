// Canvas.h
//
// 2017/07/20 Ian Wu/Real0000
//

#ifndef _CANVAS_H_
#define _CANVAS_H_

namespace R
{

class Scene;
class InputMediator;

class EngineCanvas : public GraphicCanvas
{
public:
	EngineCanvas(wxWindow *a_pParent);
	virtual ~EngineCanvas();

	void resize(glm::ivec2 a_NewSize);
	
private:
	std::shared_ptr<Scene> m_RefScene;
};

}

#endif