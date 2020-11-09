// Scene.h
//
// 2015/02/10 Ian Wu/Real0000
//

#ifndef _SCENE_H_
#define _SCENE_H_

namespace R
{

struct InputData;
struct SharedBatchData;
class Camera;
class DirLight;
class EngineComponent;
class GraphicCommander;
class IndexBuffer;
class MaterialBlock;
class OmniLight;
class RenderableMesh;
class RenderPipeline;
class Scene;
class SceneBatcher;
class SceneNode;
class ScenePartition;
class SpotLight;
class VertexBuffer;

template<typename T>
class LightContainer;

struct SharedSceneMember
{
	enum
	{
		GRAPH_MESH = 0,
		GRAPH_STATIC_MESH,
		GRAPH_LIGHT,
		GRAPH_STATIC_LIGHT,
		GRAPH_CAMERA,

		NUM_GRAPH_TYPE
	};

	SharedSceneMember();
	~SharedSceneMember();

	SharedSceneMember& operator=(const SharedSceneMember &a_Src);
	
	ScenePartition *m_pGraphs[NUM_GRAPH_TYPE];
	SceneBatcher *m_pBatcher;
	LightContainer<DirLight> *m_pDirLights;
	LightContainer<OmniLight> *m_pOmniLights;
	LightContainer<SpotLight> *m_pSpotLights;
	std::shared_ptr<Scene> m_pScene;
	std::shared_ptr<SceneNode> m_pSceneNode;
};

class SceneBatcher
{
public:
	struct MeshVtxCache
	{
		unsigned int m_VertexStart;
		unsigned int m_IndexStart;
		unsigned int m_IndexCount;
	};
	typedef std::vector<MeshVtxCache> MeshCache;
	typedef std::pair<int, unsigned int> TransitionCache;
	struct SingletonBatchData
	{
		SingletonBatchData();
		~SingletonBatchData();

		std::shared_ptr<MaterialBlock> m_SkinBlock, m_WorldBlock;
		VirtualMemoryPool m_SkinSlotManager, m_WorldSlotManager;
		std::map<std::shared_ptr<Asset>, TransitionCache> m_SkinCache;// origin mesh asset : (offset, ref count)
		std::map<std::shared_ptr<RenderableMesh>, TransitionCache> m_WorldCache;
		std::mutex m_InstanceVtxLock, m_SkinSlotLock, m_WorldSlotLock;
	};
public:
	SceneBatcher();
	virtual ~SceneBatcher();

	// for render pipeline
	int requestInstanceVtxBuffer();
	void recycleInstanceVtxBuffer(int a_BufferID);
	void renderEnd();

	// for renderable mesh
	int requestSkinSlot(std::shared_ptr<Asset> a_pAsset);
	void recycleSkinSlot(std::shared_ptr<Asset> a_pAsset);
	int requestWorldSlot(std::shared_ptr<RenderableMesh> a_pComponent);
	void recycleWorldSlot(std::shared_ptr<RenderableMesh> a_pComponent);
	void calculateStaticBatch(SharedSceneMember *a_pGraphOwner);
	bool getBatchParam(std::shared_ptr<Asset> a_pMeshAsset, MeshCache **a_ppOutput);
	void bindBatchDrawData(GraphicCommander *a_pCommander);

	// render required member
	std::shared_ptr<VertexBuffer> getVertexBuffer(){ return m_VertexBufffer; }
	std::shared_ptr<IndexBuffer> getIndexBuffer(){ return m_IndexBuffer; }
	std::shared_ptr<MaterialBlock> getSkinMatrixBlock(){ return m_pSingletonBatchData->m_SkinBlock; }
	std::shared_ptr<MaterialBlock> getWorldMatrixBlock(){ return m_pSingletonBatchData->m_WorldBlock; }

private:
	std::deque<int> m_InstancVtxBuffers;
	std::vector<int> m_UsedInstancVtxBuffers;

	std::shared_ptr<VertexBuffer> m_VertexBufffer;// for static objects
	std::shared_ptr<IndexBuffer> m_IndexBuffer;
	std::map<std::shared_ptr<Asset>, MeshCache*> m_StaticMeshes;// origin mesh asset : cache data

