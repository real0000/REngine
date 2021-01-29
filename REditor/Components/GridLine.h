// GridLine.h
//
// 2021/01/29 Ian Wu/Real0000
//

#ifndef _GRID_LINE_H_
#define _GRID_LINE_H_

namespace R
{

class GridLine : public EngineComponent
{
	COMPONENT_HEADER(GridLine)
public:
	enum GridPlane
	{
		XY_PLANE = 0,
		XZ_PLANE,
		YZ_PLANE
	};
public:
	virtual ~GridLine();

	void setup(GridPlane a_Plane, float a_Length, float a_Unit);
	
private:
	GridLine(std::shared_ptr<Scene> a_pRefScene, std::shared_ptr<SceneNode> a_pNode);

	std::shared_ptr<VertexBuffer> m_VertexBuffer;
	std::shared_ptr<IndexBuffer> m_IndexBuffer;
};

}

#endif