// Camera.cpp
//
// 2015/01/21 Ian Wu/Real0000
//

#ifndef _CAMERA_H_
#define _CAMERA_H_

namespace R
{

class EngineComponent;

class CameraComponent : public EngineComponent
{
public:
	enum
	{
		VIEW = 0,
		PROJECTION,
		VIEWPROJECTION,
		INVERTVIEW,

		NUM_MATRIX
	};
	CameraComponent(std::shared_ptr<SceneNode> a_pOwner);
	virtual ~CameraComponent();
	
	virtual unsigned int typeID(){ return COMPONENT_CAMERA; }
	virtual bool isHidden(){ return false; }

	void setEyeCenter(glm::vec3 a_Eye, glm::vec3 a_Center);
	void setEye(glm::vec3 a_Eye);
	glm::vec3 getEye(){ return m_Eye; }
	void setLookAt(glm::vec3 a_Center);
	glm::vec3 getLookAt(){ return m_LookAt; }
	void setUp(glm::vec3 a_Up);
	glm::vec3 getUp(){ return m_Up; }
	void setOrthoView(float a_Width, float a_Height, float a_Near, float a_Far);
    void setPerspectiveView(float a_Fovy, float a_Aspect, float a_Near, float a_Far);
	glm::vec4 getViewParam(){ return m_ViewParam; }
	bool isOrthoView(){ return m_bOrtho; }
	
	glm::mat4x4* getMatrix(unsigned int a_ID){ if( a_ID < NUM_MATRIX ) return &(m_Matrices[a_ID]); return nullptr; }
	glm::mat4x4* getMatrix(){ return m_Matrices; }
	
private:
	void calView();
	void calProjection();
	void calViewProjection();

	glm::mat4x4 m_Matrices[NUM_MATRIX];
	glm::vec4 m_ViewParam;
	bool m_bOrtho;
	
	glm::vec3 m_Eye, m_LookAt, m_Up;
};

}

#endif