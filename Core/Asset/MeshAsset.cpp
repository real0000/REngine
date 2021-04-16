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
#include "TextureAsset.h"

namespace R
{

#pragma region MeshAsset
//
// MeshAsset
// 
MeshAsset::MeshAsset()
	: m_VtxSlots(0)
	, m_Position(0)
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

	clearVertexData();

	for( unsigned int i=0 ; i<m_Meshes.size() ; ++i )
	{
		m_Meshes[i]->m_Materials.clear();
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
	wxString l_ClearFileName(getFileName(a_File, false));
	wxString l_FilePath(getFilePath(a_File));
	
	// setup material
	std::map<wxString, std::shared_ptr<Asset>> l_MaterialMap;
	{
		std::map<wxString, ModelData::Material> &l_SrcTextureSet = l_pModel->getMaterials();
		for( auto it=l_SrcTextureSet.begin() ; it!=l_SrcTextureSet.end() ; ++it )
		{
			std::shared_ptr<Asset> l_pBaseColor = nullptr;
			std::shared_ptr<Asset> l_pNormal = nullptr;
			std::shared_ptr<Asset> l_pMetal = nullptr;
			std::shared_ptr<Asset> l_pRoughness = nullptr;

			ModelData::Material &l_ThisMaterial = it->second;
			wxString l_MatFile(wxString::Format(wxT("%s/%s_%s_Opaque.%s"), l_FilePath, l_ClearFileName, it->first, MaterialAsset::validAssetKey().mbc_str()));
			std::shared_ptr<Asset> l_pMat = AssetManager::singleton().createAsset(l_MatFile);
			if( l_pMat->dirty() )// new file
			{
				auto it2 = l_ThisMaterial.find(DefaultTextureUsageType::TEXUSAGE_BASECOLOR);
				if( l_ThisMaterial.end() == it2 ) l_pBaseColor = AssetManager::singleton().getAsset(WHITE_TEXTURE_ASSET_NAME);
				else l_pBaseColor = AssetManager::singleton().getAsset(concatFilePath(l_FilePath, it2->second));

				it2 = l_ThisMaterial.find(DefaultTextureUsageType::TEXUSAGE_NORMAL);
				if( l_ThisMaterial.end() == it2 ) l_pNormal = AssetManager::singleton().getAsset(BLUE_TEXTURE_ASSET_NAME);
				else l_pNormal = AssetManager::singleton().getAsset(concatFilePath(l_FilePath, it2->second));
				
				it2 = l_ThisMaterial.find(DefaultTextureUsageType::TEXUSAGE_METALLIC);
				if( l_ThisMaterial.end() == it2 ) l_pMetal = AssetManager::singleton().getAsset(WHITE_TEXTURE_ASSET_NAME);
				else l_pMetal = AssetManager::singleton().getAsset(concatFilePath(l_FilePath, it2->second));

				it2 = l_ThisMaterial.find(DefaultTextureUsageType::TEXUSAGE_ROUGHNESS);
				if( l_ThisMaterial.end() == it2 ) l_pRoughness = AssetManager::singleton().getAsset(DARK_GRAY_TEXTURE_ASSET_NAME);
				else l_pRoughness = AssetManager::singleton().getAsset(concatFilePath(l_FilePath, it2->second));

				MaterialAsset *l_pMatInst = l_pMat->getComponent<MaterialAsset>();
				l_pMatInst->init(ProgramManager::singleton().getData(DefaultPrograms::Standard));
				l_pMatInst->setTexture(STANDARD_TEXTURE_BASECOLOR, l_pBaseColor);
				l_pMatInst->setTexture(STANDARD_TEXTURE_NORMAL, l_pNormal);
				l_pMatInst->setTexture(STANDARD_TEXTURE_METAL, l_pMetal);
				l_pMatInst->setTexture(STANDARD_TEXTURE_ROUGHNESS, l_pRoughness);
			}
			l_MaterialMap.insert(std::make_pair(it->first, l_pMat));
		}
	}

	std::set<ModelNode *> l_NodeSet;
	m_VtxSlots = FULL_VTX_SLOT & (~VTXFLAG_COLOR);
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
			l_pDst->m_Materials.insert(std::make_pair(MATSLOT_OPAQUE, l_MaterialMap[l_pSrc->m_RefMaterial]));

			assignDefaultRuntimeMaterial(l_pDst);

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
			for( unsigned int j=0 ; j<l_pSrc->m_RefNode.size() ; ++j ) l_NodeSet.insert(l_pSrc->m_RefNode[j]);
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
		std::copy((*it)->getRefMesh().begin(), (*it)->getRefMesh().end(), l_pRelNode->m_RefMesh.begin());
	}