	static SingletonBatchData *m_pSingletonBatchData;
	static unsigned int m_NumSharedMember;
};

class SceneNode : public std::enable_shared_from_this<SceneNode>
{
	friend class EngineComponent;
	friend class SceneNode;
public:
	virtual ~SceneNode();// don't call this method directly
	static std::shared_ptr<SceneNode> create(SharedSceneMember *a_pSharedMember, std::shared_ptr<SceneNode> a_pOwner, wxString a_Name = wxT("NoName"));

	std::shared_ptr<SceneNode> addChild();
	void addChild(boost::property_tree::ptree &a_pTreeNode);
	
	template<typename T>
	static void registComponentReflector()
	{
		std::string l_Name(T::typeName());
		assert(m_ComponentReflector.end() == m_ComponentReflector.find(l_Name));
		m_ComponentReflector.insert(std::make_pair(l_Name, [=](std::shared_ptr<SceneNode> a_pOwner) -> std::shared_ptr<EngineComponent>
		{
			return a_pOwner->addComponent<T>();
		}));
	}
	template<typename T>
	std::shared_ptr<T> addComponent()
	{
		return EngineComponent::create<T>(m_pMembers, shared_from_this());
	}
	std::shared_ptr<EngineComponent> addComponent(std::string a_Name);
	void bakeNode(boost::property_tree::ptree &a_pTreeNode);
	
	void destroy();
	void setParent(std::shared_ptr<SceneNode> a_pNewParent);
	std::shared_ptr<SceneNode> find(wxString a_Name);

	void setTransform(glm::vec3 a_Pos, glm::vec3 a_Scale, glm::quat a_Rot);
	void setPosition(glm::vec3 a_Pos);
	void setScale(glm::vec3 a_Scale);
	void setRotate(glm::quat a_Rot);
	void update(glm::mat4x4 a_ParentTranform);

	wxString getName(){ return m_Name; }
	void setName(wxString a_Name){ m_Name = a_Name; }
	const std::list<std::shared_ptr<SceneNode>>& getChildren(){ return m_Children; }
	const std::shared_ptr<SceneNode> getParent(){ return m_pMembers->m_pSceneNode; }
	const glm::mat4x4& getTransform(){ return m_World; }
	const glm::vec3& getLocalPos(){ return m_LocalPos; }
	const glm::vec3& getLocalScale(){ return m_LocalScale; }
	const glm::quat& getLocalRotate(){ return m_LocalRot; }

	std::shared_ptr<EngineComponent> getComponent(wxString a_Name);
	void getComponent(wxString a_Name, std::vector<std::shared_ptr<EngineComponent>> &a_Output);
	template<typename T>
	void getComponent(unsigned int a_TypeID, std::vector<std::shared_ptr<T>> &a_Output)
	{
		auto it = m_Components.find(a_TypeID);
		if( m_Components.end() == it || it->second.empty() ) return;
		a_Output.resize(it->second.size());
		std::transform(it->second.begin(), it->second.end(), a_Output.begin()
			, [=](std::shared_ptr<EngineComponent> a_pComponent){ return reinterpret_cast<std::shared_ptr<T>&>(a_pComponent); });
	}
	const std::map<unsigned int, std::set<std::shared_ptr<EngineComponent>>>& getComponents(){ return m_Components; }
	
private:
	SceneNode(SharedSceneMember *a_pSharedMember, std::shared_ptr<SceneNode> a_pOwner, wxString a_Name);

	void add(std::shared_ptr<EngineComponent> a_pComponent);
	void remove(std::shared_ptr<EngineComponent> a_pComponent);
	void addTranformListener(std::shared_ptr<EngineComponent> a_pComponent);
	void removeTranformListener(std::shared_ptr<EngineComponent> a_pComponent);

	wxString m_Name;
	glm::mat4x4 m_World;
	glm::vec3 m_LocalPos, m_LocalScale;
	glm::quat m_LocalRot;
	glm::mat4x4 m_LocalTransformCache;

