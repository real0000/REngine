// DebugLineHelper.h
//
// 2021/04/28 Ian Wu/Real0000
//

#ifndef _DEBUG_LINE_HELPER_H_
#define _DEBUG_LINE_HELPER_H_

namespace R
{

class Asset;
class Camera;
class OmniLight;
class RenderableMesh;
class SceneNode;
class SpotLight;

class DebugLineHelper
{
public:
	enum LineType
	{
		BOX = 0,
		CONE,
		SPHERE,

		COUNT
	};
public:
	static DebugLineHelper& singleton();

	// draw bounding lines
	void addDebugLine(std::shared_ptr<SceneNode> a_pNode);// all camera | mesh | light under node
	void addDebugLine(std::shared_ptr<Camera> a_pCamera);
	void addDebugLine(std::shared_ptr<RenderableMesh> a_pMesh);
	void addDebugLine(std::shared_ptr<OmniLight> a_pOmniLight);
	void addDebugLine(std::shared_ptr<SpotLight> a_pLight);

private:
	DebugLineHelper();
	virtual ~DebugLineHelper();

	wxString m_Name;
};

}

#endif