	initBuffers();
	setDirty();
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

		boost::property_tree::ptree l_Materials = it->second.get_child("Materials");
		for( auto it2=l_Materials.begin() ; it2!=l_Materials.end() ; ++it2 )
		{
			if( "<xmlattr>" == it2->first ) continue;
			
			unsigned int l_Slot = 0;
			sscanf(it->first.c_str(), "Slot%d", &l_Slot);
			l_pDst->m_Materials.insert(std::make_pair(l_Slot, AssetManager::singleton().getAsset(it2->second.data())));
		}

		assignDefaultRuntimeMaterial(l_pDst);
	}
	
	boost::property_tree::ptree l_Relations = l_Root.get_child("Relations");
	for( auto it=l_Relations.begin() ; it!=l_Relations.end() ; ++it )
	{
		if( "<xmlattr>" == it->first ) continue;

		Relation *l_pNewRelation = new Relation();
		m_Relation.push_back(l_pNewRelation);

		l_pNewRelation->m_Nodename = it->second.get<std::string>("<xmlattr>.name");
		parseShaderParamValue(ShaderParamType::float4x4, it->second.get_child("Transform").data(), reinterpret_cast<char *>(&l_pNewRelation->m_Tansform));

		std::vector<wxString> l_IndiciesStr;
		splitString(wxT(','), it->second.get_child("RefMeshIdx").data(), l_IndiciesStr);
		for( unsigned int i=0 ; i<l_IndiciesStr.size() ; ++i ) l_pNewRelation->m_RefMesh.push_back(atoi(l_IndiciesStr[i].c_str()));
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
		l_Instance.add_child("<xmlattr>", l_InstanceAttr);
		
		boost::property_tree::ptree l_Materials;
		for( auto it=m_Meshes[i]->m_Materials.begin() ; it!=m_Meshes[i]->m_Materials.end() ; ++it )
		{
			if( it->second->getRuntimeOnly() ) continue;

			char l_Buff[32];
			snprintf(l_Buff, 32, "Slot%d", it->first);
			l_Materials.put(l_Buff, it->second->getKey().c_str());

			AssetManager::singleton().saveAsset(it->second);
		}
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
		l_Instance.add_child("Materials", l_Materials);
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

		std::string l_Serial("");
		for( unsigned int j=0 ; j<m_Relation[i]->m_RefMesh.size() ; ++j )
		{
			char l_Buff[32];
			snprintf(l_Buff, 32, "%d,", m_Relation[i]->m_RefMesh[j]);
			l_Serial += l_Buff;
		}
		if( !l_Serial.empty() ) l_Serial.pop_back();
		l_Relation.put("RefMeshIdx", l_Serial);

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

void MeshAsset::init(unsigned int a_SlotFlag)
{
	assert(0 == m_VtxSlots && 0 != (a_SlotFlag & VTXFLAG_POSITION));
	m_VtxSlots = a_SlotFlag;
	setDirty();
}

MeshAsset::Instance* MeshAsset::addSubMesh(unsigned int a_ReserveVtxCount, unsigned int a_ReserveIdxCount
	, unsigned int **a_ppDstIndicies
	, glm::vec3 **a_ppDstPos
	, glm::vec4 **a_ppDstTexcoord01
	, glm::vec3 **a_ppDstNormal
	, glm::vec3 **a_ppDstBinormal
	, glm::vec3 **a_ppDstTangent
	, glm::ivec4 ** a_ppDstBoneId
	, glm::vec4 **a_ppDstWeight
	, glm::vec4 **a_ppDstTexcoord23
	, glm::vec4 **a_ppDstTexcoord45
	, glm::vec4 **a_ppDstTexcoord67
	, unsigned int **a_ppDstColor)
{
	assert(0 != m_VtxSlots);

	Instance *l_pRes = new Instance();
	l_pRes->m_BaseVertex = m_Position.size();
	l_pRes->m_StartIndex = m_Indicies.size();
	l_pRes->m_VtxFlag = m_VtxSlots;
	m_Meshes.push_back(l_pRes);
	
	unsigned int l_NewVtxSize = l_pRes->m_BaseVertex + a_ReserveVtxCount;
	unsigned int l_NewIdxSize = l_pRes->m_StartIndex + a_ReserveIdxCount;
	for( unsigned int i=VTXSLOT_POSITION ; i<=VTXSLOT_COLOR ; ++i )
	{
		if( 0 == (m_VtxSlots & (0x000000001 << i)) ) continue;
		switch(i)
		{
			case VTXSLOT_POSITION:
				m_Position.reserve(l_NewVtxSize);
				for( unsigned int j=l_pRes->m_BaseVertex ; j<l_NewVtxSize ; ++j ) m_Position.push_back(glm::vec3());
				assert(nullptr != a_ppDstPos);
				*a_ppDstPos = &(m_Position[l_pRes->m_BaseVertex]);
				break;

			case VTXSLOT_TEXCOORD01:
				m_Texcoord[0].reserve(l_NewVtxSize);
				for( unsigned int j=l_pRes->m_BaseVertex ; j<l_NewVtxSize ; ++j ) m_Texcoord[0].push_back(glm::vec4());
				assert(nullptr != a_ppDstTexcoord01);
				*a_ppDstTexcoord01 = &(m_Texcoord[0][l_pRes->m_BaseVertex]);
				break;

			case VTXSLOT_TEXCOORD23:
				m_Texcoord[1].reserve(l_NewVtxSize);
				for( unsigned int j=l_pRes->m_BaseVertex ; j<l_NewVtxSize ; ++j ) m_Texcoord[1].push_back(glm::vec4());
				assert(nullptr != a_ppDstTexcoord23);
				*a_ppDstTexcoord23 = &(m_Texcoord[1][l_pRes->m_BaseVertex]);
				break;

			case VTXSLOT_TEXCOORD45:
				m_Texcoord[2].reserve(l_NewVtxSize);
				for( unsigned int j=l_pRes->m_BaseVertex ; j<l_NewVtxSize ; ++j ) m_Texcoord[2].push_back(glm::vec4());
				assert(nullptr != a_ppDstTexcoord45);
				*a_ppDstTexcoord45 = &(m_Texcoord[2][l_pRes->m_BaseVertex]);
				break;

			case VTXSLOT_TEXCOORD67:
				m_Texcoord[3].reserve(l_NewVtxSize);
				for( unsigned int j=l_pRes->m_BaseVertex ; j<l_NewVtxSize ; ++j ) m_Texcoord[3].push_back(glm::vec4());
				assert(nullptr != a_ppDstTexcoord67);
				*a_ppDstTexcoord67 = &(m_Texcoord[3][l_pRes->m_BaseVertex]);
				break;

			case VTXSLOT_NORMAL:
				m_Normal.reserve(l_NewVtxSize);
				for( unsigned int j=l_pRes->m_BaseVertex ; j<l_NewVtxSize ; ++j ) m_Normal.push_back(glm::vec3());
				assert(nullptr != a_ppDstNormal);
				*a_ppDstNormal = &(m_Normal[l_pRes->m_BaseVertex]);
				break;

			case VTXSLOT_TANGENT:
				m_Tangent.reserve(l_NewVtxSize);
				for( unsigned int j=l_pRes->m_BaseVertex ; j<l_NewVtxSize ; ++j ) m_Tangent.push_back(glm::vec3());
				assert(nullptr != a_ppDstTangent);
				*a_ppDstTangent = &(m_Tangent[l_pRes->m_BaseVertex]);
				break;

			case VTXSLOT_BINORMAL:
				m_Binormal.reserve(l_NewVtxSize);
				for( unsigned int j=l_pRes->m_BaseVertex ; j<l_NewVtxSize ; ++j ) m_Binormal.push_back(glm::vec3());
				assert(nullptr != a_ppDstBinormal);
				*a_ppDstBinormal = &(m_Binormal[l_pRes->m_BaseVertex]);
				break;

			case VTXSLOT_BONE:
				m_BoneId.reserve(l_NewVtxSize);
				for( unsigned int j=l_pRes->m_BaseVertex ; j<l_NewVtxSize ; ++j ) m_BoneId.push_back(glm::ivec4());
				assert(nullptr != a_ppDstBoneId);
				*a_ppDstBoneId = &(m_BoneId[l_pRes->m_BaseVertex]);
				break;

			case VTXSLOT_WEIGHT:
				m_Weight.reserve(l_NewVtxSize);
				for( unsigned int j=l_pRes->m_BaseVertex ; j<l_NewVtxSize ; ++j ) m_Weight.push_back(glm::vec4());
				assert(nullptr != a_ppDstWeight);
				*a_ppDstWeight = &(m_Weight[l_pRes->m_BaseVertex]);
				break;

			case VTXSLOT_COLOR:
				m_Color.reserve(l_NewVtxSize);
				for( unsigned int j=l_pRes->m_BaseVertex ; j<l_NewVtxSize ; ++j ) m_Color.push_back(0u);
				assert(nullptr != a_ppDstColor);
				*a_ppDstColor = &(m_Color[l_pRes->m_BaseVertex]);
				break;

			default:break;
		}
	}

	m_Indicies.reserve(l_NewIdxSize);
	for( unsigned int j=l_pRes->m_StartIndex ; j<l_NewIdxSize ; ++j ) m_Indicies.push_back(0u);
	assert(nullptr != a_ppDstIndicies);
	*a_ppDstIndicies = &(m_Indicies[l_pRes->m_StartIndex]);

	initBuffers();
	setDirty();

	return l_pRes;
}

void MeshAsset::updateMeshData(glm::ivec2 a_VertexRange, glm::ivec2 a_IdxRange)
{
	for( unsigned int i=VTXSLOT_POSITION ; i<=VTXSLOT_COLOR ; ++i )
	{
		if( 0 == (m_VtxSlots & (0x000000001 << i)) ) continue;

		void *l_pSrcData = nullptr;
		switch(i)
		{
			case VTXSLOT_POSITION:
				l_pSrcData = &(m_Position[a_VertexRange.x]);
				break;

			case VTXSLOT_TEXCOORD01:
				l_pSrcData = &(m_Texcoord[0][a_VertexRange.x]);
				break;

			case VTXSLOT_TEXCOORD23:
				l_pSrcData = &(m_Texcoord[1][a_VertexRange.x]);
				break;

			case VTXSLOT_TEXCOORD45:
				l_pSrcData = &(m_Texcoord[2][a_VertexRange.x]);
				break;

			case VTXSLOT_TEXCOORD67:
				l_pSrcData = &(m_Texcoord[3][a_VertexRange.x]);
				break;

			case VTXSLOT_NORMAL:
				l_pSrcData = &(m_Normal[a_VertexRange.x]);
				break;

			case VTXSLOT_TANGENT:
				l_pSrcData = &(m_Tangent[a_VertexRange.x]);
				break;

			case VTXSLOT_BINORMAL:
				l_pSrcData = &(m_Binormal[a_VertexRange.x]);
				break;

			case VTXSLOT_BONE:
				l_pSrcData = &(m_BoneId[a_VertexRange.x]);
				break;

			case VTXSLOT_WEIGHT:
				l_pSrcData = &(m_Weight[a_VertexRange.x]);
				break;

			case VTXSLOT_COLOR:
				l_pSrcData = &(m_Color[a_VertexRange.x]);
				break;

			default:break;
		}
		m_VertexBuffer->updateVertexData(i, l_pSrcData, a_VertexRange.y, a_VertexRange.x);
	}
	
	m_IndexBuffer->updateIndexData(&(m_Indicies[a_IdxRange.x]), a_IdxRange.y, a_IdxRange.x);
	setDirty();
}

void MeshAsset::clearVertexData()
{
	m_Position.clear();
    for( unsigned int i=0 ; i<4 ; ++i ) m_Texcoord[i].clear();
    m_Normal.clear();
    m_Tangent.clear();
    m_Binormal.clear();
	m_BoneId.clear();
	m_Weight.clear();
	m_Color.clear();
	m_Indicies.clear();
}

void MeshAsset::initBuffers()
{
	m_VertexBuffer = std::shared_ptr<VertexBuffer>(new VertexBuffer());
	m_IndexBuffer = std::shared_ptr<IndexBuffer>(new IndexBuffer());

	m_VertexBuffer->setNumVertex(m_Position.size());
	m_VertexBuffer->setVertex(VTXSLOT_POSITION, m_Position.data());
	if( !m_Texcoord[0].empty() ) m_VertexBuffer->setVertex(VTXSLOT_TEXCOORD01, m_Texcoord[0].data());
	if( !m_Texcoord[1].empty() ) m_VertexBuffer->setVertex(VTXSLOT_TEXCOORD23, m_Texcoord[1].data());
	if( !m_Texcoord[2].empty() ) m_VertexBuffer->setVertex(VTXSLOT_TEXCOORD45, m_Texcoord[2].data());
	if( !m_Texcoord[3].empty() ) m_VertexBuffer->setVertex(VTXSLOT_TEXCOORD67, m_Texcoord[3].data());
	if( !m_Normal.empty() ) m_VertexBuffer->setVertex(VTXSLOT_NORMAL, m_Normal.data());
	if( !m_Tangent.empty() ) m_VertexBuffer->setVertex(VTXSLOT_TANGENT, m_Tangent.data());
	if( !m_Binormal.empty() ) m_VertexBuffer->setVertex(VTXSLOT_BINORMAL, m_Binormal.data());
	if( !m_BoneId.empty() ) m_VertexBuffer->setVertex(VTXSLOT_BONE, m_BoneId.data());
	if( !m_Weight.empty() ) m_VertexBuffer->setVertex(VTXSLOT_WEIGHT, m_Weight.data());
	if( !m_Color.empty() ) m_VertexBuffer->setVertex(VTXSLOT_COLOR, m_Color.data());
	m_VertexBuffer->init();

	m_IndexBuffer->setIndicies(true, m_Indicies.data(), m_Indicies.size());
	m_IndexBuffer->init();
}

void MeshAsset::assignDefaultRuntimeMaterial(Instance *a_pInst)
{
	const std::pair<wxString, int> c_ShadowSlots[] = {
		std::make_pair(DEFAULT_DIR_SHADOWMAP_MAT_ASSET_NAME, MATSLOT_DIR_SHADOWMAP),
		std::make_pair(DEFAULT_SPOT_SHADOWMAP_MAT_ASSET_NAME, MATSLOT_SPOT_SHADOWMAP),
		std::make_pair(DEFAULT_OMNI_SHADOWMAP_MAT_ASSET_NAME, MATSLOT_OMNI_SHADOWMAP)};
	for( unsigned int j=0 ; j<3 ; ++j )
	{
		unsigned int l_MatSlot = c_ShadowSlots[j].second;
		if( a_pInst->m_Materials.end() != a_pInst->m_Materials.find(l_MatSlot) ) continue;

		a_pInst->m_Materials.insert(std::make_pair(l_MatSlot, AssetManager::singleton().getAsset(c_ShadowSlots[j].first)));
	}

	if( a_pInst->m_Materials.end() == a_pInst->m_Materials.find(MATSLOT_LIGHTMAP) )
	{
		std::shared_ptr<Asset> l_pBaseColor = nullptr;
		std::shared_ptr<Asset> l_pNormal = nullptr;
		std::shared_ptr<Asset> l_pMetal = nullptr;
		std::shared_ptr<Asset> l_pRoughness = nullptr;
		std::shared_ptr<Asset> l_pRefraction = nullptr;
		{
			auto it = a_pInst->m_Materials.find(MATSLOT_OPAQUE);
			MaterialAsset *l_pSrcMatInst = nullptr;
			if( a_pInst->m_Materials.end() != it ) l_pSrcMatInst = it->second->getComponent<MaterialAsset>();
			else
			{
				it = a_pInst->m_Materials.find(MATSLOT_TRANSPARENT);
				if( a_pInst->m_Materials.end() != it ) l_pSrcMatInst = it->second->getComponent<MaterialAsset>();
			}

			l_pBaseColor = l_pSrcMatInst->getTexture(STANDARD_TEXTURE_BASECOLOR);
			if( nullptr == l_pBaseColor ) l_pBaseColor = AssetManager::singleton().getAsset(WHITE_TEXTURE_ASSET_NAME);

			l_pNormal = l_pSrcMatInst->getTexture(STANDARD_TEXTURE_NORMAL);
			if( nullptr == l_pNormal ) l_pNormal = AssetManager::singleton().getAsset(BLUE_TEXTURE_ASSET_NAME);

			l_pMetal = l_pSrcMatInst->getTexture(STANDARD_TEXTURE_METAL);
			if( nullptr == l_pMetal ) l_pMetal = AssetManager::singleton().getAsset(WHITE_TEXTURE_ASSET_NAME);

			l_pRoughness = l_pSrcMatInst->getTexture(STANDARD_TEXTURE_ROUGHNESS);
			if( nullptr == l_pRoughness ) l_pRoughness = AssetManager::singleton().getAsset(DARK_GRAY_TEXTURE_ASSET_NAME);

			l_pRefraction = AssetManager::singleton().getAsset(DARK_GRAY_TEXTURE_ASSET_NAME);
		}

		std::shared_ptr<Asset> l_pMat = AssetManager::singleton().createRuntimeAsset<MaterialAsset>();
		MaterialAsset *l_pMatInst = l_pMat->getComponent<MaterialAsset>();
		l_pMatInst->init(ProgramManager::singleton().getData(DefaultPrograms::RayScatter));
		l_pMatInst->setTexture(STANDARD_TEXTURE_BASECOLOR, l_pBaseColor);
		l_pMatInst->setTexture(STANDARD_TEXTURE_NORMAL, l_pNormal);
		l_pMatInst->setTexture(STANDARD_TEXTURE_METAL, l_pMetal);
		l_pMatInst->setTexture(STANDARD_TEXTURE_ROUGHNESS, l_pRoughness);
		l_pMatInst->setTexture(STANDARD_TEXTURE_REFRACT, l_pRefraction);
		a_pInst->m_Materials.insert(std::make_pair(MATSLOT_LIGHTMAP, l_pMat));
	}
}
#pragma endregion

}