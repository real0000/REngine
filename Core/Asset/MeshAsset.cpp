// MeshAsset.cpp
//
// 2020/06/08 Ian Wu/Real0000
//

#include "CommonUtil.h"
#include "RGDeviceWrapper.h"
#include "RImporters.h"

#include "Core.h"
#include "MaterialAsset.h"
#include "MeshAsset.h"

namespace R
{

#pragma region MeshAsset
//
// MeshAsset
// 
MeshAsset::MeshAsset()
	: m_Position(0)
	, m_Texcoord{std::vector<glm::vec4>(0), std::vector<glm::vec4>(0), std::vector<glm::vec4>(0), std::vector<glm::vec4>(0)}
    , m_Normal(0)
    , m_Tangent(0)
    , m_Binormal(0)
	, m_BoneId(0)
	, m_Weight(0)
	, m_Indicies(0)
	, m_VertexBuffer(nullptr)
	, m_IndexBuffer(nullptr)
	, m_Meshes(0)
	, m_Relation()
	, m_Bones(0)
{
}

MeshAsset::~MeshAsset()
{
	m_VertexBuffer = nullptr;
	m_IndexBuffer = nullptr;

	m_Position.clear();
    for( unsigned int i=0 ; i<4 ; ++i ) m_Texcoord[i].clear();
    m_Normal.clear();
    m_Tangent.clear();
    m_Binormal.clear();
	m_BoneId.clear();
	m_Weight.clear();
	m_Indicies.clear();

	for( unsigned int i=0 ; i<m_Meshes.size() ; ++i )
	{
		m_Meshes[i]->m_pMaterial = nullptr;
		delete m_Meshes[i];
	}
	m_Meshes.clear();

	for( unsigned int i=0 ; i<m_Relation.size() ; ++i ) delete m_Relation[i];
	m_Relation.clear();

	m_Bones.clear();
}

void MeshAsset::validImportExt(std::vector<wxString> &a_Output)
{
	a_Output.push_back(wxT("3ds"));
	a_Output.push_back(wxT("fbx"));
	a_Output.push_back(wxT("dxf"));
	a_Output.push_back(wxT("dae"));
	a_Output.push_back(wxT("obj"));
}

wxString MeshAsset::validAssetKey()
{
	return wxT("Mesh");
}

void MeshAsset::importFile(wxString a_File)
{
	std::shared_ptr<ModelData> l_pModel = ModelManager::singleton().getData(a_File).second;
	
	std::set<ModelNode *> l_NodeSet;
	{// setup meshes
		auto &l_Src = l_pModel->getMeshes();
		for( unsigned int i=0 ; i<l_Src.size() ; ++i )
		{
			ModelData::Meshes *l_pSrc = l_Src[i];
			Instance *l_pDst = new Instance();
			m_Meshes.push_back(l_pDst);
			
			l_pDst->m_BaseVertex = m_Position.size();
			l_pDst->m_VisibleBoundingBox = l_pSrc->m_BoundingBox;
			l_pDst->m_IndexCount = l_pSrc->m_Indicies.size();
			l_pDst->m_VtxFlag = GENERAL_VTX_FLAG;
			if( !l_pModel->getBones().empty() ) l_pDst->m_VtxFlag |= VTXFLAG_BONE | VTXFLAG_WEIGHT | VTXFLAG_USE_SKIN;

			l_pDst->m_Name = l_pSrc->m_Name;
			l_pDst->m_StartIndex = m_Indicies.size();
			l_pDst->m_pMaterial = AssetManager::singleton().createAsset("Default.Material").second;
			l_pDst->m_pMaterial->getComponent<MaterialAsset>()->setTexture("m_DiffTex", EngineCore::singleton().getWhiteTexture());
			l_pDst->m_pMaterial->getComponent<MaterialAsset>()->setTexture("m_NormalTex", EngineCore::singleton().getWhiteTexture());

			for( unsigned int j=0 ; j<l_pSrc->m_Vertex.size() ; ++j )
			{
				m_Position.push_back(l_pSrc->m_Vertex[j].m_Position);
				m_Texcoord[0].push_back(l_pSrc->m_Vertex[j].m_Texcoord[0]);
				m_Texcoord[1].push_back(l_pSrc->m_Vertex[j].m_Texcoord[1]);
				m_Texcoord[2].push_back(l_pSrc->m_Vertex[j].m_Texcoord[2]);
				m_Texcoord[3].push_back(l_pSrc->m_Vertex[j].m_Texcoord[3]);
				m_Normal.push_back(l_pSrc->m_Vertex[j].m_Normal);
				m_Tangent.push_back(l_pSrc->m_Vertex[j].m_Tangent);
				m_Binormal.push_back(l_pSrc->m_Vertex[j].m_Binormal);
				m_BoneId.push_back(l_pSrc->m_Vertex[j].m_BoneId);
				m_Weight.push_back(l_pSrc->m_Vertex[j].m_Weight);
			}

			for( unsigned int j=0 ; j<l_pSrc->m_Indicies.size() ; ++j ) m_Indicies.push_back(l_pSrc->m_Indicies[j]);
			for( unsigned int j=0 ; j<l_pSrc->m_RefNode.size() ; ++j ) l_NodeSet.insert(l_pSrc->m_RefNode[i]);
		}
	}

	{// copy bones
		auto &l_Src = l_pModel->getBones();
		if( !l_Src.empty() )
		{
			m_Bones.resize(l_Src.size());
			std::copy(m_Bones.begin(), m_Bones.end(), l_Src.begin());
		}
	}

	// setup nodes
	for( auto it=l_NodeSet.begin() ; it!=l_NodeSet.end() ; ++it )
	{
		Relation *l_pRelNode = new Relation();
		m_Relation.push_back(l_pRelNode);
		l_pRelNode->m_Nodename = (*it)->getName();
		l_pRelNode->m_Tansform = (*it)->getAbsoluteTransform();
		l_pRelNode->m_RefMesh.resize((*it)->getRefMesh().size());
		std::copy(l_pRelNode->m_RefMesh.begin(), l_pRelNode->m_RefMesh.end(), (*it)->getRefMesh().begin());
	}

	initBuffers();
}

void MeshAsset::loadFile(boost::property_tree::ptree &a_Src)
{
	boost::property_tree::ptree &l_Root = a_Src.get_child("root");

	boost::property_tree::ptree l_Instances = l_Root.get_child("Instances");
	for( auto it=l_Instances.begin() ; it!=l_Instances.end() ; ++it )
	{
		if( "<xmlattr>" == it->first ) continue;

		boost::property_tree::ptree l_InstanceAttr = it->second.get_child("<xmlattr>");

		Instance *l_pDst = new Instance();
		m_Meshes.push_back(l_pDst);

		l_pDst->m_Name = l_InstanceAttr.get<std::string>("name");
		l_pDst->m_BaseVertex = l_InstanceAttr.get<unsigned int>("baseVertex");
		l_pDst->m_StartIndex = l_InstanceAttr.get<unsigned int>("indexStart");
		l_pDst->m_IndexCount = l_InstanceAttr.get<unsigned int>("indexCount");
		l_pDst->m_VtxFlag = l_InstanceAttr.get<unsigned int>("vtxFlag");
		l_pDst->m_pMaterial = AssetManager::singleton().getAsset(l_InstanceAttr.get<std::string>("material")).second;
		
		boost::property_tree::ptree l_BoundingBoxes = it->second.get_child("BoundingBoxes");
		for( auto it2=l_BoundingBoxes.begin() ; it2!=l_BoundingBoxes.end() ; ++it2 )
		{
			if( "<xmlattr>" == it2->first ) continue;

			glm::aabb l_Box;
			if( "VisibleBoundingBox" == it2->first )
			{
				parseShaderParamValue(ShaderParamType::float3, it2->second.get_child("Center").data(), reinterpret_cast<char *>(&l_pDst->m_VisibleBoundingBox.m_Center));
				parseShaderParamValue(ShaderParamType::float3, it2->second.get_child("Size").data(), reinterpret_cast<char *>(&l_pDst->m_VisibleBoundingBox.m_Size));
			}
			else
			{
				parseShaderParamValue(ShaderParamType::float3, it2->second.get_child("Center").data(), reinterpret_cast<char *>(&l_Box.m_Center));
				parseShaderParamValue(ShaderParamType::float3, it2->second.get_child("Size").data(), reinterpret_cast<char *>(&l_Box.m_Size));
				l_pDst->m_PhysicsBoundingBox.push_back(l_Box);
			}
		}
	}
	
	boost::property_tree::ptree l_Relations = l_Root.get_child("Relations");
	for( auto it=l_Relations.begin() ; it!=l_Relations.end() ; ++it )
	{
		if( "<xmlattr>" == it->first ) continue;

		Relation *l_pNewRelation = new Relation();
		m_Relation.push_back(l_pNewRelation);

		l_pNewRelation->m_Nodename = it->second.get<std::string>("<xmlattr>.name");
		parseShaderParamValue(ShaderParamType::float4x4, it->second.get_child("Transform").data(), reinterpret_cast<char *>(&l_pNewRelation->m_Tansform));

		std::vector<char> l_Buff;
		base642Binary(it->second.get_child("RefMeshIdx").data(), l_Buff);
		l_pNewRelation->m_RefMesh.resize(l_Buff.size() / sizeof(unsigned int));
		memcpy(l_pNewRelation->m_RefMesh.data(), l_Buff.data(), l_Buff.size());
	}

	boost::optional<boost::property_tree::ptree&> l_BoneSrc = l_Root.get_child_optional("Bones");
	if( l_BoneSrc )
	{
		std::vector<char> l_Buff;
		base642Binary(l_BoneSrc.get().data(), l_Buff);
		m_Bones.resize(l_Buff.size() / sizeof(glm::mat4x4));
		memcpy(m_Bones.data(), l_Buff.data(), l_Buff.size());
	}

	boost::property_tree::ptree l_Vertex = l_Root.get_child("Vertex");
	unsigned int l_NumVtx = l_Vertex.get<unsigned int>("<xmlattr>.num");

	{
		std::vector<char> l_Buff;

		base642Binary(l_Vertex.get_child("Position").data(), l_Buff);
		m_Position.resize(l_NumVtx);
		memcpy(m_Position.data(), l_Buff.data(), l_Buff.size());

		for( unsigned int i=0 ; i<4 ; ++i )
		{
			char l_Key[16];
			snprintf(l_Key, 16, "Textcoord%d", i);

			base642Binary(l_Vertex.get_child(l_Key).data(), l_Buff);
			m_Texcoord[i].resize(l_NumVtx);
			memcpy(m_Texcoord[i].data(), l_Buff.data(), l_Buff.size());
		}
		
		base642Binary(l_Vertex.get_child("Normal").data(), l_Buff);
		m_Normal.resize(l_NumVtx);
		memcpy(m_Normal.data(), l_Buff.data(), l_Buff.size());
		
		base642Binary(l_Vertex.get_child("Tangent").data(), l_Buff);
		m_Tangent.resize(l_NumVtx);
		memcpy(m_Tangent.data(), l_Buff.data(), l_Buff.size());

		base642Binary(l_Vertex.get_child("Binormal").data(), l_Buff);
		m_Binormal.resize(l_NumVtx);
		memcpy(m_Binormal.data(), l_Buff.data(), l_Buff.size());
		
		base642Binary(l_Vertex.get_child("BoneId").data(), l_Buff);
		m_BoneId.resize(l_NumVtx);
		memcpy(m_BoneId.data(), l_Buff.data(), l_Buff.size());
		
		base642Binary(l_Vertex.get_child("Weight").data(), l_Buff);
		m_Weight.resize(l_NumVtx);
		memcpy(m_Weight.data(), l_Buff.data(), l_Buff.size());

		base642Binary(l_Vertex.get_child("Indicies").data(), l_Buff);
		m_Indicies.resize(l_Buff.size() / sizeof(unsigned int));
		memcpy(m_Indicies.data(), l_Buff.data(), l_Buff.size());
	}

	initBuffers();
}

void MeshAsset::saveFile(boost::property_tree::ptree &a_Dst)
{
	boost::property_tree::ptree l_Root;

	boost::property_tree::ptree l_Instances;
	for( unsigned int i=0 ; i<m_Meshes.size() ; ++i )
	{
		boost::property_tree::ptree l_Instance;
		boost::property_tree::ptree l_InstanceAttr;
		l_InstanceAttr.add("name", m_Meshes[i]->m_Name.c_str());
		l_InstanceAttr.add("baseVertex", m_Meshes[i]->m_BaseVertex);
		l_InstanceAttr.add("indexStart", m_Meshes[i]->m_StartIndex);
		l_InstanceAttr.add("indexCount", m_Meshes[i]->m_IndexCount);
		l_InstanceAttr.add("vtxFlag", m_Meshes[i]->m_VtxFlag);
		l_InstanceAttr.add("material", m_Meshes[i]->m_pMaterial->getKey().c_str());
		l_Instance.add_child("<xmlattr>", l_InstanceAttr);

		boost::property_tree::ptree l_BoundingBoxes;
		{
			boost::property_tree::ptree l_BoundingBox;
			l_BoundingBox.put("Center", convertParamValue(ShaderParamType::float3, reinterpret_cast<char *>(&m_Meshes[i]->m_VisibleBoundingBox.m_Center)));
			l_BoundingBox.put("Size", convertParamValue(ShaderParamType::float3, reinterpret_cast<char *>(&m_Meshes[i]->m_VisibleBoundingBox.m_Size)));
			l_BoundingBoxes.add_child("VisibleBoundingBox", l_BoundingBox);
		}
		for( unsigned int j=0 ; j<m_Meshes[i]->m_PhysicsBoundingBox.size() ; ++j )
		{
			boost::property_tree::ptree l_BoundingBox;
			l_BoundingBox.put("Center", convertParamValue(ShaderParamType::float3, reinterpret_cast<char *>(&m_Meshes[i]->m_PhysicsBoundingBox[j].m_Center)));
			l_BoundingBox.put("Size", convertParamValue(ShaderParamType::float3, reinterpret_cast<char *>(&m_Meshes[i]->m_PhysicsBoundingBox[j].m_Size)));
			l_BoundingBoxes.add_child("PhysicsBoundingBox", l_BoundingBox);
		}
		l_Instance.add_child("BoundingBoxes", l_BoundingBoxes);
		l_Instances.add_child("Instance", l_Instance);
	}
	l_Root.add_child("Instances", l_Instances);
	
	boost::property_tree::ptree l_Relations;
	for( unsigned int i=0 ; i<m_Relation.size() ; ++i )
	{
		boost::property_tree::ptree l_Relation;
		l_Relation.add("<xmlattr>.name", m_Relation[i]->m_Nodename.c_str());
		l_Relation.put("Transform", convertParamValue(ShaderParamType::float4x4, reinterpret_cast<char *>(&m_Relation[i]->m_Tansform)));

		std::string l_Base64("");
		binary2Base64(m_Relation[i]->m_RefMesh.data(), m_Relation[i]->m_RefMesh.size() * sizeof(unsigned int), l_Base64);
		l_Relation.put("RefMeshIdx", l_Base64);

		l_Relations.add_child("Relation", l_Relation);
	}
	l_Root.add_child("Relations", l_Relations);

	if( !m_Bones.empty() )
	{
		std::string l_RawData("");
		binary2Base64(m_Bones.data(), sizeof(glm::mat4x4) * m_Bones.size(), l_RawData);
		l_Root.put("Bones", l_RawData);
	}

	boost::property_tree::ptree l_Vertex;
	l_Vertex.add("<xmlattr>.num", m_Position.size());
	{
		std::string l_Base64("");

		binary2Base64(m_Position.data(), m_Position.size() * sizeof(glm::vec3), l_Base64);
		l_Vertex.put("Position", l_Base64);

		for( unsigned int i=0 ; i<4 ; ++i )
		{
			char l_Buff[16];
			snprintf(l_Buff, 16, "Textcoord%d", i);
			binary2Base64(m_Texcoord[i].data(), m_Texcoord[i].size() * sizeof(glm::vec4), l_Base64);
			l_Vertex.put(l_Buff, l_Base64);
		}

		binary2Base64(m_Normal.data(), m_Normal.size() * sizeof(glm::vec3), l_Base64);
		l_Vertex.put("Normal", l_Base64);

		binary2Base64(m_Tangent.data(), m_Tangent.size() * sizeof(glm::vec3), l_Base64);
		l_Vertex.put("Tangent", l_Base64);

		binary2Base64(m_Binormal.data(), m_Binormal.size() * sizeof(glm::vec3), l_Base64);
		l_Vertex.put("Binormal", l_Base64);

		binary2Base64(m_BoneId.data(), m_BoneId.size() * sizeof(glm::ivec4), l_Base64);
		l_Vertex.put("BoneId", l_Base64);
		
		binary2Base64(m_Weight.data(), m_Weight.size() * sizeof(glm::vec4), l_Base64);
		l_Vertex.put("Weight", l_Base64);
		
		binary2Base64(m_Indicies.data(), m_Indicies.size() * sizeof(unsigned int), l_Base64);
		l_Vertex.put("Indicies", l_Base64);
	}
	l_Root.add_child("Vertex", l_Vertex);

	a_Dst.add_child("root", l_Root);
}

void MeshAsset::initBuffers()
{
	m_VertexBuffer = std::shared_ptr<VertexBuffer>(new VertexBuffer());
	m_IndexBuffer = std::shared_ptr<IndexBuffer>(new IndexBuffer());

	m_VertexBuffer->setNumVertex(m_Position.size());
	m_VertexBuffer->setVertex(VTXSLOT_POSITION, m_Position.data());
	m_VertexBuffer->setVertex(VTXSLOT_TEXCOORD01, m_Texcoord[0].data());
	m_VertexBuffer->setVertex(VTXSLOT_TEXCOORD23, m_Texcoord[1].data());
	m_VertexBuffer->setVertex(VTXSLOT_TEXCOORD45, m_Texcoord[2].data());
	m_VertexBuffer->setVertex(VTXSLOT_TEXCOORD67, m_Texcoord[3].data());
	m_VertexBuffer->setVertex(VTXSLOT_NORMAL, m_Normal.data());
	m_VertexBuffer->setVertex(VTXSLOT_TANGENT, m_Tangent.data());
	m_VertexBuffer->setVertex(VTXSLOT_BINORMAL, m_Binormal.data());
	m_VertexBuffer->setVertex(VTXSLOT_BONE, m_BoneId.data());
	m_VertexBuffer->setVertex(VTXSLOT_WEIGHT, m_Weight.data());
	m_VertexBuffer->init();

	m_IndexBuffer->setIndicies(true, m_Indicies.data(), m_Indicies.size());
	m_IndexBuffer->init();
}
#pragma endregion

}