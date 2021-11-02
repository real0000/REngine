// DebugLineHelper.cpp
//
// 2021/04/28 Ian Wu/Real0000
//

#include "REngine.h"
#include "DebugLineHelper.h"

namespace R
{

#pragma region DebugLineHelper
//
// DebugLineHelper
// 
DebugLineHelper& DebugLineHelper::singleton()
{
	static DebugLineHelper s_Instance;
	return s_Instance;
}

DebugLineHelper::DebugLineHelper()
	: m_Name(wxT(""))
{
	std::shared_ptr<Asset> l_pRuntimeMesh = AssetManager::singleton().createRuntimeAsset<MeshAsset>();
	l_pRuntimeMesh->getComponent<MeshAsset>()->init(VTXFLAG_POSITION);
	m_Name = l_pRuntimeMesh->getKey();

	std::shared_ptr<Asset> l_pRuntimeMaterial = AssetManager::singleton().createRuntimeAsset<MaterialAsset>();
	MaterialAsset *l_pMatInst = l_pRuntimeMaterial->getComponent<MaterialAsset>();
	l_pMatInst->init(ProgramManager::singleton().getData(DefaultPrograms::Line));
	m_pTempBlock = l_pMatInst->createExternalBlock(ShaderRegType::UavBuffer, "InstanceInfo", 1024);
	l_pMatInst->setBlock("InstanceInfo", m_pTempBlock);
	l_pMatInst->setTopology(Topology::line_list);

	MeshAsset *l_pAssetInst = l_pRuntimeMesh->getComponent<MeshAsset>();
	{ // Box
		//      3______ 7
		//      /     /|
		//    2/____6/ |
		//     | 1   | /5
		//     |_____|/
		//    0       4
		const glm::vec3 c_BoxPos[8] = {
			glm::vec3(-0.5f, -0.5f, -0.5f),
			glm::vec3(-0.5f, -0.5f, 0.5f),
			glm::vec3(-0.5f, 0.5f, -0.5f),
			glm::vec3(-0.5f, 0.5f, 0.5f),
			glm::vec3(0.5f, -0.5f, -0.5f),
			glm::vec3(0.5f, -0.5f, 0.5f),
			glm::vec3(0.5f, 0.5f, -0.5f),
			glm::vec3(0.5f, 0.5f, 0.5f)};
		const unsigned int c_BoxIdx[24] = {
			2, 6,
			6, 7,
			7, 3,
			3, 2,
			3, 1,
			2, 0,
			6, 4,
			7, 5,
			0, 4,
			4, 5,
			5, 1,
			1, 0};
		unsigned int *l_pIdxBuffer = nullptr;
		glm::vec3 *l_pPosBuffer = nullptr;
		MeshAsset::Instance *l_pInst = l_pAssetInst->addSubMesh(8, 24, &l_pIdxBuffer, &l_pPosBuffer);
		memcpy(l_pIdxBuffer, c_BoxIdx, sizeof(c_BoxIdx));
		memcpy(l_pPosBuffer, c_BoxPos, sizeof(c_BoxPos));
		l_pInst->m_IndexCount = 24;
		l_pInst->m_Name = wxT("DebugBox");
		l_pInst->m_VtxFlag = VTXFLAG_POSITION | VTXFLAG_USE_WORLD_MAT;
		l_pInst->m_VisibleBoundingBox = glm::aabb(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f));
		l_pInst->m_PhysicsBoundingBox.push_back(l_pInst->m_VisibleBoundingBox);
		l_pInst->m_Materials.insert(std::make_pair(MATSLOT_TRANSPARENT, l_pRuntimeMaterial));
	}

