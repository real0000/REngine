// Scene.cpp
//
// 2015/02/10 Ian Wu/Real0000
//

#include "CommonUtil.h"
#include "Core.h"
#include "Scene.h"

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
	: m_pRenderer(nullptr)
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
	m_pRenderer = a_Src.m_pRenderer;
	m_pScene = a_Src.m_pScene;
	m_pSceneNode = a_Src.m_pSceneNode;
	return *this;
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
	, m_bStatic(false)
	, m_pMembers(new SharedSceneMember)
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

void SceneNode::setStatic(bool a_bStatic)
{
	if( a_bStatic == m_bStatic ) return;
	m_bStatic = a_bStatic;
	for( auto it = m_Components.begin() ; it != m_Components.end() ; ++it )
	{
		for( std::shared_ptr<EngineComponent> l_pComponent : it->second ) l_pComponent->staticFlagChanged();
	}
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
std::shared_ptr<Scene> Scene::create()
{
	return std::shared_ptr<Scene>(new Scene());
}

Scene::Scene()
	: m_bLoading(false), m_LoadingProgress(0.0f), m_LoadingCompleteCallback(nullptr)
	, m_pMembers(new SharedSceneMember)
	, m_pCurrCamera(nullptr)
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
	SAFE_DELETE(m_pMembers)
}

void Scene::initEmpty()
{
	assert(!m_bLoading);
	m_bLoading = true;
	clear();
	
	m_pMembers->m_pScene = shared_from_this();
	for( unsigned int i=0 ; i<SharedSceneMember::NUM_GRAPH_TYPE ; ++i ) m_pMembers->m_pGraphs[i] = new OctreePartition();
	m_pMembers->m_pRenderer = new DeferredRenderer(m_pMembers);
	m_pMembers->m_pSceneNode = SceneNode::create(m_pMembers, nullptr, wxT("Root"));

	m_pCurrCamera = CameraComponent::create(m_pMembers, m_pMembers->m_pSceneNode);
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

void Scene::processInput(InputData &a_Data)
{
	{// do add/remove listener
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
}

void Scene::update(float a_Delta)
{
	for( auto it = m_UpdateCallback.begin() ; it != m_UpdateCallback.end() ; ++it ) (*it)->updateListener(a_Delta);
}

void Scene::render()
{
	if( nullptr == m_pCurrCamera ) return;
	m_pMembers->m_pRenderer->render();
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

void Scene::addRenderStage(unsigned int a_Stage)
{
	// maybe add mutex lock ?
	m_RenderStages.insert(a_Stage);
}

void Scene::removeRenderStage(unsigned int a_Stage)
{
	m_RenderStages.erase(a_Stage);
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
#pragma endregion

}