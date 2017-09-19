// Camera.cpp
//
// 2015/01/21 Ian Wu/Real0000
//

#ifndef _CAMERA_H_
#define _CAMERA_H_

namespace R
{
	
struct SharedSceneMember;
class EngineComponent;

class CameraComponent : public EngineComponent
{
	friend class EngineComponent;
public:
	enum
	{
		VIEW = 0,
		PROJECTION,
		VIEWPROJECTION,
		INVERTVIEW,

		TETRAHEDRON_0 = 0,
		TETRAHEDRON_1,
		TETRAHEDRON_2,
		TETRAHEDRON_3,

		CUBEMAP_POSITIVE_X = 0,
		CUBEMAP_NEGATIVE_X,
		CUBEMAP_POSITIVE_Y,
		CUBEMAP_NEGATIVE_Y,
		CUBEMAP_POSITIVE_Z,
		CUBEMAP_NEGATIVE_Z,

		NUM_MATRIX
	};
	enum CameraType
	{
		ORTHO = 0,
		PERSPECTIVE,
		TETRAHEDRON,
		CUBE,
	};
	virtual ~CameraComponent();// don't call this method directly
	
	virtual void start();
	virtual void end();

	virtual unsigned int typeID(){ return COMPONENT_CAMERA; }
	virtual bool isHidden(){ return false; }

	void setOrthoView(float a_Width, float a_Height, float a_Near, float a_Far);
    void setPerspectiveView(float a_Fovy, float a_Aspect, float a_Near, float a_Far);
	void setTetrahedonView(float a_Range);// implement later...
	void setCubeView(float a_Range);
	glm::vec4 getViewParam(){ return m_ViewParam; }
	CameraType getCameraType(){ return m_Type; }
	
	glm::mat4x4* getMatrix(unsigned int a_ID){ assert( a_ID < NUM_MATRIX ); return &(m_Matrices[a_ID]); }
	glm::mat4x4* getMatrix(){ return m_Matrices; }
	glm::frustumface& getFrustum(){ return m_Frustum; }

	virtual void transformListener(glm::mat4x4 &a_NewTransform);

private:
	CameraComponent(SharedSceneMember *a_pSharedMember, std::shared_ptr<SceneNode> a_pOwner);

	void calView();
	void calProjection();
	void calViewProjection();

	glm::mat4x4 m_Matrices[NUM_MATRIX];
	glm::vec4 m_ViewParam;
	CameraType m_Type;
	
	glm::frustumface m_Frustum;
};

}

#endif