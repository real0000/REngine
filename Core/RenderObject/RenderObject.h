// RenderObject.h
//
// 2017/07/19 Ian Wu/Real0000
//

#ifndef _RENDER_OBJECT_H_
#define _RENDER_OBJECT_H_

namespace R
{
	
struct IndirectDrawData;
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

class ModelCache
{
public:
	struct Instance
	{
		std::shared_ptr<ModelData> m_pModel;
		std::shared_ptr<VertexBuffer> m_pVtxBuffer;
		std::shared_ptr<IndexBuffer> m_pIdxBuffer;
		std::map<wxString, IndirectDrawData> m_SubMeshes;
	};
public:
	ModelCache();
	virtual ~ModelCache();
	
	std::shared_ptr<Instance> loadMesh(wxString a_Filename);

	void clearCache();

private:
	std::shared_ptr<Instance> getInstance(wxString a_Filename, std::shared_ptr<ModelData> a_pSrc, bool &a_bNeedInitInstance);
	void initMeshes(std::shared_ptr<Instance> a_pInst, std::shared_ptr<ModelData> a_pSrc);

	std::map<wxString, std::shared_ptr<Instance> > m_FileCache;
	std::mutex m_CacheLock;
};

}

#endif