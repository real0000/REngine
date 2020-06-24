// ModelImporter.h
//
// 2017/06/02 Ian Wu/Real0000
//
#ifndef _MODEL_IMPORTER_H_
#define _MODEL_IMPORTER_H_

namespace R
{

class AnimationData;
class ModelData;

enum DefaultTextureUsageType
{
	TEXUSAGE_DIFFUSE = 0,
	TEXUSAGE_SPECULAR,
	TEXUSAGE_GLOSSINESS,
	TEXUSAGE_NORMAL,
	TEXUSAGE_HEIGHT,

	TEXUSAGE_TYPECOUNT
};

class ModelNode
{
	friend class ModelData;
	friend class AnimationData;
public:
    ModelNode();
    virtual ~ModelNode();

	ModelNode* getParent(){ return m_pParent; }
	ModelNode* find(wxString a_Name);
	wxString getName(){ return m_NodeName; }
	glm::mat4x4 getTransform(){ return m_Transform; }
	glm::mat4x4 getAbsoluteTransform();
	std::vector<unsigned int>& getRefMesh(){ return m_RefMesh; }
	std::vector<ModelNode *> getChildren(){ return m_Children; }

private:
    ModelNode *m_pParent;
    wxString m_NodeName;
    glm::mat4x4 m_Transform;
    std::vector<ModelNode *> m_Children;
    std::vector<unsigned int> m_RefMesh;//index
};

class ModelData
{
public:
	struct Vertex
    {
        Vertex()
            : m_Position(0.0f, 0.0f, 0.0f)
			, m_Texcoord{glm::zero<glm::vec4>(), glm::zero<glm::vec4>(), glm::zero<glm::vec4>(), glm::zero<glm::vec4>()}
            , m_Normal(0.0f, 0.0f, 0.0f)
            , m_Tangent(0.0f, 0.0f, 0.0f)
            , m_Binormal(0.0f, 0.0f, 0.0f)
			, m_BoneId(0, 0, 0, 0)
			, m_Weight(1.0f, 0.0f, 0.0f, 0.0f){}
		virtual ~Vertex(){}

        glm::vec3 m_Position;
        glm::vec4 m_Texcoord[4];
        glm::vec3 m_Normal;
        glm::vec3 m_Tangent;
        glm::vec3 m_Binormal;
		glm::ivec4 m_BoneId;
		glm::vec4 m_Weight;
    };
    struct Meshes
    {
        Meshes()
			: m_Name(wxT(""))
			, m_Index(0)
			, m_RefMaterial(-1)
			, m_bHasBone(false)
			, m_BoundingBox()
		{
		}
		virtual ~Meshes(){}

        wxString m_Name;
        unsigned int m_Index;
        std::vector<Vertex> m_Vertex;
        std::vector<unsigned int> m_Indicies;

		int m_RefMaterial;
        std::vector<ModelNode *> m_RefNode;
		bool m_bHasBone;
		glm::aabb m_BoundingBox;
    };
public:
	ModelData();
	virtual ~ModelData();

	void init(wxString a_Filepath);

	std::vector<Meshes *>& getMeshes(){ return m_Meshes; }
	ModelNode* find(wxString a_Name);
	ModelNode* getRootNode(){ return m_pRootNode; }
	std::vector<glm::mat4x4>& getBones(){ return m_Bones; }

private:
	std::set<int> m_Materials;
    std::vector<Meshes *> m_Meshes;
	std::vector<glm::mat4x4> m_Bones;
	ModelNode *m_pRootNode;
};

class ModelManager : public SearchPathSystem<ModelData>
{
public:
	static ModelManager& singleton();

private:
	ModelManager();
	virtual ~ModelManager();

	void loadFile(std::shared_ptr<ModelData> a_pInst, wxString a_Path);
};

}

#endif