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
#include "Scene/Camera.h"
#include "Scene/Graph/NoPartition.h"
#include "Scene/Graph/Octree.h"
#include "Scene/RenderPipline/Deferred.h"

namespace R
{

#pragma region SharedSceneMember
//
// SharedSceneMember
//
SharedSceneMember::SharedSceneMember()
	: m_pDirLights(nullptr), m_pOmniLights(nullptr), m_pSpotLights(nullptr)
	, m_pScene(nullptr)
	, m_pSceneNode(nullptr)
{
	memset(m_pGraphs, NULL, sizeof(ScenePartition *) * NUM_GRAPH_TYPE);
}

SharedSceneMember::~SharedSceneMember()
{
	memset(m_pGraphs, NULL, sizeof(ScenePartition *) * NUM_GRAPH_TYPE);
	m_pScene = nullptr;
	m_pSceneNode = nullptr;
}

SharedSceneMember& SharedSceneMember::operator=(const SharedSceneMember &a_Src)
{
	memcpy(m_pGraphs, a_Src.m_pGraphs, sizeof(ScenePartition *) * NUM_GRAPH_TYPE);
	m_pBatcher = a_Src.m_pBatcher;
	m_pDirLights = a_Src.m_pDirLights;
	m_pOmniLights = a_Src.m_pOmniLights;
	m_pSpotLights = a_Src.m_pSpotLights;
	m_pScene = a_Src.m_pScene;
	m_pSceneNode = a_Src.m_pSceneNode;
	return *this;
}
#pragma endregion

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

void SceneBatcher::calculateStaticBatch(SharedSceneMember *a_pGraphOwner)
{
	m_VertexBufffer = nullptr;
	m_IndexBuffer = nullptr;

	for( auto it=m_StaticMeshes.begin() ; it!=m_StaticMeshes.end() ; ++it ) delete it->second;
	m_StaticMeshes.clear();

	std::vector<std::shared_ptr<RenderableComponent>> l_Meshes;
	a_pGraphOwner->m_pGraphs[SharedSceneMember::GRAPH_STATIC_MESH]->getAllComponent(l_Meshes);

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
std::shared_ptr<SceneNode> SceneNode::create(SharedSceneMember *a_pSharedMember, std::shared_ptr<SceneNode> a_pOwner, wxString a_Name)
{
	return std::shared_ptr<SceneNode>(new SceneNode(a_pSharedMember, a_pOwner, a_Name));
}

SceneNode::SceneNode(SharedSceneMember *a_pSharedMember, std::shared_ptr<SceneNode> a_pOwner, wxString a_Name)
	: m_Name(a_Name)
	, m_World(1.0f)
	, m_LocalTransform(1.0f)
	, m_pMembers(new SharedSceneMember)
	, m_pLinkedAsset(nullptr)
{
	*m_pMembers = *a_pSharedMember;
	m_pMembers->m_pSceneNode = a_pOwner;
}

SceneNode::~SceneNode()
{
}

void SceneNode::setTransform(glm::mat4x4 a_Transform)
{
	m_LocalTransform = a_Transform;
	update(m_pMembers->m_pSceneNode->m_World);
}

std::shared_ptr<SceneNode> SceneNode::addChild()
{
	std::shared_ptr<SceneNode> l_pNewNode = SceneNode::create(m_pMembers, shared_from_this());
	m_Children.push_back(l_pNewNode);
	return l_pNewNode;
}

void SceneNode::destroy()
{
	SAFE_DELETE(m_pMembers)
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
	if( a_pNewParent == m_pMembers->m_pSceneNode ) return;
	assert( nullptr != a_pNewParent );
	assert( nullptr != m_pMembers->m_pSceneNode );// don't do this to root node
	
	auto it = m_pMembers->m_pSceneNode->m_Children.begin();
	for( ; it != m_pMembers->m_pSceneNode->m_Children.end() ; ++it )
	{
		if( (*it).get() == this )
		{
			m_pMembers->m_pSceneNode->m_Children.erase(it);
			break;
		}
	}

	m_pMembers->m_pSceneNode = a_pNewParent;
	m_pMembers->m_pSceneNode->m_Children.push_back(shared_from_this());
	update(m_pMembers->m_pSceneNode->m_World);
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
	m_World = a_ParentTranform * m_LocalTransform;
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
Scene::Scene()
	: m_bLoading(false), m_LoadingProgress(0.0f), m_LoadingCompleteCallback(nullptr)
	, m_pRenderer(nullptr)
	, m_pMembers(new SharedSceneMember)
	, m_pCurrCamera(nullptr)
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
	delete m_pMembers->m_pDirLights;
	delete m_pMembers->m_pOmniLights;
	delete m_pMembers->m_pSpotLights;
	SAFE_DELETE(m_pMembers)
}

void Scene::initEmpty()
{
	assert(!m_bLoading);
	m_bLoading = true;
	clear();
	
	m_pMembers->m_pScene = shared_from_this();
	for( unsigned int i=0 ; i<SharedSceneMember::NUM_GRAPH_TYPE ; ++i ) m_pMembers->m_pGraphs[i] = new NoPartition();//OctreePartition();
	m_pMembers->m_pSceneNode = SceneNode::create(m_pMembers, nullptr, wxT("Root"));
	m_pMembers->m_pDirLights = new LightContainer<DirLight>("DirLight");
	m_pMembers->m_pOmniLights = new LightContainer<OmniLight>("OmniLight");
	m_pMembers->m_pSpotLights = new LightContainer<SpotLight>("SpotLight");
	m_pRenderer = new DeferredRenderer(m_pMembers);

	std::shared_ptr<SceneNode> l_pCameraNode = m_pMembers->m_pSceneNode->addChild();
	l_pCameraNode->setName(wxT("Default Camera"));
	m_pCurrCamera = l_pCameraNode->addComponent<CameraComponent>();
	m_pCurrCamera->setName(wxT("DefaultCamera"));

	m_bLoading = false;
}

bool Scene::load(wxString a_Path)
{
	try
	{
		assert(!m_bLoading);
		m_bLoading = true;
		clear();

		boost::property_tree::ptree l_XMLTree;
		boost::property_tree::xml_parser::read_xml(static_cast<const char *>(a_Path.c_str()), l_XMLTree);
		assert( !l_XMLTree.empty() );

		loadScenePartitionSetting(l_XMLTree.get_child("root.ScenePartition"));
		loadRenderPipelineSetting(l_XMLTree.get_child("root.RenderPipeline"));
		loadAssetSetting(l_XMLTree.get_child("root.Assets"));

		m_pMembers->m_pScene = shared_from_this();
	}
	catch( ... )
	{
		m_bLoading = false;
		return false;
	}

	m_bLoading = false;
	return true;
}

void Scene::loadAsync(wxString a_Path, std::function<void(bool)> a_Callback)
{
	m_LoadingCompleteCallback = a_Callback;
	std::thread l_NewThread(&Scene::loadAsyncThread, this, a_Path);
}

bool Scene::save(wxString a_Path)
{
	return true;
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
	m_pMembers->m_pBatcher->renderEnd();
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

std::shared_ptr<SceneNode> Scene::getRootNode()
{
	return m_pMembers->m_pSceneNode;
}

void Scene::clear()
{
	m_LoadingProgress = 0.0f;
	
	if( nullptr != m_pMembers->m_pSceneNode ) m_pMembers->m_pSceneNode->destroy();
	m_pMembers->m_pSceneNode = nullptr;
	if( nullptr != m_pMembers->m_pGraphs[0] )
	{
		for( unsigned int i=0 ; i<SharedSceneMember::NUM_GRAPH_TYPE ; ++i )
		{
			m_pMembers->m_pGraphs[i]->clear();
			delete m_pMembers->m_pGraphs[i];
		}
		memset(m_pMembers->m_pGraphs, NULL, sizeof(ScenePartition *) * SharedSceneMember::NUM_GRAPH_TYPE);
	}
	SAFE_DELETE(m_pRenderer)
	SAFE_DELETE(m_pMembers->m_pBatcher)
	if( nullptr != m_pMembers->m_pDirLights ) m_pMembers->m_pDirLights->clear();
	if( nullptr != m_pMembers->m_pOmniLights ) m_pMembers->m_pOmniLights->clear();
	if( nullptr != m_pMembers->m_pSpotLights ) m_pMembers->m_pSpotLights->clear();
	for( auto it=m_SceneAssets.begin() ; it!=m_SceneAssets.end() ; ++it ) delete it->second;
	m_SceneAssets.clear();

	m_pCurrCamera = nullptr;
}

void Scene::loadAsyncThread(wxString a_Path)
{
	bool l_bSuccess = load(a_Path);
	m_LoadingCompleteCallback(l_bSuccess);
}

void Scene::loadScenePartitionSetting(boost::property_tree::ptree &a_Node)
{
	boost::property_tree::ptree &l_Attr = a_Node.get_child("<xmlattr>");
	switch( ScenePartitionType::fromString(l_Attr.get("type", "Octree")) )
	{
		case ScenePartitionType::None:{
			for( unsigned int i=0 ; i<SharedSceneMember::NUM_GRAPH_TYPE ; ++i ) m_pMembers->m_pGraphs[i] = new NoPartition();
			}break;

		case ScenePartitionType::Octree:{
			double l_RootEdge = a_Node.get("RootEdge", DEFAULT_OCTREE_ROOT_SIZE);
			double l_MinEdge = a_Node.get("MinEdge", DEFAULT_OCTREE_EDGE);
			for( unsigned int i=0 ; i<SharedSceneMember::NUM_GRAPH_TYPE ; ++i ) m_pMembers->m_pGraphs[i] = new OctreePartition(l_RootEdge, l_MinEdge);
			}break;
	}
}

void Scene::loadRenderPipelineSetting(boost::property_tree::ptree &a_Node)
{
}

void Scene::loadAssetSetting(boost::property_tree::ptree &a_Node)
{

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