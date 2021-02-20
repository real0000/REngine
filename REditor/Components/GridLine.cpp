// GridLine.cpp
//
// 2021/02/17 Ian Wu/Real0000
//

#include "REngine.h"
#include "REditor.h"

namespace R
{

#pragma region GridLine
//
// GridLine
//
GridLine::GridLine(std::shared_ptr<Scene> a_pRefScene, std::shared_ptr<SceneNode> a_pNode)
	: RenderableMesh(a_pRefScene, a_pNode)
	, m_pGridMat(AssetManager::singleton().createAsset(GRID_LINE_ASSET_NAME))
	, m_pGridMatInst(nullptr)
{
	m_pGridMatInst = m_pGridMat->getComponent<MaterialAsset>();
	m_pGridMatInst->init(ProgramManager::singleton().getData(DefaultPrograms::Grid));

}

void GridLine::start()
{
	RenderableMesh::start();
	setMesh(AssetManager::singleton().getAsset(QUAD_MESH_ASSET_NAME), 0);
	setMaterial(MATSLOT_TRANSPARENT, m_pGridMat);
}

void GridLine::setGridType(GridPlane a_Plane)
{
	switch(a_Plane)
	{
		case XY:
			break;

		case YZ:
			break;

		case XZ:
			break;
	}
}
#pragma endregion

}