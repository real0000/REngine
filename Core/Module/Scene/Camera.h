// Camera.cpp
//
// 2015/01/21 Ian Wu/Real0000
//

#ifndef _CAMERA_H_
#define _CAMERA_H_

#include "Module/Scene/SceneNode.h"

namespace R
{

class Camera : public SceneNode
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
	Camera();
	virtual ~Camera();

	void setEyeCenter(glm::vec3 l_Eye, glm::vec3 l_Center);
	void setEye(glm::vec3 l_Eye);
	glm::vec3 getEye(){ return m_Eye; }
	void setLookAt(glm::vec3 l_Center);
	glm::vec3 getLookAt(){ return m_LookAt; }
	void setUp(glm::vec3 l_Up);
	glm::vec3 getUp(){ return m_Up; }
	void setZoom(float l_Zoom);
	float getZoom(){ return m_Zoom; }
	void setOrthoView(float l_Width, float l_Height);
    void setPerspectiveView(float l_Fovy, float l_Aspect, float l_Near, float l_Far);
	glm::vec4 getPerspectiveParam(){ return m_PersParam; }
	bool isOrthoView(){ return m_bOrtho; }
	
	glm::mat4x4* getMatrix(unsigned int l_ID){ if( l_ID < NUM_MATRIX ) return &(m_Matrices[l_ID]); return nullptr; }
	glm::mat4x4* getMatrix(){ return m_Matrices; }
	
private:
	void calView();
	void calProjection();
	void calViewProjection();

	glm::mat4x4 m_Matrices[NUM_MATRIX];
	glm::vec2 m_ViewSize;
	glm::vec4 m_PersParam;
	float m_Zoom;
	bool m_bOrtho;
	
	glm::vec3 m_Eye, m_LookAt, m_Up;
};

}

#endif