// ModelImporter.h
//
// 2017/06/02 Ian Wu/Real0000
//
#ifndef _MODEL_IMPORTER_H_
#define _MODEL_IMPORTER_H_

namespace R
{

enum DefaultTextureUsageType
{
	TEXUSAGE_DIFFUSE = 0,
	TEXUSAGE_SPECULAR,
	TEXUSAGE_GLOSSINESS,
	TEXUSAGE_NORMAL,
	TEXUSAGE_HEIGHT,

	TEXUSAGE_TYPECOUNT
};

class ModelData
{
public:
	struct ModelVertex
    {
        ModelVertex()
            : m_Position(0.0f, 0.0f, 0.0f)
            , m_Normal(0.0f, 0.0f, 0.0f)
            , m_Tangent(0.0f, 0.0f, 0.0f)
            , m_Binormal(0.0f, 0.0f, 0.0f)
			, m_BoneId(0, 0, 0, 0)
			, m_Weight(0.0f, 0.0f, 0.0f, 0.0f)
			, m_Color(0xffffffff)
		{
			memset(m_Texcoord, 0, sizeof(glm::vec4) * 4);
		}
		virtual ~ModelVertex(){}

        glm::vec3 m_Position;
        glm::vec4 m_Texcoord[4];
        glm::vec3 m_Normal;
        glm::vec3 m_Tangent;
        glm::vec3 m_Binormal;
		glm::ivec4 m_BoneId;
		glm::vec4 m_Weight;
		unsigned int m_Color;
    };
    struct ModelMeshes
    {
        ModelMeshes()
			: m_Name(wxT(""))
			, m_Index(0)
			, m_BoxSize(0.0f, 0.0f, 0.0f)
		{
		}
		virtual ~ModelMeshes(){}

        wxString m_Name;
        unsigned int m_Index;
        std::vector<ModelVertex> m_Vertex;
        std::vector<unsigned int> m_Indicies;

        std::map<unsigned int, std::string> m_Texures[8];
        std::vector<unsigned int> m_RefNode;
        glm::vec3 m_BoxSize;
    };
	struct ModelNode
    {
        ModelNode() : m_pParent(nullptr), m_NodeName(""), m_Transform(1.0f){}
        ~ModelNode()
        {
            m_Children.clear();
        }

        ModelNode *m_pParent;
        std::string m_NodeName;
        glm::mat4x4 m_Transform;
        std::vector<unsigned int> m_Children;//index
        std::vector<unsigned int> m_RefMesh;//index
    };
public:
	ModelData();
	virtual ~ModelData();

	void init(wxString a_Filepath);
	ModelNode* find(wxString a_Name);

    std::vector<ModelMeshes *> m_Meshes;
	std::vector<ModelNode *> m_Nodes;
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