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
	std::vector<unsigned int>& getRefMesh(){ return m_RefMesh; }

private:
    ModelNode *m_pParent;
    wxString m_NodeName;
    glm::mat4x4 m_Transform;
    std::vector<ModelNode *> m_Children;//index
    std::vector<unsigned int> m_RefMesh;//index
};

class ModelData
{
public:
	struct Vertex
    {
        Vertex()
            : m_Position(0.0f, 0.0f, 0.0f)
            , m_Normal(0.0f, 0.0f, 0.0f)
            , m_Tangent(0.0f, 0.0f, 0.0f)
            , m_Binormal(0.0f, 0.0f, 0.0f)
			, m_BoneId(0, 0, 0, 0)
			, m_Weight(1.0f, 0.0f, 0.0f, 0.0f)
		{
			memset(m_Texcoord, 0, sizeof(glm::vec4) * 4);
		}
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
			, m_BoxSize(0.0f, 0.0f, 0.0f)
			, m_bHasBone(false)
		{
		}
		virtual ~Meshes(){}

        wxString m_Name;
        unsigned int m_Index;
        std::vector<Vertex> m_Vertex;
        std::vector<unsigned int> m_Indicies;

        std::map<unsigned int, std::string> m_Texures[8];
		std::vector<glm::mat4x4> m_Bones;
        std::vector<ModelNode *> m_RefNode;
        glm::vec3 m_BoxSize;
		bool m_bHasBone;
    };
public:
	ModelData();
	virtual ~ModelData();

	void init(wxString a_Filepath);

	std::vector<Meshes *>& getMeshes(){ return m_Meshes; }
	ModelNode* find(wxString a_Name);
	ModelNode* getRootNode(){ return m_pRootNode; }

private:
    std::vector<Meshes *> m_Meshes;
	ModelNode * m_pRootNode;
};

class ModelManager : public SearchPathSystem<ModelData>
{
public:
	static ModelManager& singleton();

private:
	ModelManager();
	virtual ~ModelManager();

	void loadFile(ModelData *a_pInst, wxString a_Path);
};

}

#endif