	std::list<std::shared_ptr<SceneNode>> m_Children;
	SharedSceneMember *m_pMembers;// scene node -> parent
	std::shared_ptr<Asset> m_pLinkedAsset;//must be PrefabAsset

	std::list<std::shared_ptr<EngineComponent>> m_TransformListener;// use std::set instead ?
	std::map<unsigned int, std::set<std::shared_ptr<EngineComponent>>> m_Components;

	static std::map<std::string, std::function<std::shared_ptr<EngineComponent>(std::shared_ptr<SceneNode>)>> m_ComponentReflector; 
};

class Scene : public std::enable_shared_from_this<Scene>
{
	friend class Scene;
	friend class SceneManager;
public:
	virtual ~Scene();

	void destroy();

	// file part
	void initEmpty();
	bool load(wxString a_Path);
	void loadAsync(wxString a_Path, std::function<void(bool)> a_Callback);
	bool save(wxString a_Path);
	float getLoadingProgress(){ return m_LoadingProgress; }

	// update part
	void preprocessInput();
	void processInput(InputData &a_Data);
	void update(float a_Delta);
	void render(GraphicCanvas *a_pCanvas);
	
	// listener
	void addUpdateListener(std::shared_ptr<EngineComponent> a_pListener);
	void removeUpdateListener(std::shared_ptr<EngineComponent> a_pListener);
	void addInputListener(std::shared_ptr<EngineComponent> a_pComponent);
	void removeInputListener(std::shared_ptr<EngineComponent> a_pComponent);
	void clearInputListener();

	// misc;
	std::shared_ptr<SceneNode> getRootNode();
	void pause(){ m_bActivate = false; }
	void resume(){ m_bActivate = true; }

private:
	struct SceneAssetInstance
	{
		SceneAssetInstance()
			: m_pSceneAsset(nullptr)
			, m_pRefSceneNode(nullptr)
			, m_bActive(false)
			{}
		virtual ~SceneAssetInstance()
		{
			m_pSceneAsset = nullptr;
			m_pRefSceneNode = nullptr;
		}

		std::shared_ptr<Asset> m_pSceneAsset;
		std::shared_ptr<SceneNode> m_pRefSceneNode;
		bool m_bActive;
	};

	Scene();

	void clear();
	void loadAsyncThread(wxString a_Path);
	void loadScenePartitionSetting(boost::property_tree::ptree &a_Node);
	void loadRenderPipelineSetting(boost::property_tree::ptree &a_Node);
	void loadAssetSetting(boost::property_tree::ptree &a_Node);

	bool m_bLoading;
	float m_LoadingProgress;
	std::function<void(bool)> m_LoadingCompleteCallback;
	
	RenderPipeline *m_pRenderer;
	SharedSceneMember *m_pMembers;
	std::shared_ptr<Camera> m_pCurrCamera;
	std::map<wxString, SceneAssetInstance*> m_SceneAssets;
	
	std::mutex m_InputLocker;
	std::list<std::shared_ptr<EngineComponent>> m_InputListener, m_ReadyInputListener;
	std::set<std::shared_ptr<EngineComponent>> m_DroppedInputListener;
	std::list<std::shared_ptr<EngineComponent>> m_UpdateCallback;
	bool m_bActivate;
};

class SceneManager
{
public:
	static SceneManager& singleton();
	std::shared_ptr<Scene> create(wxString a_Name);

	void setMainScene(GraphicCanvas *a_pCanvas, std::shared_ptr<Scene> a_pScene);
	void dropScene(wxString a_Name);
	void dropCanvas(GraphicCanvas *a_pWeak);
	std::shared_ptr<Scene> getScene(wxString a_Name);

	// focused main scene only
	void processInput(std::vector<InputData *> &a_Data);
	void update(float a_Delta);
	void render();

private:
	SceneManager();
	virtual ~SceneManager();

	std::map<std::shared_ptr<Scene>, GraphicCanvas*> m_CanvasMainScene;
	std::map<wxString, std::shared_ptr<Scene> > m_SceneMap;
	std::mutex m_CanvasDropLock;
};

}
#endif