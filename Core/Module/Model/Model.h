// Model.h
//
// 2017/07/19 Ian Wu/Real0000
//

#ifndef _MODEL_H_
#define _MODEL_H_

namespace R
{

class Material;
class SceneNode;

class ModelComponent : public EngineComponent
{
	friend class ModelFactory;
public:
	std::shared_ptr<VertexBuffer> getVertexBuffer(){ return m_pRefVtxBuffer; }
	std::shared_ptr<IndexBuffer> getIndexBuffer(){ return m_pRefIdxBuffer; }
	std::pair<int, int> getIndexRange(){ return m_Range; }

private:
	ModelComponent(SceneNode *a_pOwner);
	virtual ~ModelComponent();

	std::shared_ptr<VertexBuffer> m_pRefVtxBuffer;
	std::shared_ptr<IndexBuffer> m_pRefIdxBuffer;
	std::pair<int, int> m_Range;// index offset : count
	std::map<unsigned int, std::vector< std::shared_ptr<Material> > > m_MaterialSet;
};

class ModelFactory
{
public:
	static ModelFactory& singleton();

	std::shared_ptr<EngineComponent> createMesh(wxString a_Filename);
	std::shared_ptr<EngineComponent> createSphere(float a_Range);

	void clearCache();

private:
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