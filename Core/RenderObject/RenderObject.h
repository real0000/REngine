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
class SceneNode;
class VertexBuffer;

class RenderableComponent : public EngineComponent
{
	friend class ModelFactory;
	friend class RenderableComponent;
public:
	virtual ~RenderableComponent(); // don't call this method directly

	std::shared_ptr<VertexBuffer> getVertexBuffer(){ return m_pRefVtxBuffer; }
	std::shared_ptr<IndexBuffer> getIndexBuffer(){ return m_pRefIdxBuffer; }
	std::pair<int, int> getIndexRange(){ return m_Range; }
	glm::daabb& boundingBox(){ return m_BoundingBox; }
	
private:
	RenderableComponent(SharedSceneMember *a_pSharedMember, std::shared_ptr<SceneNode> a_pOwner);

	std::shared_ptr<VertexBuffer> m_pRefVtxBuffer;
	std::shared_ptr<IndexBuffer> m_pRefIdxBuffer;
	std::pair<int, int> m_Range;// index offset : count
	glm::daabb m_BoundingBox;
	std::map<unsigned int, std::set< std::shared_ptr<Material> > > m_MaterialSet;// render group id : [material ...]
};

class RenderableComponentFactory
{
public:
	static ModelFactory& singleton();

	std::shared_ptr<EngineComponent> createMesh(std::shared_ptr<SceneNode> a_pOwner, wxString a_Filename, std::shared_ptr<Material> a_pMateral, bool a_bAsync);
	std::shared_ptr<EngineComponent> createSphere(std::shared_ptr<SceneNode> a_pOwner);
	std::shared_ptr<EngineComponent> createBox(std::shared_ptr<SceneNode> a_pOwner);
	//std::shared_ptr<EngineComponent> createVoxelTerrain(std::shared_ptr<SceneNode> a_pOwner, glm::ivec3 a_Size);

	void clearCache();

private:
	RenderableComponentFactory();
	virtual ~RenderableComponentFactory();

	struct Instance
	{
		std::shared_ptr<VertexBuffer> m_pVtxBuffer;
		std::shared_ptr<IndexBuffer> m_pIdxBuffer;
		std::map<wxString, std::map<int, int> > m_Meshes;
	};
	std::map<wxString, std::shared_ptr<Instance> > m_FileCache;

};

}

#endif