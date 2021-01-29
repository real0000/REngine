// Camera.cpp
//
// 2015/01/21 Ian Wu/Real0000
//

#ifndef _CAMERA_H_
#define _CAMERA_H_

#include "RenderObject/RenderObject.h"

namespace R
{

class MaterialBlock;
class TextureAsset;

class Camera : public RenderableComponent
{
	COMPONENT_HEADER(Camera)
public:
	enum
	{
		VIEW = 0,
		PROJECTION,
		VIEWPROJECTION,
		INVERTVIEW,
		INVERTVIEWPROJECTION,

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
	virtual ~Camera();// don't call this method directly
	
	virtual void postInit();
	virtual void start();
	virtual void end();
	virtual void hiddenFlagChanged();

	virtual void setShadowed(bool a_bShadow){}
	virtual bool getShadowed(){ return false; }
	
	virtual void loadComponent(boost::property_tree::ptree &a_Src);
	virtual void saveComponent(boost::property_tree::ptree &a_Dst);

	void setOrthoView(float a_Width, float a_Height, float a_Near, float a_Far, glm::mat4x4 &a_Transform);
    void setPerspectiveView(float a_Fovy, float a_Aspect, float a_Near, glm::mat4x4 &a_Transform);
	void setTetrahedonView(glm::mat4x4 &a_Transform);// implement later...
	void setCubeView(glm::mat4x4 &a_Transform);
	glm::vec4 getViewParam(){ return m_ViewParam; }
	CameraType getCameraType(){ return m_Type; }
	
	void getCameraParam(glm::vec3 &a_Eye, glm::vec3 &a_Dir, glm::vec3 &a_Up);
	glm::mat4x4* getMatrix(unsigned int a_ID){ assert( a_ID < NUM_MATRIX ); return &(m_Matrices[a_ID]); }
	glm::mat4x4* getMatrix(){ return m_Matrices; }
	std::shared_ptr<MaterialBlock> getMaterialBlock(){ return m_pCameraBlock; }
	glm::frustumface& getFrustum(){ return m_Frustum; }

	virtual void transformListener(const glm::mat4x4 &a_NewTransform) override;

private:
	Camera(std::shared_ptr<Scene> a_pRefScene, std::shared_ptr<SceneNode> a_pOwner);

	void calView(const glm::mat4x4 &a_NewTransform);
	void calProjection(const glm::mat4x4 &a_NewTransform);
	void calViewProjection(const glm::mat4x4 &a_NewTransform);

	glm::mat4x4 m_Matrices[NUM_MATRIX];
	glm::vec4 m_ViewParam;
	CameraType m_Type;
	
	glm::frustumface m_Frustum;
	std::shared_ptr<MaterialBlock> m_pCameraBlock;
};

}

#endif