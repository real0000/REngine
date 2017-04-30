// ExternModel.h
//
// 2014/06/06 Ian Wu/Real0000
//

#ifndef _EXTERN_MODEL_H_
#define _EXTERN_MODEL_H_

#include "fbxsdk.h"

namespace R
{

class VertexBuffer;
class IndexBuffer;

class ExternModelMesh
{
public:
	ExternModelMesh();
	virtual ~ExternModelMesh();
	
	void parseMeshData(const FbxScene *a_pSceneData, const FbxMesh *a_pMeshData, std::vector<VertexPosTex2NormalBone> &a_VtxOutput, std::vector<unsigned int> &a_IdxOutput);
	void parseBoneData(std::map<wxString, unsigned int> &l_NameIdxMap, const aiMesh *l_pMeshData, std::vector<VertexPosTex2NormalBone> &a_VtxOutput, unsigned int a_VtxBase);

	wxString getName(){ return m_Name; }
	wxString getDefaultDiffuseTexture(){ return m_DiffuseTex; }
	wxString getDefaultNormalTexture(){ return m_NormalTex; }

private:
	std::vector<unsigned int> m_BoneID;
	std::pair<unsigned int, unsigned int> m_IndexRange;//start, count
	wxString m_Name;
	wxString m_NormalTex, m_DiffuseTex;
};

struct ExternModelNode
{
	ExternModelNode();
	virtual ~ExternModelNode();

	wxString m_Name;
	glm::mat4x4 m_Origin;
	glm::mat4x4 m_Trans;
	int m_Parent;
	std::vector<unsigned int> m_RefMesh;
	std::vector<unsigned int> m_Children;
};

struct ExternModelAnime
{
public:
	ExternModelAnime();
	virtual ~ExternModelAnime();
	
	void parseAnimeData(std::map<wxString, unsigned int> &l_NameIdxMap, const aiAnimation *l_pAnimData, std::vector<ExternModelNode *> &l_Nodes);//Node for trans matrix

private:
	struct AnimeNodes//tick, 1/60 sec
	{
		unsigned int m_NodeID;
		std::vector<unsigned int> m_PosKeyMap;
		std::vector< std::pair<unsigned int, glm::vec3> > m_PosKey;
		std::vector<unsigned int> m_ScaleKeyMap;
		std::vector< std::pair<unsigned int, glm::vec3> > m_ScaleKey;
		std::vector<unsigned int> m_RotKeyMap;
		std::vector< std::pair<unsigned int, glm::quat> > m_RotKey;
	};
	template<typename l_T>
	void initAnimeNode(std::vector< std::pair<unsigned int, l_T> > &l_Keys, std::vector<unsigned int> &l_Maps, l_T &l_Default, const aiNodeAnim *l_pNodeAnimData);

	wxString m_Name;
	float m_Length;//in second
	std::vector<AnimeNodes *> m_Nodes;
};

class ExternModelData
{
public:
	ExternModelData();
	virtual ~ExternModelData();
	
	VertexBuffer *m_pVertex;
	IndexBuffer *m_pIndex;
	std::vector<ExternModelMesh *> m_Meshes;
	std::vector<ExternModelNode *> m_Nodes;// 0 == root
	std::vector<ExternModelAnime *> m_Animes;

	bool loadMeshFile(wxString a_File, wxString &a_ErrorMsg);
	void initDrawBuffer();
	bool isValid(){ return m_bValid; }

private:
	bool m_bValid;
};

class ExternModelFileManager
{
public:
	static ExternModelFileManager& singleton();
	static void purge();

	void clearCache();
	ExternModelData* getModelData(wxString l_Filename, bool l_bImmediate);

private:
	ExternModelFileManager();
	virtual ~ExternModelFileManager();

	std::map<wxString, ExternModelData *> m_ModelCache;
};

}

#endif