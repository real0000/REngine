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
class IndirectDrawBuffer;
class MaterialAsset;
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

class SceneBatcher
{
public:
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
	void drawSortedMeshes(GraphicCommander *a_pCmd
		, std::vector<RenderableMesh*> &a_SortedMesh, unsigned int a_ThreadIdx, unsigned int a_NumThread, unsigned int a_MatSlot
		, std::function<void(MaterialAsset*)> a_BindingFunc, std::function<unsigned int(std::vector<glm::ivec4>&, unsigned int)> a_InstanceFunc);
	void renderBegin();
	void renderEnd();

	// for renderable mesh
	int requestSkinSlot(std::shared_ptr<Asset> a_pAsset);
	void recycleSkinSlot(std::shared_ptr<Asset> a_pAsset);
	int requestWorldSlot(std::shared_ptr<RenderableMesh> a_pComponent);
	void recycleWorldSlot(std::shared_ptr<RenderableMesh> a_pComponent);
	void updateWorldSlot(int a_Slot, glm::mat4x4 a_Transform, int a_VtxFlag, int a_SkinOffset);

	// render required member
	std::shared_ptr<MaterialBlock> getSkinMatrixBlock(){ return m_pSingletonBatchData->m_SkinBlock; }
	std::shared_ptr<MaterialBlock> getWorldMatrixBlock(){ return m_pSingletonBatchData->m_WorldBlock; }

private:
	IndirectDrawBuffer* requestIndirectBuffer();

	std::deque<int> m_InstancVtxBuffers;
	std::vector<int> m_UsedInstancVtxBuffers;
	
	std::deque<IndirectDrawBuffer*> m_IndirectBufferPool;
	std::list<IndirectDrawBuffer*> m_IndirectBufferInUse;
	std::mutex m_BufferLock;

	bool m_bWorldDirty;
	glm::ivec2 m_UpdateRange;

	static SingletonBatchData *m_pSingletonBatchData;
	static unsigned int m_NumSharedMember;
};

class SceneNode : public std::enable_shared_from_this<SceneNode>
{
	friend class EngineComponent;
	friend class SceneNode;
public:
	virtual ~SceneNode();// don't call this method directly
	static std::shared_ptr<SceneNode> create(std::shared_ptr<Scene> a_pRefScene, std::shared_ptr<SceneNode> a_pOwner, wxString a_Name = wxT("NoName"));

	std::shared_ptr<SceneNode> addChild();
	std::shared_ptr<SceneNode> addChild(wxString a_MeshPath);
	std::shared_ptr<SceneNode> addChild(boost::property_tree::ptree &a_TreeNode);
	
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
		return EngineComponent::create<T>(m_pRefScene, shared_from_this());
	}
	template<>
	std::shared_ptr<DirLight> addComponent();
	template<>
	std::shared_ptr<OmniLight> addComponent();
	template<>
	std::shared_ptr<SpotLight> addComponent();
	std::shared_ptr<EngineComponent> addComponent(std::string a_Name);
	void bakeNode(boost::property_tree::ptree &a_TreeNode);
	
	void destroy();
	void setParent(std::shared_ptr<SceneNode> a_pNewParent);
	std::shared_ptr<SceneNode> find(wxString a_Name);

	void setTransform(glm::mat4x4 a_Trans);
	void setTransform(glm::vec3 a_Pos, glm::vec3 a_Scale, glm::quat a_Rot);
	void setPosition(glm::vec3 a_Pos);
	void setScale(glm::vec3 a_Scale);
	void setRotate(glm::quat a_Rot);
	void update(glm::mat4x4 a_ParentTranform);

	wxString getName(){ return m_Name; }
	void setName(wxString a_Name){ m_Name = a_Name; }
	const std::list<std::shared_ptr<SceneNode>>& getChildren(){ return m_Children; }
	const std::shared_ptr<SceneNode> getParent(){ return m_pParentNode; }
	const std::shared_ptr<Scene> getScene(){ return m_pRefScene; }
	const glm::mat4x4& getTransform(){ return m_World; }
	const glm::vec3& getLocalPos(){ return m_LocalPos; }
	const glm::vec3& getLocalScale(){ return m_LocalScale; }
	const glm::quat& getLocalRotate(){ return m_LocalRot; }

	std::shared_ptr<EngineComponent> getComponent(wxString a_Name);
	void getComponent(wxString a_Name, std::vector<std::shared_ptr<EngineComponent>> &a_Output);
	template<typename T>
	void getComponent(std::vector<std::shared_ptr<T>> &a_Output)
	{
		auto it = m_Components.find(T::staticTypeID());
		if( m_Components.end() == it || it->second.empty() ) return;
		a_Output.resize(it->second.size());
		std::transform(it->second.begin(), it->second.end(), a_Output.begin()
			, [=](std::shared_ptr<EngineComponent> a_pComponent){ return reinterpret_cast<std::shared_ptr<T>&>(a_pComponent); });
	}
	const std::map<unsigned int, std::set<std::shared_ptr<EngineComponent>>>& getComponents(){ return m_Components; }
	