	{ // Cone - 36 edge
		//	    /\ 0
		//     /  \
		//    /    \
		//   /______\ 1 ~ 36
		unsigned int *l_pIdxBuffer = nullptr;
		glm::vec3 *l_pPosBuffer = nullptr;
		MeshAsset::Instance *l_pInst = l_pAssetInst->addSubMesh(37, 144, &l_pIdxBuffer, &l_pPosBuffer);
		l_pInst->m_IndexCount = 144;
		l_pPosBuffer[0] = glm::vec3(0.0f, 1.0f, 0.0f);
		for( unsigned int i=0 ; i<36 ; ++i )
		{
			float l_Angle = glm::pi<float>() / 18.0f * i;
			float l_Cos = cos(l_Angle);
			float l_Sin = sin(l_Angle);
			l_pPosBuffer[i+1] = glm::vec3(0.5f * l_Cos, 0.0f, 0.5f * l_Sin);

			l_pIdxBuffer[i*4] = 0;
			l_pIdxBuffer[i*4 + 1] = i+1;
			l_pIdxBuffer[i*4 + 2] = i+1;
			l_pIdxBuffer[i*4 + 3] = ((i+1) % 36)+1;
		}
		l_pInst->m_Name = wxT("DebugCone");
		l_pInst->m_VtxFlag = VTXFLAG_POSITION | VTXFLAG_USE_WORLD_MAT;
		l_pInst->m_VisibleBoundingBox = glm::aabb(glm::vec3(0.0f, 0.5f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f));
		l_pInst->m_PhysicsBoundingBox.push_back(l_pInst->m_VisibleBoundingBox);
		l_pInst->m_Materials.insert(std::make_pair(MATSLOT_TRANSPARENT, l_pRuntimeMaterial));
	}

	{ // sphere
		// x - y - z aligned circle
		// 36 segment per circle
		unsigned int *l_pIdxBuffer = nullptr;
		glm::vec3 *l_pPosBuffer = nullptr;
		MeshAsset::Instance *l_pInst = l_pAssetInst->addSubMesh(36 * 3, 72 * 3, &l_pIdxBuffer, &l_pPosBuffer);
		l_pInst->m_IndexCount = 72 * 3;
		for( unsigned int i=0 ; i<36 ; ++i )
		{
			float l_Angle = glm::pi<float>() / 18.0f * i;
			float l_Cos = cos(l_Angle) * 0.5f;
			float l_Sin = sin(l_Angle) * 0.5f;
			l_pPosBuffer[i] = glm::vec3(0.0f, l_Cos, l_Sin);
			l_pPosBuffer[36+i] = glm::vec3(l_Cos, 0.0f, l_Sin);
			l_pPosBuffer[72+i] = glm::vec3(l_Cos, l_Sin, 0.0f);

			l_pIdxBuffer[i*2] = i;
			l_pIdxBuffer[i*2+1] = ((i+1) % 36);
			l_pIdxBuffer[72 + i*2] = 36 + i;
			l_pIdxBuffer[72 + i*2+1] = 36 + ((i+1) % 36);
			l_pIdxBuffer[144 + i*2] = 72 + i;
			l_pIdxBuffer[144 + i*2+1] = 72 + ((i+1) % 36);
		}
		l_pInst->m_Name = wxT("DebugSphere");
		l_pInst->m_VtxFlag = VTXFLAG_POSITION | VTXFLAG_USE_WORLD_MAT;
		l_pInst->m_VisibleBoundingBox = glm::aabb(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f));
		l_pInst->m_PhysicsBoundingBox.push_back(l_pInst->m_VisibleBoundingBox);
		l_pInst->m_Materials.insert(std::make_pair(MATSLOT_TRANSPARENT, l_pRuntimeMaterial));
	}
	l_pAssetInst->updateMeshData(glm::ivec2(0, 8 + 37 + 36 * 3), glm::ivec2(0, 24 + 144 + 72 * 3));
}

DebugLineHelper::~DebugLineHelper()
{
}

