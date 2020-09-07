// ShaderBase.cpp
//
// 2015/09/25 Ian Wu/Real0000
//

#include "RGDeviceWrapper.h"

namespace R
{

STRING_ENUM_CLASS_INST(ShaderRegType)
STRING_ENUM_CLASS_INST(ShaderStages)

#pragma region ProgramStrctures
//
// RegisterInfo
//
RegisterInfo::RegisterInfo()
	: m_Type(ShaderRegType::ConstBuffer)
	, m_RootIndex(0)
	, m_Slot(0)
	, m_Offset(0)
	, m_Size(0)
	, m_bReserved(false)
{
}

//
// ProgramTextureDesc
//
ProgramTextureDesc::ProgramTextureDesc()
	: m_Describe(wxT(""))
	, m_bWrite(false)
	, m_Type(ShaderParamType::float4)
	, m_pRegInfo(nullptr)
{
}

ProgramTextureDesc::~ProgramTextureDesc()
{
	SAFE_DELETE(m_pRegInfo)
}

//
// ProgramParamDesc
// 
ProgramParamDesc::ProgramParamDesc()
	: m_Describe(wxT(""))
	, m_ArraySize(1)
	, m_Offset(0)
	, m_Type(ShaderParamType::float1)
	, m_pDefault(nullptr)
	, m_pRegInfo(nullptr)
{
}

ProgramParamDesc::~ProgramParamDesc()
{
	SAFE_DELETE_ARRAY(m_pDefault)
	SAFE_DELETE(m_pRegInfo)
}

//
// ProgramBlockDesc
//
ProgramBlockDesc::ProgramBlockDesc()
	: m_bReserved(false)
	, m_bWrite(false)
	, m_StructureName("")
	, m_BlockSize(0)
	, m_pRegInfo(nullptr)
{
}

ProgramBlockDesc::~ProgramBlockDesc()
{
	for( auto it = m_ParamDesc.begin() ; it != m_ParamDesc.end() ; ++it ) delete it->second;
	m_ParamDesc.clear();
	SAFE_DELETE(m_pRegInfo)
}
#pragma endregion

#pragma region ShaderProgram
//
// ShaderProgram
// 
VertexBuffer *ShaderProgram::m_pNullVtxBuffer = nullptr;
int ShaderProgram::m_NullVtxRefCount = 0;
ShaderProgram::ShaderProgram()
	: m_bCompute(false)
	, m_bIndexedDraw(true)
	, m_Name(wxT(""))
{
	if( nullptr == m_pNullVtxBuffer )
	{
		glm::vec3 l_Position(0.0f, 0.0f, 0.0f);
		glm::vec4 l_Texcoord[4] = {glm::zero<glm::vec4>()};
		glm::vec3 l_Normal(0.0f, 1.0f, 0.0f);
		glm::vec3 l_Tangent(1.0f, 0.0f, 0.0f);
		glm::vec3 l_Binormal(0.0f, 0.0f, 1.0f);
		glm::ivec4 l_BoneID(0, 0, 0, 0);
		glm::vec4 l_Weight(1.0f, 0.0f, 0.0f, 0.0f);
		unsigned int l_Colors = 0xffffffff;
		m_pNullVtxBuffer = new VertexBuffer();
		m_pNullVtxBuffer->setName(wxT("null vertex buffer"));
		m_pNullVtxBuffer->setNumVertex(1);
		m_pNullVtxBuffer->setVertex(VTXSLOT_POSITION, &l_Position);
		m_pNullVtxBuffer->setVertex(VTXSLOT_TEXCOORD01, &l_Texcoord[0]);
		m_pNullVtxBuffer->setVertex(VTXSLOT_TEXCOORD23, &l_Texcoord[1]);
		m_pNullVtxBuffer->setVertex(VTXSLOT_TEXCOORD45, &l_Texcoord[2]);
		m_pNullVtxBuffer->setVertex(VTXSLOT_TEXCOORD67, &l_Texcoord[3]);
		m_pNullVtxBuffer->setVertex(VTXSLOT_NORMAL, &l_Normal);
		m_pNullVtxBuffer->setVertex(VTXSLOT_TANGENT, &l_Tangent);
		m_pNullVtxBuffer->setVertex(VTXSLOT_BINORMAL, &l_Binormal);
		m_pNullVtxBuffer->setVertex(VTXSLOT_BONE, &l_BoneID);
		m_pNullVtxBuffer->setVertex(VTXSLOT_WEIGHT, &l_Weight);
		m_pNullVtxBuffer->setVertex(VTXSLOT_COLOR, &l_Colors);
		m_pNullVtxBuffer->init();
	}
	++m_NullVtxRefCount;
}

ShaderProgram::~ShaderProgram()
{
	m_RegMap.clear();

	for( auto it = m_TextureDesc.begin() ; it != m_TextureDesc.end() ; ++it ) delete it->second;
	m_TextureDesc.clear();

	for( unsigned int i = ShaderRegType::ConstBuffer ; i <= ShaderRegType::UavBuffer ; ++i )
	{
		auto &l_Container = getBlockDesc((ShaderRegType::Key)i);
		for( unsigned int j=0 ; j<l_Container.size() ; ++j ) delete l_Container[j];
		l_Container.clear();
	}
	
	--m_NullVtxRefCount;
	if( 0 == m_NullVtxRefCount )
	{
		SAFE_DELETE(m_pNullVtxBuffer)
	}
}

void ShaderProgram::setup(wxString a_Name, boost::property_tree::ptree &a_Root)
{
	m_Name = getFileName(a_Name);

	const std::pair<int, int> c_ShaderModelList[] = {
		std::make_pair(6, 0),
		std::make_pair(5, 1),
		std::make_pair(5, 0),
		std::make_pair(4, 1),
		std::make_pair(4, 0),
		std::make_pair(3, 0)};
	const unsigned int c_NumShaderModel = sizeof(c_ShaderModelList) / sizeof(const std::pair<int, int>);

	m_bCompute = a_Root.get<bool>("root.<xmlattr>.isCompute");
	m_bIndexedDraw = a_Root.get<bool>("root.<xmlattr>.isIndexed");

	boost::property_tree::ptree &l_ParamSetting = a_Root.get_child("root.ParamList");
	boost::property_tree::ptree &l_Shaders = a_Root.get_child("root.Shaders");

	// find max valid shader model block
	{
		std::pair<int, int> l_DeviceSM = GDEVICE()->maxShaderModel();

		char l_Buff[256];
		unsigned int l_Start = 0;
		for( ; l_Start<c_NumShaderModel ; ++l_Start )
		{
			if( l_DeviceSM == c_ShaderModelList[l_Start] ) break;
		}
		for( unsigned int i=l_Start ; i<c_NumShaderModel ; ++i )
		{
			snprintf(l_Buff, 256, "Model_%d_%d", c_ShaderModelList[i].first, c_ShaderModelList[i].second);
			if( l_Shaders.not_found() != l_Shaders.find(l_Buff) )
			{
				m_ShaderInUse = c_ShaderModelList[i];
				break;
			}
		}
	}
	
	// init constant blocks
	unsigned int l_ParamOffset = 0;
	for( auto it = l_ParamSetting.begin() ; it != l_ParamSetting.end() ; ++it )
	{
		ShaderRegType::Key l_RegType = ShaderRegType::fromString(it->first);
		switch( l_RegType )
		{
			case ShaderRegType::ConstBuffer:
			case ShaderRegType::UavBuffer:{
				ProgramBlockDesc *l_pNewBlock = ProgramManager::singleton().createBlockFromDesc(it->second);
				getBlockDesc(l_RegType).push_back(l_pNewBlock);
				l_pNewBlock->m_bReserved = l_RegType == ShaderRegType::UavBuffer || it->second.get("<xmlattr>.reserved", "false") == "true";
				}break;

			case ShaderRegType::Srv2D:
			case ShaderRegType::Srv2DArray:
			case ShaderRegType::Srv3D:
			case ShaderRegType::SrvCube:
			case ShaderRegType::SrvCubeArray:{
				std::string l_Name(it->second.get<std::string>("<xmlattr>.name"));
				ProgramTextureDesc *l_pNewTexture = new ProgramTextureDesc();
				l_pNewTexture->m_bWrite = it->second.get("<xmlattr>.write", "false") == "true";
				l_pNewTexture->m_Type = ShaderParamType::fromString(it->second.get<std::string>("<xmlattr>.type", "float4"));
				l_pNewTexture->m_Describe = it->second.get("<xmlattr>.desc", "");
				l_pNewTexture->m_bReserved = it->second.get("<xmlattr>.reserved", "false") == "true";

				assert(m_TextureDesc.end() == m_TextureDesc.find(l_Name));
				m_TextureDesc.insert(std::make_pair(l_Name, l_pNewTexture));
				}break;

			default: break;
		}
	}

	init(a_Root);

	for( unsigned int i=ShaderRegType::ConstBuffer ; i<=ShaderRegType::UavBuffer ; ++i )
	{
		std::vector<ProgramBlockDesc *> &l_BlockList = m_BlockDesc[i - ShaderRegType::ConstBuffer];
		for( unsigned int j=0 ; j<l_BlockList.size() ; ++j )
		{
			ProgramBlockDesc *l_pCurrBlock = l_BlockList[j];
			if( nullptr != l_pCurrBlock->m_pRegInfo )
			{
				m_RegMap.insert(std::make_pair(l_pCurrBlock->m_Name, l_pCurrBlock->m_pRegInfo));
			}

			for( auto it = l_pCurrBlock->m_ParamDesc.begin() ; it != l_pCurrBlock->m_ParamDesc.end() ; ++it )
			{
				if( nullptr == it->second->m_pRegInfo ) continue;
				m_RegMap.insert(std::make_pair(it->first, it->second->m_pRegInfo));
			}
		}
	}
}

std::vector<ProgramBlockDesc *>& ShaderProgram::getBlockDesc(ShaderRegType::Key a_Type)
{
	unsigned int l_Idx = a_Type - ShaderRegType::ConstBuffer;
	assert(l_Idx < 3);
	return m_BlockDesc[l_Idx];
}

void ShaderProgram::assignIndirectDrawCommand(unsigned int &a_Offset, char *a_pOutput, IndirectDrawData a_DrawInfo)
{
	assignIndirectDrawCommand(a_Offset, a_pOutput, a_DrawInfo.m_IndexCount, a_DrawInfo.m_InstanceCount, a_DrawInfo.m_StartIndex, a_DrawInfo.m_BaseVertex, a_DrawInfo.m_StartInstance);
}

RegisterInfo* ShaderProgram::getRegisterInfo(std::string a_Name)
{
	auto it = m_RegMap.find(a_Name);
	if( m_RegMap.end() == it ) return nullptr;
	return it->second;
}

ProgramBlockDesc* ShaderProgram::newConstBlockDesc()
{
	auto &l_TargetContainer = getBlockDesc(ShaderRegType::Constant);
	l_TargetContainer.push_back(new ProgramBlockDesc());
	return l_TargetContainer.back();
}
#pragma endregion

#pragma region ProgramManager
//
// ProgramManager
//
ProgramManagerComponent* ProgramManager::m_pShaderComponent = nullptr;
void ProgramManager::init(ProgramManagerComponent *a_pComponent)
{
	assert(nullptr == m_pShaderComponent);
	m_pShaderComponent = a_pComponent;

	wxString l_Path(wxGetCwd());
	l_Path.Replace("\\", "/");
	if( !l_Path.EndsWith("/") ) l_Path += wxT("/");
	ProgramManager::singleton().addSearchPath(l_Path + SHADER_PATH);
	ProgramManager::singleton().initBlockDefine(l_Path + SHADER_PATH + SAHDER_BLOCK_DEFINE_FILE);
	ProgramManager::singleton().initDefaultProgram();
}

ProgramManager& ProgramManager::singleton()
{
	static ProgramManager s_Inst;
	return s_Inst;
}

ProgramManager::ProgramManager()
	: SearchPathSystem(std::bind(&ProgramManager::loadFile, this, std::placeholders::_1, std::placeholders::_2), std::bind(&ProgramManager::allocator, this))
{	
}

ProgramManager::~ProgramManager()
{
	m_BlockDefineMap.clear();
	m_BlockDefineFile.clear();
	SAFE_DELETE(m_pShaderComponent);
}

void* ProgramManager::getShader(ShaderProgram *a_pProgrom, wxString a_Filename, ShaderStages::Key a_Stage, std::pair<int, int> a_Module, std::map<std::string, std::string> &a_ParamDefine)
{
	assert(nullptr != m_pShaderComponent);
	return m_pShaderComponent->getShader(a_pProgrom, a_Filename, a_Stage, a_Module, a_ParamDefine);
}

unsigned int ProgramManager::calculateParamOffset(unsigned int &a_Offset, ShaderParamType::Key a_Type, unsigned int a_ArraySize)
{
	assert(nullptr != m_pShaderComponent);
	return m_pShaderComponent->calculateParamOffset(a_Offset, a_Type, a_ArraySize);
}

ProgramBlockDesc* ProgramManager::createBlockFromDesc(boost::property_tree::ptree &a_Node)
{
	ProgramBlockDesc *l_pNewBlock = new ProgramBlockDesc();
	l_pNewBlock->m_Name = a_Node.get<std::string>("<xmlattr>.name");
	l_pNewBlock->m_bWrite = a_Node.get<std::string>("<xmlattr>.write", "false") == "true";// only uav use this flag
	l_pNewBlock->m_StructureName = a_Node.get<std::string>("<xmlattr>.preDef", "");
	assert(!l_pNewBlock->m_Name.empty());
	
	boost::property_tree::ptree *l_pTargetNode = nullptr;
	{
		for( auto l_ParamIt = a_Node.begin() ; l_ParamIt != a_Node.end() ; ++l_ParamIt )
		{
			if( "<xmlattr>" == l_ParamIt->first ) continue;
			l_pTargetNode = &a_Node;
			break;
		}
		if( nullptr == l_pTargetNode )
		{
			std::string l_DefName(a_Node.get<std::string>("<xmlattr>.preDef"));
			assert(m_BlockDefineMap.find(l_DefName) != m_BlockDefineMap.end());
			l_pTargetNode = m_BlockDefineMap[l_DefName];
		}
	}

	unsigned int l_ParamOffset = 0;
	for( auto l_ParamIt = l_pTargetNode->begin() ; l_ParamIt != l_pTargetNode->end() ; ++l_ParamIt )
	{
		if( "<xmlattr>" == l_ParamIt->first ) continue;

		ProgramParamDesc *l_pNewParam = new ProgramParamDesc();
		std::string l_ParamName(l_ParamIt->second.get<std::string>("<xmlattr>.name"));
		assert( !l_ParamName.empty() );
		l_pNewBlock->m_ParamDesc[l_ParamName] = l_pNewParam;

		auto l_ParamAttr = l_ParamIt->second.get_child("<xmlattr>");
		ShaderParamType::Key l_ParamType = ShaderParamType::fromString(l_ParamAttr.get<std::string>("type"));
		unsigned int l_ParamSize = GDEVICE()->getParamAlignmentSize(l_ParamType);
		
		l_pNewParam->m_ArraySize = l_ParamAttr.get<unsigned int>("size", 1);
		l_pNewParam->m_Describe = l_ParamAttr.get<std::string>("desc", "");
		l_pNewParam->m_Type = l_ParamType;
		l_pNewParam->m_pDefault = new char[l_ParamSize];
		
		if( 0 == l_ParamAttr.count("init") ) 
		{
			memset(l_pNewParam->m_pDefault, 0, GDEVICE()->getParamAlignmentSize(l_ParamType));
		}
		else parseShaderParamValue(l_ParamType, l_ParamAttr.get<std::string>("init"), l_pNewParam->m_pDefault);

		l_pNewParam->m_Offset = ProgramManager::singleton().calculateParamOffset(l_ParamOffset, l_ParamType, l_pNewParam->m_ArraySize);
	}
	if( 0 != (l_ParamOffset % (sizeof(float) * 4)) ) l_ParamOffset += sizeof(float) * 4 - (l_ParamOffset % (sizeof(float) * 4));
	l_pNewBlock->m_BlockSize = l_ParamOffset;
	return l_pNewBlock;
}

ProgramBlockDesc* ProgramManager::createBlockFromDesc(std::string a_BlockName)
{
	assert(m_BlockDefineMap.find(a_BlockName) != m_BlockDefineMap.end());
	return createBlockFromDesc(*(m_BlockDefineMap[a_BlockName]));
}

std::shared_ptr<ShaderProgram> ProgramManager::allocator()
{
	return std::shared_ptr<ShaderProgram>(m_pShaderComponent->newProgram());
}

void ProgramManager::loadFile(std::shared_ptr<ShaderProgram> a_pInst, wxString a_Path)
{
	boost::property_tree::ptree l_XMLTree;
	boost::property_tree::xml_parser::read_xml(static_cast<const char *>(a_Path.c_str()), l_XMLTree, boost::property_tree::xml_parser::no_comments);
	assert( !l_XMLTree.empty() );
	a_pInst->setup(getFileName(a_Path), l_XMLTree);
}

void ProgramManager::initBlockDefine(wxString a_Filepath)
{
	boost::property_tree::xml_parser::read_xml(static_cast<const char *>(a_Filepath.c_str()), m_BlockDefineFile, boost::property_tree::xml_parser::no_comments);
	assert( !m_BlockDefineFile.empty() );

	boost::property_tree::ptree &l_Root = m_BlockDefineFile.get_child("root");
	for( auto it = l_Root.begin() ; it != l_Root.end() ; ++it )
	{
		m_BlockDefineMap[it->second.get<std::string>("<xmlattr>.name")] = &it->second;
	}
}

void ProgramManager::initDefaultProgram()
{
	for( unsigned int i=0 ; i<DefaultPrograms::num_default_program ; ++i )
	{
		wxString l_FileToCompile(DefaultPrograms::toString((DefaultPrograms::Key)i) + wxT(".xml"));
		getData(l_FileToCompile);
	}
}
#pragma endregion

bool drawDataCompare(const IndirectDrawData *a_pLeft, const IndirectDrawData *a_pRight)
{
	if( a_pLeft->m_IndexCount < a_pRight->m_IndexCount ) return true;
	else if( a_pLeft->m_IndexCount > a_pRight->m_IndexCount ) return false;
	else if( a_pLeft->m_StartIndex < a_pRight->m_StartIndex ) return true;
	else if( a_pLeft->m_StartIndex > a_pRight->m_StartIndex ) return false;
	return a_pLeft->m_BaseVertex < a_pRight->m_BaseVertex;
}

std::string convertParamValue(ShaderParamType::Key a_Type, char *a_pSrc)
{
	char l_Buff[1024];
	switch( a_Type )
	{
		case ShaderParamType::int1:{
			snprintf(l_Buff, 1024, "%d", *reinterpret_cast<int *>(a_pSrc));
			}break;

		case ShaderParamType::int2:{
			int *l_pConvert = reinterpret_cast<int *>(a_pSrc);
			snprintf(l_Buff, 1024, "(%d,%d)", l_pConvert[0], l_pConvert[1]);
			}break;

		case ShaderParamType::int3:{
			int *l_pConvert = reinterpret_cast<int *>(a_pSrc);
			snprintf(l_Buff, 1024, "(%d,%d,%d)", l_pConvert[0], l_pConvert[1], l_pConvert[2]);
			}break;
						
		case ShaderParamType::int4:{
			int *l_pConvert = reinterpret_cast<int *>(a_pSrc);
			snprintf(l_Buff, 1024, "(%d,%d,%d,%d)", l_pConvert[0], l_pConvert[1], l_pConvert[2], l_pConvert[3]);
			}break;

		case ShaderParamType::float1:{
			snprintf(l_Buff, 1024, "%f", *reinterpret_cast<float*>(a_pSrc));
			}break;
						
		case ShaderParamType::float2:{
			float *l_pConvert = reinterpret_cast<float*>(a_pSrc);
			snprintf(l_Buff, 1024, "(%f,%f)", l_pConvert[0], l_pConvert[1]);
			}break;

		case ShaderParamType::float3:{
			float *l_pConvert = reinterpret_cast<float*>(a_pSrc);
			snprintf(l_Buff, 1024, "(%f,%f,%f)", l_pConvert[0], l_pConvert[1], l_pConvert[2]);
			}break;
						
		case ShaderParamType::float4:{
			float *l_pConvert = reinterpret_cast<float*>(a_pSrc);
			snprintf(l_Buff, 1024, "(%f,%f,%f,%f)", l_pConvert[0], l_pConvert[1], l_pConvert[2], l_pConvert[3]);
			}break;
						
		case ShaderParamType::float3x3:{
			float *l_pConvert = reinterpret_cast<float*>(a_pSrc);
			snprintf(l_Buff, 1024, "(%f,%f,%f),(%f,%f,%f),(%f,%f,%f)"
				, l_pConvert[0], l_pConvert[1], l_pConvert[2]
				, l_pConvert[3], l_pConvert[4], l_pConvert[5]
				, l_pConvert[6], l_pConvert[7], l_pConvert[8]);
			}break;
						
		case ShaderParamType::float4x4:{
			float *l_pConvert = reinterpret_cast<float*>(a_pSrc);
			snprintf(l_Buff, 1024, "(%f,%f,%f,%f),(%f,%f,%f,%f),(%f,%f,%f,%f),(%f,%f,%f,%f)"
				, l_pConvert[0], l_pConvert[1], l_pConvert[2], l_pConvert[3]
				, l_pConvert[4], l_pConvert[5], l_pConvert[6], l_pConvert[7]
				, l_pConvert[8], l_pConvert[9], l_pConvert[10], l_pConvert[11]
				, l_pConvert[12], l_pConvert[13], l_pConvert[14], l_pConvert[15]);
			}break;

		default:
			assert(false && "invalid param type");
			break;
	}
	return l_Buff;
}

void parseShaderParamValue(ShaderParamType::Key a_Type, std::string a_Src, char *a_pDst)
{
	switch( a_Type )
	{
		case ShaderParamType::int1:
			*reinterpret_cast<int *>(a_pDst) = atoi(a_Src.c_str());
			break;

		case ShaderParamType::int2:
			sscanf(a_Src.c_str(), "(%d,%d)"
				, a_pDst + sizeof(int) * 0, a_pDst + sizeof(int) * 1);
			break;

		case ShaderParamType::int3:
			sscanf(a_Src.c_str(), "(%d,%d,%d)"
				, a_pDst + sizeof(int) * 0, a_pDst + sizeof(int) * 1, a_pDst + sizeof(int) * 2);
			break;
						
		case ShaderParamType::int4:
			sscanf(a_Src.c_str(), "(%d,%d,%d,%d)"
				, a_pDst + sizeof(int) * 0, a_pDst + sizeof(int) * 1, a_pDst + sizeof(int) * 2, a_pDst + sizeof(int) * 3);
			break;

		case ShaderParamType::float1:
			*reinterpret_cast<float *>(a_pDst) = atof(a_Src.c_str());
			break;
						
		case ShaderParamType::float2:
			sscanf(a_Src.c_str(), "(%f,%f)"
				, a_pDst + sizeof(float) * 0, a_pDst + sizeof(float) * 1);
			break;

		case ShaderParamType::float3:
			sscanf(a_Src.c_str(), "(%f,%f,%f)"
				, a_pDst + sizeof(float) * 0, a_pDst + sizeof(float) * 1, a_pDst + sizeof(float) * 2);
			break;
						
		case ShaderParamType::float4:
			sscanf(a_Src.c_str(), "(%f,%f,%f,%f)"
				, a_pDst + sizeof(float) * 0, a_pDst + sizeof(float) * 1, a_pDst + sizeof(float) * 2, a_pDst + sizeof(float) * 3);
			break;
						
		case ShaderParamType::float3x3:
			sscanf(a_Src.c_str(), "(%f,%f,%f),(%f,%f,%f),(%f,%f,%f)"
				, a_pDst + sizeof(float) * 0, a_pDst + sizeof(float) * 1, a_pDst + sizeof(float) * 2
				, a_pDst + sizeof(float) * 3, a_pDst + sizeof(float) * 4, a_pDst + sizeof(float) * 5
				, a_pDst + sizeof(float) * 6, a_pDst + sizeof(float) * 7, a_pDst + sizeof(float) * 8);
			break;
						
		case ShaderParamType::float4x4:
			sscanf(a_Src.c_str(), "(%f,%f,%f,%f),(%f,%f,%f,%f),(%f,%f,%f,%f),(%f,%f,%f,%f)"
				, a_pDst + sizeof(float) * 0, a_pDst + sizeof(float) * 1, a_pDst + sizeof(float) * 2, a_pDst + sizeof(float) * 3
				, a_pDst + sizeof(float) * 4, a_pDst + sizeof(float) * 5, a_pDst + sizeof(float) * 6, a_pDst + sizeof(float) * 7
				, a_pDst + sizeof(float) * 8, a_pDst + sizeof(float) * 9, a_pDst + sizeof(float) * 10, a_pDst + sizeof(float) * 11
				, a_pDst + sizeof(float) * 12, a_pDst + sizeof(float) * 13, a_pDst + sizeof(float) * 14, a_pDst + sizeof(float) * 15);
			break;

		default:
			assert(false && "invalid param type");
			break;
	}
}

}