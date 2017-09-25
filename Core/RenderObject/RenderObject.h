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
class SceneNode;
class VertexBuffer;

class RenderableComponent : public EngineComponent
{
	friend class RenderableComponent;
public:
	RenderableComponent(SharedSceneMember *a_pSharedMember, std::shared_ptr<SceneNode> a_pOwner);
	virtual ~RenderableComponent(); // don't call this method directly
	
	glm::daabb& boundingBox(){ return m_BoundingBox; }
	
private:
	glm::daabb m_BoundingBox;
};

class ModelComponentFactory
{
public:
	static ModelComponentFactory& singleton();

	std::shared_ptr<EngineComponent> createMesh(std::shared_ptr<SceneNode> a_pOwner, wxString a_Filename, bool a_bAsync);
	std::shared_ptr<EngineComponent> createSphere(std::shared_ptr<SceneNode> a_pOwner);
	std::shared_ptr<EngineComponent> createBox(std::shared_ptr<SceneNode> a_pOwner);
	//std::shared_ptr<EngineComponent> createVoxelTerrain(std::shared_ptr<SceneNode> a_pOwner, glm::ivec3 a_Size);

	void clearCache();

private:
	ModelComponentFactory();
	virtual ~ModelComponentFactory();

	struct Instance
	{
		std::shared_ptr<VertexBuffer> m_pVtxBuffer;
		std::shared_ptr<IndexBuffer> m_pIdxBuffer;
		std::map<wxString, std::map<int, int> > m_Meshes;// mesh name : {(index count : base vertex)...}
	};
	std::map<wxString, std::shared_ptr<Instance> > m_FileCache;

};

}

#endif