void DebugLineHelper::addDebugLine(std::shared_ptr<SceneNode> a_pNode)
{
	std::vector<std::shared_ptr<Camera>> l_Cameras;
	a_pNode->getComponentsInChildren<Camera>(l_Cameras);
	
	std::vector<std::shared_ptr<RenderableMesh>> l_Meshes;
	a_pNode->getComponentsInChildren<RenderableMesh>(l_Meshes);

	std::vector<std::shared_ptr<OmniLight>> l_OmniLights;
	a_pNode->getComponentsInChildren<OmniLight>(l_OmniLights);

	std::vector<std::shared_ptr<SpotLight>> l_SpotLights;
	a_pNode->getComponentsInChildren<SpotLight>(l_SpotLights);

	for( unsigned int i=0 ; i<l_Cameras.size() ; ++i ) addDebugLine(l_Cameras[i]);
	for( unsigned int i=0 ; i<l_Meshes.size() ; ++i ) addDebugLine(l_Meshes[i]);
	for( unsigned int i=0 ; i<l_OmniLights.size() ; ++i ) addDebugLine(l_OmniLights[i]);
	for( unsigned int i=0 ; i<l_SpotLights.size() ; ++i ) addDebugLine(l_SpotLights[i]);
	m_pTempBlock->sync(true, false);
}

void DebugLineHelper::addDebugLine(std::shared_ptr<Camera> a_pCamera)
{
	glm::vec4 l_View(a_pCamera->getViewParam());
	switch( a_pCamera->getCameraType() )
	{
		case Camera::PERSPECTIVE:{
			
			}break;

		case Camera::ORTHO:{

			}break;

		default: break;
	}
}

void DebugLineHelper::addDebugLine(std::shared_ptr<RenderableMesh> a_pMesh)
{
	MeshAsset *l_pMesh = a_pMesh->getMesh()->getComponent<MeshAsset>();
	MeshAsset::Instance *l_pInst = l_pMesh->getMeshes()[a_pMesh->getMeshIdx()];

	std::shared_ptr<SceneNode> l_pChild = a_pMesh->getOwner()->addChild();
	l_pChild->setPosition(l_pInst->m_VisibleBoundingBox.m_Center);
	l_pChild->setScale(l_pInst->m_VisibleBoundingBox.m_Size);

	std::shared_ptr<RenderableMesh> l_pLine = l_pChild->addComponent<RenderableMesh>();
	l_pLine->setMesh(AssetManager::singleton().getAsset(m_Name), BOX);
	RenderableMesh::MaterialData l_Pair = l_pLine->getMaterial(MATSLOT_TRANSPARENT);
	MaterialAsset *l_pMatInst = l_Pair.second->getComponent<MaterialAsset>();
	l_pMatInst->setParam("InstanceInfo", "m_Params", l_Pair.first, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
}

void DebugLineHelper::addDebugLine(std::shared_ptr<OmniLight> a_pOmniLight)
{
	std::shared_ptr<SceneNode> l_pChild = a_pOmniLight->getOwner()->addChild();
	l_pChild->setPosition(glm::vec3(0.0f, 0.0f, 0.0f));
	l_pChild->setScale(glm::vec3(1.0f));

	std::shared_ptr<RenderableMesh> l_pLine = l_pChild->addComponent<RenderableMesh>();
	l_pLine->setMesh(AssetManager::singleton().getAsset(m_Name), SPHERE);
	RenderableMesh::MaterialData l_Pair = l_pLine->getMaterial(MATSLOT_TRANSPARENT);
	MaterialAsset *l_pMatInst = l_Pair.second->getComponent<MaterialAsset>();
	l_pMatInst->setParam("InstanceInfo", "m_Params", l_Pair.first, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
}

void DebugLineHelper::addDebugLine(std::shared_ptr<SpotLight> a_pLight)
{
}

/*glm::vec3 DebugLineHelper::calculateBaseScale(glm::vec3 a_BoxSize)
{
	return a_BoxSize;
}

glm::vec3 DebugLineHelper::calculateBaseScale(float a_Radius, float a_Tall)
{
	return glm::vec3(a_Radius, a_Tall, a_Radius) / glm::vec3(0.5f, 1.0f, 0.5f);
}

glm::vec3 DebugLineHelper::calculateBaseScale(float a_Radius)
{
	return glm::vec3(a_Radius, a_Radius, a_Radius) / glm::vec3(0.5f, 0.5f, 0.5f);
}

std::shared_ptr<Asset> DebugLineHelper::getMesh()
{
	return AssetManager::singleton().getAsset(m_Name);
}*/
#pragma endregion

}