// RenderObject.h
//
// 2017/07/19 Ian Wu/Real0000
//

#ifndef _RENDER_OBJECT_H_
#define _RENDER_OBJECT_H_

namespace R
{
	
struct SharedSceneMember;
class IndexBuffer;
class Material;
class ModelData;
class RenderableMesh;
class SceneNode;
class VertexBuffer;

class RenderableComponent : public EngineComponent
{
	friend class RenderableComponent;
public:
	RenderableComponent(SharedSceneMember *a_pSharedMember, std::shared_ptr<SceneNode> a_pOwner);
	virtual ~RenderableComponent(); // don't call this method directly
	
	virtual void setShadowed(bool a_bShadow) = 0;
	virtual bool getShadowed() = 0;
	glm::daabb& boundingBox(){ return m_BoundingBox; }
	
private:
	glm::daabb m_BoundingBox;
};

class ModelComponentFactory
{
private:
	struct Instance
	{
		std::shared_ptr<VertexBuffer> m_pVtxBuffer;
		std::shared_ptr<IndexBuffer> m_pIdxBuffer;
		std::vector< std::pair<int, int> > m_Meshes;// mesh name : {(index count : base vertex)...}
	};
public:
	ModelComponentFactory(SharedSceneMember *a_pSharedMember);
	virtual ~ModelComponentFactory();

	void createMesh(std::shared_ptr<SceneNode> a_pOwner, wxString a_Filename, std::shared_ptr<Material> a_pMaterial, std::function<void()> a_pCallback);// model file
	void createMesh(std::shared_ptr<SceneNode> a_pOwner, wxString a_Filename, std::function<void()> a_pCallback);// settings file
	std::shared_ptr<RenderableMesh> createSphere(std::shared_ptr<SceneNode> a_pOwner, std::shared_ptr<Material> a_pMaterial);
	std::shared_ptr<RenderableMesh> createBox(std::shared_ptr<SceneNode> a_pOwner, std::shared_ptr<Material> a_pMaterial);
	//std::shared_ptr<EngineComponent> createVoxelTerrain(std::shared_ptr<SceneNode> a_pOwner, glm::ivec3 a_Size);

	void clearCache();

private:
	void loadMesh(std::shared_ptr<SceneNode> a_pOwner, wxString a_Filename, std::shared_ptr<Material> a_pMaterial, std::function<void()> a_pCallback);
	void loadSetting(std::shared_ptr<SceneNode> a_pOwner, wxString a_Filename, std::function<void()> a_pCallback);
	std::shared_ptr<Instance> getInstance(wxString a_Filename, std::shared_ptr<ModelData> a_pSrc, bool &a_bNeedInitInstance);
	void initMeshes(std::shared_ptr<Instance> a_pInst, std::shared_ptr<ModelData> a_pSrc);
	void initNodes(std::shared_ptr<SceneNode> a_pOwner, std::shared_ptr<Instance> a_pInst, std::shared_ptr<ModelData> a_pSrc, std::list<std::shared_ptr<RenderableMesh> > &a_OutputMeshComponent);

	SharedSceneMember *m_pSharedMember;
	std::map<wxString, std::shared_ptr<Instance> > m_FileCache;
	std::mutex m_CacheLock;
};

}

#endif