private:
	SceneNode(std::shared_ptr<Scene> a_pRefScene, std::shared_ptr<SceneNode> a_pOwner, wxString a_Name);

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
	std::shared_ptr<Scene> m_pRefScene;
	std::shared_ptr<SceneNode> m_pParentNode;

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

	// setup part
	void initEmpty();
	void setup(std::shared_ptr<Asset> a_pSceneAsset);

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

	// misc
	void saveSceneGraphSetting(boost::property_tree::ptree &a_Dst);
	void saveRenderSetting(boost::property_tree::ptree &a_Dst, boost::property_tree::ptree &a_ShadowDst);
	RenderPipeline* getShadowMapBaker(){ return m_pShadowMapBaker; }
	std::shared_ptr<Asset> getLightmap(){ return m_pLightmap; }
	LightContainer<DirLight>* getDirLightContainer(){ return m_pDirLights; }
	LightContainer<OmniLight>* getOmniLightContainer(){ return m_pOmniLights; }
	LightContainer<SpotLight>* getSpotLightContainer(){ return m_pSpotLights; }
	
	template<typename T>
	static void registSceneGraphReflector()
	{
		std::string l_TypeName(T::typeName());
		assert(m_SceneGraphReflectors.end() == m_SceneGraphReflectors.find(l_TypeName));
		m_SceneGraphReflectors.insert(std::make_pair(l_TypeName, [=](boost::property_tree::ptree &a_Src) -> ScenePartition*
		{
			return T::create(a_Src);
		}));
	}
	template<typename T>
	static void registRenderPipelineReflector()
	{
		std::string l_TypeName(T::typeName());
		assert(m_RenderPipeLineReflectors.end() == m_RenderPipeLineReflectors.find(l_TypeName));
		m_RenderPipeLineReflectors.insert(std::make_pair(l_TypeName, [=](boost::property_tree::ptree &a_Src, std::shared_ptr<Scene> a_pScene) -> RenderPipeline*
		{
			return T::create(a_Src, a_pScene);
		}));
	}
	ScenePartition* getSceneGraph(SceneGraphType a_Slot){ return m_pGraphs[a_Slot]; }
	SceneBatcher* getRenderBatcher(){ return m_pBatcher; }

	std::shared_ptr<SceneNode> getRootNode();
	void pause(){ m_bActivate = false; }
	void resume(){ m_bActivate = true; }

private:
	Scene();

	void clear();
	
	// reference render data
	RenderPipeline *m_pRenderer, *m_pShadowMapBaker;
	std::shared_ptr<SceneNode> m_pRootNode;
	ScenePartition *m_pGraphs[NUM_GRAPH_TYPE];
	SceneBatcher *m_pBatcher;
	LightContainer<DirLight> *m_pDirLights;
	LightContainer<OmniLight> *m_pOmniLights;
	LightContainer<SpotLight> *m_pSpotLights;
	std::shared_ptr<Camera> m_pCurrCamera;
	std::shared_ptr<Asset> m_pLightmap;

	std::mutex m_InputLocker;
	std::list<std::shared_ptr<EngineComponent>> m_InputListener, m_ReadyInputListener;
	std::set<std::shared_ptr<EngineComponent>> m_DroppedInputListener;
	std::list<std::shared_ptr<EngineComponent>> m_UpdateCallback;
	bool m_bActivate;
	
	static unsigned int m_LightmapSerial;
	static std::map<std::string, std::function<RenderPipeline*(boost::property_tree::ptree&, std::shared_ptr<Scene>)>> m_RenderPipeLineReflectors;
	static std::map<std::string, std::function<ScenePartition*(boost::property_tree::ptree&)>> m_SceneGraphReflectors;
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