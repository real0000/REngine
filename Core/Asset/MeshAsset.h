// MeshAsset.h
//
// 2020/06/08 Ian Wu/Real0000
//

#ifndef _MESH_ASSET_H_
#define _MESH_ASSET_H_

#include "AssetBase.h"

namespace R
{

class IndexBuffer;
class VertexBuffer;

class MeshAsset : public AssetComponent
{
public:
	struct Instance
	{
        wxString m_Name;
		unsigned int m_IndexCount;
		unsigned int m_StartIndex;
		unsigned int m_BaseVertex;
		unsigned int m_VtxFlag;
		glm::aabb m_VisibleBoundingBox;
		std::vector<glm::aabb> m_PhysicsBoundingBox;
		std::map<unsigned int, std::shared_ptr<Asset>> m_Materials;// mat slot : material asset
	};
	struct Relation
	{
		wxString m_Nodename;
		glm::mat4x4 m_Tansform;
		std::vector<unsigned int> m_RefMesh;
	};
public:
	MeshAsset();
	virtual ~MeshAsset();

	// for asset factory
	static void validImportExt(std::vector<wxString> &a_Output);
	static wxString validAssetKey();

	virtual wxString getAssetExt(){ return MeshAsset::validAssetKey(); }
	virtual void importFile(wxString a_File);
	virtual void loadFile(boost::property_tree::ptree &a_Src);
	virtual void saveFile(boost::property_tree::ptree &a_Dst);

	const std::vector<glm::vec3>& getPosition(){ return m_Position; }
    const std::vector<glm::vec4>& getTexcoord(unsigned int a_Slot){ return m_Texcoord[a_Slot]; }
    const std::vector<glm::vec3>& getNormal(){ return m_Normal; }
    const std::vector<glm::vec3>& getTangent(){ return m_Tangent; }
    const std::vector<glm::vec3>& getBinormal(){ return m_Binormal; }
	const std::vector<glm::ivec4>& getBoneId(){ return m_BoneId; }
	const std::vector<glm::vec4>& getWeight(){ return m_Weight; }
	const std::vector<unsigned int>& getIndicies(){ return m_Indicies; }
	std::vector<Instance*>& getMeshes(){ return m_Meshes; }// remove const for bounding box and material edit
	const std::vector<Relation*>& getRelation(){ return m_Relation; }
	const std::vector<glm::mat4x4>& getBones(){ return m_Bones; }
	std::shared_ptr<VertexBuffer> getVertexBuffer(){ return m_VertexBuffer; }
	std::shared_ptr<IndexBuffer> getIndexBuffer(){ return m_IndexBuffer; }

private:
	void initBuffers();

    std::vector<glm::vec3> m_Position;
    std::vector<glm::vec4> m_Texcoord[4];
    std::vector<glm::vec3> m_Normal;
    std::vector<glm::vec3> m_Tangent;
    std::vector<glm::vec3> m_Binormal;
	std::vector<glm::ivec4> m_BoneId;
	std::vector<glm::vec4> m_Weight;
	std::vector<unsigned int> m_Indicies;
	
	std::shared_ptr<VertexBuffer> m_VertexBuffer;
	std::shared_ptr<IndexBuffer> m_IndexBuffer;

    std::vector<Instance*> m_Meshes;
	std::vector<Relation*> m_Relation;
	std::vector<glm::mat4x4> m_Bones;
};

}
#endif