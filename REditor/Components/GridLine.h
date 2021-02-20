// GridLine.h
//
// 2021/02/17 Ian Wu/Real0000
//

#ifndef _GRID_LINE_H_
#define _GRID_LINE_H_

namespace R
{

class MaterialAsset;

// editor asset
#define GRID_LINE_ASSET_NAME wxT("GridLine.Material")

class GridLine : public RenderableMesh
{
	COMPONENT_HEADER(GridLine)
public:
	enum GridPlane
	{
		XY = 0,
		XZ,
		YZ
	};
public:
	virtual ~GridLine();
	
	virtual void start();
	virtual void setShadowed(bool a_bShadow){}
	virtual bool getShadowed(){ return false; }
	virtual bool runtimeOnly(){ return true; }

	void setGridType(GridPlane a_Plane);

private:
	GridLine(std::shared_ptr<Scene> a_pRefScene, std::shared_ptr<SceneNode> a_pNode);

	std::shared_ptr<Asset> m_pGridMat;
	MaterialAsset *m_pGridMatInst;
};

}

#endif