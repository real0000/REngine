// CameraController.h
//
// 2020/12/21 Ian Wu/Real0000
//

#ifndef _CAMERA_CONTROLLER_H_
#define _CAMERA_CONTROLLER_H_

#define CAMERA_SPEEDUP_DURATION 3.0f

namespace R
{

class CameraController : public EngineComponent
{
	COMPONENT_HEADER(CameraController)
public:
	virtual ~CameraController();
	
	virtual void start();

	virtual void loadComponent(boost::property_tree::ptree &a_Src);
	virtual void saveComponent(boost::property_tree::ptree &a_Dst);
	virtual bool inputListener(InputData &a_Input);
	virtual void updateListener(float a_Delta);

	void setMaxSpeed(float a_MaxSpeed){ m_MaxSpeed = a_MaxSpeed; }

private:
	CameraController(std::shared_ptr<Scene> a_pRefScene, std::shared_ptr<SceneNode> a_pNode);

	void mouseDown();
	void mouseMove();
	void mouseUp();

	bool m_bValid;
	glm::vec2 m_MousePt, m_PrevPt;
	float m_MaxSpeed;
	float m_MovingSpeed;
	glm::ivec3 m_MovingFlag;
};

}

#endif