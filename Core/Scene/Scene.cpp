// Scene.cpp
//
// 2015/02/10 Ian Wu/Real0000
//

#include "CommonUtil.h"
#include "RGDeviceWrapper.h"
#include "Core.h"
#include "Scene.h"

#include "RenderObject/Light.h"
#include "RenderObject/Mesh.h"

#include "Asset/AssetBase.h"
#include "Asset/MaterialAsset.h"
#include "Asset/MeshAsset.h"
#include "Asset/SceneAsset.h"
#include "Scene/Camera.h"
#include "Scene/Graph/NoPartition.h"
#include "Scene/Graph/Octree.h"
#include "Scene/RenderPipline/Deferred.h"
#include "Scene/RenderPipline/ShadowMap.h"

namespace R
{

#pragma region SceneBatcher
#pragma region SceneBatcher::SingletonBatchData
SceneBatcher::SingletonBatchData::SingletonBatchData()
	: m_SkinBlock(nullptr), m_WorldBlock(nullptr)
	, m_SkinSlotManager(), m_WorldSlotManager()
{
	auto &l_DescList = ProgramManager::singleton().getData(DefaultPrograms::TextureOnly)->getBlockDesc(ShaderRegType::UavBuffer);
	auto it = std::find_if(l_DescList.begin(), l_DescList.end(), [=](ProgramBlockDesc *a_pDesc) -> bool{ return "m_SkinTransition" == a_pDesc->m_Name; });
	m_SkinBlock = MaterialBlock::create(ShaderRegType::UavBuffer, *it, BATCHDRAW_UNIT);
	m_SkinSlotManager.init(BATCHDRAW_UNIT);

	it = std::find_if(l_DescList.begin(), l_DescList.end(), [=](ProgramBlockDesc *a_pDesc) -> bool{ return "m_NormalTransition" == a_pDesc->m_Name; });
	m_WorldBlock = MaterialBlock::create(ShaderRegType::UavBuffer, *it, BATCHDRAW_UNIT);
	m_WorldSlotManager.init(BATCHDRAW_UNIT);
}

SceneBatcher::SingletonBatchData::~SingletonBatchData()
{
	m_SkinBlock = nullptr;
	m_WorldBlock = nullptr;
}
#pragma endregion
//
// SceneBatcher
//
SceneBatcher::SingletonBatchData *SceneBatcher::m_pSingletonBatchData = nullptr;
unsigned int SceneBatcher::m_NumSharedMember = 0;
SceneBatcher::SceneBatcher()
	: m_VertexBufffer(nullptr)
	, m_IndexBuffer(nullptr)
{
	if( nullptr == m_pSingletonBatchData ) m_pSingletonBatchData = new SingletonBatchData();
	++m_NumSharedMember;
}

SceneBatcher::~SceneBatcher()
{
	m_VertexBufffer = nullptr;
	m_IndexBuffer = nullptr;

	--m_NumSharedMember;
	if( 0 == m_NumSharedMember ) delete m_pSingletonBatchData;
	m_pSingletonBatchData = nullptr;
	
	while( !m_InstancVtxBuffers.empty() )
	{
		GDEVICE()->freeVertexBuffer(m_InstancVtxBuffers.front());
		m_InstancVtxBuffers.pop_front();
	}
}

int SceneBatcher::requestInstanceVtxBuffer()
{
	std::lock_guard<std::mutex> l_Guard(m_pSingletonBatchData->m_InstanceVtxLock);

	int l_Res = -1;
	if( m_InstancVtxBuffers.empty() ) l_Res = GDEVICE()->requestVertexBuffer(nullptr, VTXSLOT_INSTANCE, BATCHDRAW_UNIT);
	else
	{
		l_Res = m_InstancVtxBuffers.front();
		m_InstancVtxBuffers.pop_front();
	}
	return l_Res;
}

void SceneBatcher::recycleInstanceVtxBuffer(int a_BufferID)
{
	m_UsedInstancVtxBuffers.push_back(a_BufferID);
}

void SceneBatcher::renderEnd()
{
	for( unsigned int i=0 ; i<m_UsedInstancVtxBuffers.size() ; ++i ) m_InstancVtxBuffers.push_back(m_UsedInstancVtxBuffers[i]);
	m_UsedInstancVtxBuffers.clear();
}

int SceneBatcher::requestSkinSlot(std::shared_ptr<Asset> a_pAsset)
{
	auto &l_BoneMatrix = a_pAsset->getComponent<MeshAsset>()->getBones();
	if( l_BoneMatrix.empty() ) return -1;

	std::lock_guard<std::mutex> l_Locker(m_pSingletonBatchData->m_SkinSlotLock);

	auto it = m_pSingletonBatchData->m_SkinCache.find(a_pAsset);
	if( m_pSingletonBatchData->m_SkinCache.end() != it )
	{
		++it->second.second;
		return it->second.first;
	}
	
	int l_Res = m_pSingletonBatchData->m_SkinSlotManager.malloc(l_BoneMatrix.size());
	if( -1 == l_Res )
	{
		m_pSingletonBatchData->m_SkinSlotManager.extend(BATCHDRAW_UNIT);
		m_pSingletonBatchData->m_SkinBlock->extend(BATCHDRAW_UNIT);
		l_Res = m_pSingletonBatchData->m_SkinSlotManager.malloc(l_BoneMatrix.size());
	}
	m_pSingletonBatchData->m_SkinCache.insert(std::make_pair(a_pAsset, std::make_pair(l_Res, 1)));
	for( unsigned int i=0 ; i<l_BoneMatrix.size() ; ++i ) m_pSingletonBatchData->m_SkinBlock->setParam("m_SkinTransition", l_Res + i, l_BoneMatrix[i]);

	return l_Res;
}

void SceneBatcher::recycleSkinSlot(std::shared_ptr<Asset> a_pComponent)
{
	if( nullptr == a_pComponent ) return;

	std::lock_guard<std::mutex> l_Locker(m_pSingletonBatchData->m_SkinSlotLock);
	auto it = m_pSingletonBatchData->m_SkinCache.find(a_pComponent);
	if( m_pSingletonBatchData->m_SkinCache.end() == it ) return;

	--it->second.second;
	if( 0 != it->second.second ) return;
	
	m_pSingletonBatchData->m_SkinSlotManager.free(it->second.first);
	m_pSingletonBatchData->m_SkinCache.erase(it);
}

int SceneBatcher::requestWorldSlot(std::shared_ptr<RenderableMesh> a_pComponent)
{
	std::lock_guard<std::mutex> l_Locker(m_pSingletonBatchData->m_WorldSlotLock);

	auto it = m_pSingletonBatchData->m_WorldCache.find(a_pComponent);
	if( m_pSingletonBatchData->m_WorldCache.end() != it )
	{
		++it->second.second;
		return it->second.first;
	}
	
	int l_Res = m_pSingletonBatchData->m_WorldSlotManager.malloc(1);
	if( -1 == l_Res )
	{
		m_pSingletonBatchData->m_WorldSlotManager.extend(BATCHDRAW_UNIT);
		m_pSingletonBatchData->m_WorldBlock->extend(BATCHDRAW_UNIT);
		l_Res = m_pSingletonBatchData->m_WorldSlotManager.malloc(1);
	}
	m_pSingletonBatchData->m_WorldCache.insert(std::make_pair(a_pComponent, std::make_pair(l_Res, 1)));

	return l_Res;
}

void SceneBatcher::recycleWorldSlot(std::shared_ptr<RenderableMesh> a_pComponent)
{
	if( nullptr == a_pComponent ) return;

	std::lock_guard<std::mutex> l_Locker(m_pSingletonBatchData->m_WorldSlotLock);
	auto it = m_pSingletonBatchData->m_WorldCache.find(a_pComponent);
	if( m_pSingletonBatchData->m_WorldCache.end() == it ) return;

	--it->second.second;
	if( 0 != it->second.second ) return;
	
	m_pSingletonBatchData->m_WorldSlotManager.free(it->second.first);
	m_pSingletonBatchData->m_WorldCache.erase(it);
}

void SceneBatcher::calculateStaticBatch(std::shared_ptr<Scene> &a_GraphOwner)
{
	m_VertexBufffer = nullptr;
	m_IndexBuffer = nullptr;

	for( auto it=m_StaticMeshes.begin() ; it!=m_StaticMeshes.end() ; ++it ) delete it->second;
	m_StaticMeshes.clear();

	std::vector<std::shared_ptr<RenderableComponent>> l_Meshes;
	a_GraphOwner->getSceneGraph(GRAPH_STATIC_MESH)->getAllComponent(l_Meshes);

	std::set<std::shared_ptr<Asset>> l_MeshAssets;
	for( unsigned int i=0 ; i<l_Meshes.size() ; ++i )
	{
		RenderableMesh *l_pInst = reinterpret_cast<RenderableMesh *>(l_Meshes[i].get());
		l_MeshAssets.insert(l_pInst->getMesh());
	}

	unsigned int l_NumVtx = 0;
	unsigned int l_NumIdx = 0;
	for( auto it=l_MeshAssets.begin() ; it!=l_MeshAssets.end() ; ++it )
	{
		MeshAsset *l_pAssetInst = (*it)->getComponent<MeshAsset>();
		l_NumVtx += l_pAssetInst->getPosition().size();
		l_NumIdx += l_pAssetInst->getIndicies().size();
	}
	
	std::vector<glm::vec3> l_Position(l_NumVtx);
    std::vector<glm::vec4> l_Texcoord[4] = {std::vector<glm::vec4>(l_NumVtx), std::vector<glm::vec4>(l_NumVtx), std::vector<glm::vec4>(l_NumVtx), std::vector<glm::vec4>(l_NumVtx)};
    std::vector<glm::vec3> l_Normal(l_NumVtx);
    std::vector<glm::vec3> l_Tangent(l_NumVtx);
    std::vector<glm::vec3> l_Binormal(l_NumVtx);
	std::vector<glm::ivec4> l_BoneId(l_NumVtx);
	std::vector<glm::vec4> l_Weight(l_NumVtx);
	std::vector<unsigned int> l_Indicies(l_NumIdx);
	unsigned int l_BaseVtx = 0, l_BaseIdx = 0;
	for( auto it=l_MeshAssets.begin() ; it!=l_MeshAssets.end() ; ++it )
	{
		MeshCache *l_pNewCache = new MeshCache();
		m_StaticMeshes.insert(std::make_pair(*it, l_pNewCache));

		MeshAsset *l_pAssetInst = (*it)->getComponent<MeshAsset>();
		std::vector<MeshAsset::Instance*> &l_List = l_pAssetInst->getMeshes();
		for( unsigned int j=0 ; j<l_List.size() ; ++j )
		{
			MeshVtxCache l_SubMesh = {};
			l_SubMesh.m_IndexStart = l_BaseIdx + l_List[j]->m_StartIndex;
			l_SubMesh.m_IndexCount = l_List[j]->m_IndexCount;
			l_SubMesh.m_VertexStart = l_BaseVtx + l_List[j]->m_BaseVertex;
			l_pNewCache->push_back(l_SubMesh);
		}

		std::copy(l_pAssetInst->getPosition().begin(), l_pAssetInst->getPosition().end(), l_Position.begin() + l_BaseVtx);
		for( unsigned int j=0 ; j<4 ; ++j ) std::copy(l_pAssetInst->getTexcoord(j).begin(), l_pAssetInst->getTexcoord(j).end(), l_Texcoord[j].begin() + l_BaseVtx);
		std::copy(l_pAssetInst->getNormal().begin(), l_pAssetInst->getNormal().end(), l_Normal.begin() + l_BaseVtx);
		std::copy(l_pAssetInst->getTangent().begin(), l_pAssetInst->getTangent().end(), l_Tangent.begin() + l_BaseVtx);
		std::copy(l_pAssetInst->getBinormal().begin(), l_pAssetInst->getBinormal().end(), l_Binormal.begin() + l_BaseVtx);
		std::copy(l_pAssetInst->getBoneId().begin(), l_pAssetInst->getBoneId().end(), l_BoneId.begin() + l_BaseVtx);
		std::copy(l_pAssetInst->getWeight().begin(), l_pAssetInst->getWeight().end(), l_Weight.begin() + l_BaseVtx);
		std::copy(l_pAssetInst->getIndicies().begin(), l_pAssetInst->getIndicies().end(), l_Indicies.begin() + l_BaseIdx);

		l_BaseVtx += l_pAssetInst->getPosition().size();
		l_BaseIdx += l_pAssetInst->getIndicies().size();
	}

	m_VertexBufffer = std::shared_ptr<VertexBuffer>(new VertexBuffer());
	m_VertexBufffer->setNumVertex(l_NumVtx);
	m_VertexBufffer->setVertex(VTXSLOT_POSITION, l_Position.data());
	m_VertexBufffer->setVertex(VTXSLOT_TEXCOORD01, l_Texcoord[0].data());
	m_VertexBufffer->setVertex(VTXSLOT_TEXCOORD23, l_Texcoord[1].data());
	m_VertexBufffer->setVertex(VTXSLOT_TEXCOORD45, l_Texcoord[2].data());
	m_VertexBufffer->setVertex(VTXSLOT_TEXCOORD67, l_Texcoord[3].data());
	m_VertexBufffer->setVertex(VTXSLOT_NORMAL, l_Normal.data());
	m_VertexBufffer->setVertex(VTXSLOT_TANGENT, l_Tangent.data());
	m_VertexBufffer->setVertex(VTXSLOT_BINORMAL, l_Binormal.data());
	m_VertexBufffer->setVertex(VTXSLOT_BONE, l_BoneId.data());
	m_VertexBufffer->setVertex(VTXSLOT_WEIGHT, l_Weight.data());
	m_VertexBufffer->init();

	m_IndexBuffer = std::shared_ptr<IndexBuffer>(new IndexBuffer());
	m_IndexBuffer->setIndicies(true, l_Indicies.data(), l_NumIdx);
	m_IndexBuffer->init();
}

bool SceneBatcher::getBatchParam(std::shared_ptr<Asset> a_pMeshAsset, MeshCache **a_ppOutput)
{
	auto it = m_StaticMeshes.find(a_pMeshAsset);
	if( m_StaticMeshes.end() == it ) return false;
	*a_ppOutput = it->second;
	return true;
}

void SceneBatcher::bindBatchDrawData(GraphicCommander *a_pCommander)
{
	if( nullptr == m_VertexBufffer ) return;

	a_pCommander->bindVertex(m_VertexBufffer.get());
	a_pCommander->bindIndex(m_IndexBuffer.get());
}
#pragma endregion

#pragma region SceneNode
///
// SceneNode
//
std::map<std::string, std::function<std::shared_ptr<EngineComponent>(std::shared_ptr<SceneNode>)>> SceneNode::m_ComponentReflector; 
std::shared_ptr<SceneNode> SceneNode::create(std::shared_ptr<Scene> a_pRefScene, std::shared_ptr<SceneNode> a_pOwner, wxString a_Name)
{
	return std::shared_ptr<SceneNode>(new SceneNode(a_pRefScene, a_pOwner, a_Name));
}

SceneNode::SceneNode(std::shared_ptr<Scene> a_pRefScene, std::shared_ptr<SceneNode> a_pOwner, wxString a_Name)
	: m_Name(a_Name)
	, m_World(1.0f)
	, m_LocalPos(0.0f, 0.0f, 0.0f), m_LocalScale(1.0f, 1.0f, 1.0f), m_LocalRot(1.0f, 0.0f, 0.0f, 0.0f)
	, m_pRefScene(a_pRefScene)
	, m_pParentNode(a_pOwner)
	, m_pLinkedAsset(nullptr)
{
}

SceneNode::~SceneNode()
{
}

void SceneNode::setTransform(glm::vec3 a_Pos, glm::vec3 a_Scale, glm::quat a_Rot)
{
	m_LocalPos = a_Pos;
	m_LocalScale = a_Scale;
	m_LocalRot = a_Rot;
	composeTRS(m_LocalPos, m_LocalScale, m_LocalRot, m_LocalTransformCache);
	update(m_pParentNode->m_World);
}

void SceneNode::setPosition(glm::vec3 a_Pos)
{
	m_LocalPos = a_Pos;
	composeTRS(m_LocalPos, m_LocalScale, m_LocalRot, m_LocalTransformCache);
	update(m_pParentNode->m_World);
}

void SceneNode::setScale(glm::vec3 a_Scale)
{
	m_LocalScale = a_Scale;
	composeTRS(m_LocalPos, m_LocalScale, m_LocalRot, m_LocalTransformCache);
	update(m_pParentNode->m_World);
}

void SceneNode::setRotate(glm::quat a_Rot)
{
	m_LocalRot = a_Rot;
	composeTRS(m_LocalPos, m_LocalScale, m_LocalRot, m_LocalTransformCache);
	update(m_pParentNode->m_World);
}

std::shared_ptr<SceneNode> SceneNode::addChild()
{
	std::shared_ptr<SceneNode> l_pNewNode = SceneNode::create(m_pRefScene, shared_from_this());
	m_Children.push_back(l_pNewNode);
	return l_pNewNode;
}

std::shared_ptr<SceneNode> SceneNode::addChild(boost::property_tree::ptree &a_TreeNode)
{
	std::shared_ptr<SceneNode> l_pNewNode = SceneNode::create(m_pRefScene, shared_from_this());
	m_Children.push_back(l_pNewNode);

	l_pNewNode->setName(a_TreeNode.get("<xmlattr>.name", ""));
	wxString l_LinkedAsset(a_TreeNode.get("<xmlattr>.prefab", ""));
	if( !l_LinkedAsset.IsEmpty() ) m_pLinkedAsset = AssetManager::singleton().getAsset(l_LinkedAsset).second;

	glm::vec3 l_Pos, l_Scale;
	glm::quat l_Rot;
	parseShaderParamValue(ShaderParamType::float3, a_TreeNode.get_child("Position").data(), reinterpret_cast<char*>(&l_Pos));
	parseShaderParamValue(ShaderParamType::float3, a_TreeNode.get_child("Scale").data(), reinterpret_cast<char*>(&l_Scale));
	parseShaderParamValue(ShaderParamType::float4, a_TreeNode.get_child("Rotate").data(), reinterpret_cast<char*>(&l_Rot));
	l_pNewNode->setTransform(l_Pos, l_Scale, l_Rot);

	boost::property_tree::ptree &l_Components = a_TreeNode.get_child("Components");
	for( auto it=l_Components.begin() ; it!=l_Components.end() ; ++it )
	{
		if( "<xmlattr>" == it->first ) continue;

		std::shared_ptr<EngineComponent> l_pComponent = l_pNewNode->addComponent(it->first);
		l_pComponent->loadComponent(it->second);
	}

	boost::property_tree::ptree &l_Children = a_TreeNode.get_child("Children");
	for( auto it=l_Children.begin() ; it!=l_Children.end() ; ++it )
	{
		if( "<xmlattr>" == it->first ) continue;
		l_pNewNode->addChild(it->second);
	}

	return l_pNewNode;
}

std::shared_ptr<EngineComponent> SceneNode::addComponent(std::string a_Name)
{
	auto l_FuncIt = SceneNode::m_ComponentReflector.find(a_Name);
	assert(l_FuncIt != SceneNode::m_ComponentReflector.end());
	return l_FuncIt->second(shared_from_this());
}

void SceneNode::bakeNode(boost::property_tree::ptree &a_TreeNode)
{
	boost::property_tree::ptree l_SceneNodeAttr;
	l_SceneNodeAttr.add("name", m_Name.mbc_str());
	if( nullptr != m_pLinkedAsset ) l_SceneNodeAttr.add("prefab", m_pLinkedAsset->getKey().mbc_str());
	a_TreeNode.add_child("<xmlattr>", l_SceneNodeAttr);

	a_TreeNode.put("Position", convertParamValue(ShaderParamType::float3, reinterpret_cast<char*>(&m_LocalPos)));
	a_TreeNode.put("Scale", convertParamValue(ShaderParamType::float3, reinterpret_cast<char*>(&m_LocalScale)));
	a_TreeNode.put("Rotate", convertParamValue(ShaderParamType::float4, reinterpret_cast<char*>(&m_LocalRot)));

	boost::property_tree::ptree l_Components;
	for( auto it=m_Components.begin() ; it!=m_Components.end() ; ++it )
	{
		for( auto it2=it->second.begin() ; it2!=it->second.end() ; ++it2 )
		{
			(*it2)->saveComponent(l_Components);
		}
	}
	a_TreeNode.add_child("Components", l_Components);

	boost::property_tree::ptree l_Children;
	for( auto it=m_Children.begin() ; it!=m_Children.end() ; ++it ) (*it)->bakeNode(l_Children);
	a_TreeNode.add_child("Children", l_Children);
}

void SceneNode::destroy()
{
	m_pRefScene = nullptr;
	m_pParentNode = nullptr;

	m_pLinkedAsset = nullptr;
	for( auto it = m_Components.begin() ; it != m_Components.end() ; ++it )
	{
		for( auto setIt = it->second.begin() ; setIt != it->second.end() ; ++setIt )
		{
			(*setIt)->detach();
		}
		it->second.clear();
	}
	m_Components.clear();
	for( auto it=m_Children.begin() ; it!=m_Children.end() ; ++it ) (*it)->destroy();
	m_Children.clear();
}

void SceneNode::setParent(std::shared_ptr<SceneNode> a_pNewParent)
{
	if( a_pNewParent == m_pParentNode ) return;
	assert( nullptr != a_pNewParent );
	assert( nullptr != m_pParentNode );// don't do this to root node
	
	auto it = m_pParentNode->m_Children.begin();
	for( ; it != m_pParentNode->m_Children.end() ; ++it )
	{
		if( (*it).get() == this )
		{
			m_pParentNode->m_Children.erase(it);
			break;
		}
	}

	m_pParentNode = a_pNewParent;
	m_pParentNode->m_Children.push_back(shared_from_this());
	update(m_pParentNode->m_World);
}

std::shared_ptr<SceneNode> SceneNode::find(wxString a_Name)
{
	if( a_Name == m_Name ) return shared_from_this();

	std::shared_ptr<SceneNode> l_Res = nullptr;
	for( auto it = m_Children.begin() ; it != m_Children.end() ; ++it )
	{
		l_Res = (*it)->find(a_Name);
		if( nullptr != l_Res ) break;
	}

	return l_Res;
}

void SceneNode::update(glm::mat4x4 a_ParentTranform)
{
	m_World = a_ParentTranform * m_LocalTransformCache;
	for( auto it=m_TransformListener.begin() ; it!=m_TransformListener.end() ; ++it ) (*it)->transformListener(m_World);
	for( auto it=m_Children.begin() ; it!=m_Children.end() ; ++it ) (*it)->update(m_World);
}

std::shared_ptr<EngineComponent> SceneNode::getComponent(wxString a_Name)
{
	if( a_Name.empty() ) return nullptr;
	for( auto it = m_Components.begin() ; it != m_Components.end() ; ++it )
	{
		for( auto setIt = it->second.begin() ; setIt != it->second.end() ; ++setIt )
		{
			if( (*setIt)->getName() == a_Name ) return *setIt;
		}
	}
	return nullptr;
}

void SceneNode::getComponent(wxString a_Name, std::vector< std::shared_ptr<EngineComponent> > &a_Output)
{
	if( a_Name.empty() ) return;
	for( auto it = m_Components.begin() ; it != m_Components.end() ; ++it )
	{
		for( auto setIt = it->second.begin() ; setIt != it->second.end() ; ++setIt )
		{
			if( (*setIt)->getName() == a_Name ) a_Output.push_back(*setIt);
		}
	}
}

void SceneNode::add(std::shared_ptr<EngineComponent> a_pComponent)
{
	auto l_TypeIt = m_Components.find(a_pComponent->typeID());
	std::set< std::shared_ptr<EngineComponent> > &l_TargetSet = m_Components.end() == l_TypeIt ? 
		(m_Components[a_pComponent->typeID()] = std::set< std::shared_ptr<EngineComponent> >()) :
		l_TypeIt->second;
	l_TargetSet.insert(a_pComponent);
}

void SceneNode::remove(std::shared_ptr<EngineComponent> a_pComponent)
{
	auto l_TypeIt = m_Components.find(a_pComponent->typeID());
	assert(m_Components.end() != l_TypeIt);
	auto l_ComponentIt = std::find(l_TypeIt->second.begin(), l_TypeIt->second.end(), a_pComponent);
	assert(l_TypeIt->second.end() != l_ComponentIt);
	l_TypeIt->second.erase(l_ComponentIt);
	if( l_TypeIt->second.empty() ) m_Components.erase(l_TypeIt);
}

void SceneNode::addTranformListener(std::shared_ptr<EngineComponent> a_pComponent)
{
	m_TransformListener.push_back(a_pComponent);
}

void SceneNode::removeTranformListener(std::shared_ptr<EngineComponent> a_pComponent)
{
	auto it = std::find(m_TransformListener.begin(), m_TransformListener.end(), a_pComponent);
	m_TransformListener.erase(it);
}
#pragma endregion

#pragma region Scene
//
// Scene
//
unsigned int Scene::m_LightmapSerial = 0;
std::map<std::string, std::function<RenderPipeline*(boost::property_tree::ptree&, std::shared_ptr<Scene>)>> Scene::m_RenderPipeLineReflectors;
std::map<std::string, std::function<ScenePartition*(boost::property_tree::ptree&)>> Scene::m_SceneGraphReflectors;
Scene::Scene()
	: m_pRenderer(nullptr), m_pShadowMapBaker(nullptr)
	, m_pRootNode(nullptr)
	, m_pGraphs{nullptr, nullptr, nullptr, nullptr, nullptr}
	, m_pBatcher(nullptr)
	, m_pDirLights(nullptr)
	, m_pOmniLights(nullptr)
	, m_pSpotLights(nullptr)
	, m_pCurrCamera(nullptr)
	, m_pLightmap(nullptr)
	, m_bActivate(true)
{
}

Scene::~Scene()
{
	m_InputListener.clear();
	m_ReadyInputListener.clear();
	m_DroppedInputListener.clear();
}

void Scene::destroy()
{
	clear();
	delete m_pDirLights;
	delete m_pOmniLights;
	delete m_pSpotLights;
}

void Scene::initEmpty()
{
	clear();
	
	boost::property_tree::ptree l_Empty;
	for( unsigned int i=0 ; i<NUM_GRAPH_TYPE ; ++i ) m_pGraphs[i] = NoPartition::create(l_Empty);//OctreePartition();

	m_pRootNode = SceneNode::create(shared_from_this(), nullptr, wxT("Root"));
	m_pDirLights = new LightContainer<DirLight>("DirLight");
	m_pOmniLights = new LightContainer<OmniLight>("OmniLight");
	m_pSpotLights = new LightContainer<SpotLight>("SpotLight");
	m_pRenderer = DeferredRenderer::create(l_Empty, shared_from_this());
	m_pShadowMapBaker = ShadowMapRenderer::create(l_Empty, shared_from_this());

	wxString l_AssetName(wxString::Format(wxT(DEFAULT_LIGHT_MAP), m_LightmapSerial++));
	m_pLightmap = AssetManager::singleton().createAsset(l_AssetName).second;

	std::shared_ptr<SceneNode> l_pCameraNode = m_pRootNode->addChild();
	l_pCameraNode->setName(wxT("Default Camera"));
	m_pCurrCamera = l_pCameraNode->addComponent<Camera>();
	m_pCurrCamera->setName(wxT("DefaultCamera"));
}

void Scene::setup(std::shared_ptr<Asset> a_pSceneAsset)
{
	clear();

	SceneAsset *l_pAssetInst = a_pSceneAsset->getComponent<SceneAsset>();

	boost::property_tree::ptree &l_PipelineSetting = l_pAssetInst->getPipelineSetting();
	std::string l_Typename(l_PipelineSetting.get("<xmlattr>.type", ""));
	if( l_Typename.empty() ) m_pRenderer = DeferredRenderer::create(l_PipelineSetting, shared_from_this());
	else
	{
		auto it = m_RenderPipeLineReflectors.find(l_Typename);
		if( m_RenderPipeLineReflectors.end() == it ) m_pRenderer = DeferredRenderer::create(l_PipelineSetting, shared_from_this());
		else m_pRenderer = it->second(l_PipelineSetting, shared_from_this());
	}
	
	boost::property_tree::ptree &l_ShadowSetting = l_pAssetInst->getShadowSetting();
	l_Typename = l_ShadowSetting.get("<xmlattr>.type", "");
	if( l_Typename.empty() ) m_pShadowMapBaker = ShadowMapRenderer::create(l_ShadowSetting, shared_from_this());
	else
	{
		auto it = m_RenderPipeLineReflectors.find(l_Typename);
		if( m_RenderPipeLineReflectors.end() == it ) m_pShadowMapBaker = ShadowMapRenderer::create(l_ShadowSetting, shared_from_this());
		else m_pShadowMapBaker = it->second(l_ShadowSetting, shared_from_this());
	}

	std::function<ScenePartition*(boost::property_tree::ptree)> l_CreateFunc = nullptr;
	boost::property_tree::ptree &l_SceneGraphSetting = l_pAssetInst->getSceneGraphSetting();
	l_Typename = l_SceneGraphSetting.get("<xmlattr>.type", "");
	if( l_Typename.empty() ) l_CreateFunc = &NoPartition::create;
	else
	{
		auto it = m_SceneGraphReflectors.find(l_Typename);
		if( m_SceneGraphReflectors.end() == it ) l_CreateFunc = &NoPartition::create;
		else l_CreateFunc = it->second;
	}
	for( unsigned int i=0 ; i<NUM_GRAPH_TYPE ; ++i ) m_pGraphs[i] = l_CreateFunc(l_SceneGraphSetting);
	
	m_pRootNode = SceneNode::create(shared_from_this(), nullptr, wxT("Root"));
	m_pRootNode->addChild(l_pAssetInst->getNodeTree());
	
	m_pLightmap = AssetManager::singleton().getAsset(l_pAssetInst->getLightmapAssetPath()).second;
}

void Scene::preprocessInput()
{
	// add/remove listener
	std::lock_guard<std::mutex> l_Locker(m_InputLocker);
	if( !m_DroppedInputListener.empty() )
	{
		for( auto it = m_DroppedInputListener.begin() ; it != m_DroppedInputListener.end() ; ++it )
		{
			auto l_ListenerIt = std::find(m_InputListener.begin(), m_InputListener.end(), *it);
			m_InputListener.erase(l_ListenerIt);
		}
		m_DroppedInputListener.clear();
	}

	if( !m_ReadyInputListener.empty() )
	{
		for( auto it = m_ReadyInputListener.begin() ; it != m_ReadyInputListener.end() ; ++it ) m_InputListener.push_back(*it);
		m_ReadyInputListener.clear();
	}
}

void Scene::processInput(InputData &a_Data)
{
	for( auto it = m_InputListener.begin() ; it != m_InputListener.end() ; ++it )
	{
		if( (*it)->inputListener(a_Data) ) return;
	}
}

void Scene::update(float a_Delta)
{
	for( auto it = m_UpdateCallback.begin() ; it != m_UpdateCallback.end() ; ++it ) (*it)->updateListener(a_Delta);

	// flush uav resources
	//m_pMembers->m_pDirLights->flush(); flush after shadow map info updated
	//m_pMembers->m_pOmniLights->flush();
	//m_pMembers->m_pSpotLights->flush();
	//m_pMembers->m_pBatcher->flush();
}

void Scene::render(GraphicCanvas *a_pCanvas)
{
	if( nullptr == m_pCurrCamera ) return;

	//
	// to do : update shadow map, enviroment map ... etc
	//

	m_pRenderer->render(m_pCurrCamera, a_pCanvas);
	m_pBatcher->renderEnd();
}

void Scene::addUpdateListener(std::shared_ptr<EngineComponent> a_pListener)
{
	m_UpdateCallback.push_back(a_pListener);
}

void Scene::removeUpdateListener(std::shared_ptr<EngineComponent> a_pListener)
{
	auto it = std::find(m_UpdateCallback.begin(), m_UpdateCallback.end(), a_pListener);
	m_UpdateCallback.erase(it);
}

void Scene::addInputListener(std::shared_ptr<EngineComponent> a_pComponent)
{
	std::lock_guard<std::mutex> l_Locker(m_InputLocker);

	auto it = std::find(m_InputListener.begin(), m_InputListener.end(), a_pComponent);
	if( it != m_InputListener.end() ) return;
	
	auto it2 = std::find(m_ReadyInputListener.begin(), m_ReadyInputListener.end(), a_pComponent);
	if( it2 != m_ReadyInputListener.end() ) return;

	m_DroppedInputListener.erase(a_pComponent);
	m_ReadyInputListener.push_back(a_pComponent);
}

void Scene::removeInputListener(std::shared_ptr<EngineComponent> a_pComponent)
{
	std::lock_guard<std::mutex> l_Locker(m_InputLocker);
	
	auto it = std::find(m_InputListener.begin(), m_InputListener.end(), a_pComponent);
	if( it == m_InputListener.end() ) return;
	
	auto it2 = m_DroppedInputListener.find(a_pComponent);
	if( it2 == m_DroppedInputListener.end() ) return;

	it = std::find(m_ReadyInputListener.begin(), m_ReadyInputListener.end(), a_pComponent);
	if( m_ReadyInputListener.end() == it ) m_ReadyInputListener.erase(it);
	m_DroppedInputListener.insert(a_pComponent);
}

void Scene::clearInputListener()
{
	std::lock_guard<std::mutex> l_Locker(m_InputLocker);
	
	m_InputListener.clear();
	m_ReadyInputListener.clear();
	m_DroppedInputListener.clear();
}

void Scene::saveSceneGraphSetting(boost::property_tree::ptree &a_Dst)
{
	m_pGraphs[0]->saveSetting(a_Dst);
}

void Scene::saveRenderSetting(boost::property_tree::ptree &a_RenderDst, boost::property_tree::ptree &a_ShadowDst)
{
	m_pRenderer->saveSetting(a_RenderDst);
	m_pShadowMapBaker->saveSetting(a_ShadowDst);
}

std::shared_ptr<SceneNode> Scene::getRootNode()
{
	return m_pRootNode;
}

void Scene::clear()
{
	if( nullptr != m_pRootNode ) m_pRootNode->destroy();
	m_pRootNode = nullptr;
	if( nullptr != m_pGraphs[0] )
	{
		for( unsigned int i=0 ; i<NUM_GRAPH_TYPE ; ++i )
		{
			m_pGraphs[i]->clear();
			delete m_pGraphs[i];
		}
		memset(m_pGraphs, NULL, sizeof(ScenePartition *) * NUM_GRAPH_TYPE);
	}
	SAFE_DELETE(m_pRenderer)
	SAFE_DELETE(m_pShadowMapBaker)
	SAFE_DELETE(m_pBatcher)
	if( nullptr != m_pDirLights ) m_pDirLights->clear();
	if( nullptr != m_pOmniLights ) m_pOmniLights->clear();
	if( nullptr != m_pSpotLights ) m_pSpotLights->clear();
	m_pLightmap = nullptr;

	m_pCurrCamera = nullptr;
}
#pragma endregion

#pragma region SceneManager
//
// SceneManager
//
SceneManager& SceneManager::singleton()
{
	static SceneManager s_Inst;
	return s_Inst;
}

std::shared_ptr<Scene> SceneManager::create(wxString a_Name)
{
	assert(m_SceneMap.end() == m_SceneMap.find(a_Name));
	std::shared_ptr<Scene> l_pNewScene = std::shared_ptr<Scene>(new Scene());
	m_SceneMap.insert(std::make_pair(a_Name, l_pNewScene));
	return l_pNewScene;
}

SceneManager::SceneManager()
{
}

SceneManager::~SceneManager()
{
	m_CanvasMainScene.clear();
	m_SceneMap.clear();
}

void SceneManager::setMainScene(GraphicCanvas *a_pCanvas, std::shared_ptr<Scene> a_pScene)
{
	assert(nullptr != a_pScene);
	m_CanvasMainScene[a_pScene] = a_pCanvas;
}

void SceneManager::dropScene(wxString a_Name)
{
	std::lock_guard<std::mutex> l_DropLock(m_CanvasDropLock);
	auto it = m_SceneMap.find(a_Name);
	if( m_SceneMap.end() == it ) return;

	it->second->destroy();
	m_CanvasMainScene.erase(it->second);
	m_SceneMap.erase(it);
}

void SceneManager::dropCanvas(GraphicCanvas *a_pWeak)
{
	std::lock_guard<std::mutex> l_DropLock(m_CanvasDropLock);
	for( auto it = m_CanvasMainScene.begin() ; it != m_CanvasMainScene.end() ; ++it )
	{
		if( it->second == a_pWeak )
		{
			m_CanvasMainScene.erase(it);
			break;
		}
	}
}

std::shared_ptr<Scene> SceneManager::getScene(wxString a_Name)
{
	auto it = m_SceneMap.find(a_Name);
	if( m_SceneMap.end() == it ) return nullptr;
	return it->second;
}

void SceneManager::processInput(std::vector<InputData *> &a_Data)
{
	std::lock_guard<std::mutex> l_DropLock(m_CanvasDropLock);
	for( auto it = m_CanvasMainScene.begin() ; it != m_CanvasMainScene.end() ; ++it )
	{
		if( !it->second->HasFocus() || !it->first->m_bActivate ) continue;
		it->first->preprocessInput();
		for( unsigned int i=0 ; i<a_Data.size() ; ++i ) it->first->processInput(*a_Data[i]);
	}
}

void SceneManager::update(float a_Delta)
{
	std::lock_guard<std::mutex> l_DropLock(m_CanvasDropLock);
	for( auto it = m_SceneMap.begin() ; it != m_SceneMap.end() ; ++it )
	{
		if( !it->second->m_bActivate ) continue;
		it->second->update(a_Delta);
	}
}

void SceneManager::render()
{
	std::lock_guard<std::mutex> l_DropLock(m_CanvasDropLock);
	for( auto it = m_SceneMap.begin() ; it != m_SceneMap.end() ; ++it )
	{
		if( !it->second->m_bActivate ) continue;

		auto l_CanvasIt = m_CanvasMainScene.find(it->second);
		it->second->render(l_CanvasIt == m_CanvasMainScene.end() ? nullptr : l_CanvasIt->second);
	}
}

}