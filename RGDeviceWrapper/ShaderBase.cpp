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
// ProgramTextureDesc
//
ProgramTextureDesc::ProgramTextureDesc()
	: m_Describe(wxT(""))
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
	, m_Offset(0)
	, m_Type(ShaderParamType::float1)
	, m_pDefault(nullptr)
	, m_pRegInfo(nullptr)
{
}

ProgramParamDesc::~ProgramParamDesc()
{
	SAFE_DELETE(m_pDefault)
	SAFE_DELETE(m_pRegInfo)
}

//
// ProgramBlockDesc
//
ProgramBlockDesc::ProgramBlockDesc()
	: m_BlockSize(0)
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
ShaderProgram::ShaderProgram()
	: m_bCompute(false)
	, m_bIndexedDraw(true)
{
}

ShaderProgram::~ShaderProgram()
{
	for( auto it = m_TextureDesc.begin() ; it != m_TextureDesc.end() ; ++it ) delete it->second;
	m_TextureDesc.clear();

	for( auto it = m_BlockDesc.begin() ; it != m_BlockDesc.end() ; ++it ) delete it->second;
	m_BlockDesc.clear();

	for( unsigned int i=0 ; i<m_ConstBlockDesc.size() ; ++i ) delete m_ConstBlockDesc[i];
	m_ConstBlockDesc.clear();

	for( unsigned int i=0 ; i<m_StorageDesc.size() ; ++i ) delete m_StorageDesc[i];
	m_StorageDesc.clear();
}

void ShaderProgram::setup(boost::property_tree::ptree &a_Root)
{
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
		if( ShaderRegType::toString(ShaderRegType::ConstBuffer) != it->first ) continue;

		ProgramBlockDesc *l_pNewBlock = new ProgramBlockDesc();
		std::string l_BlockName = it->second.get<std::string>("<xmlattr>.name");
		assert(!l_BlockName.empty());
		assert(m_BlockDesc.find(l_BlockName) == m_BlockDesc.end());
		m_BlockDesc.insert(std::make_pair(l_BlockName, l_pNewBlock));

		if( it->second.get("<xmlattr>.reserved", "false") == "true" )
		{
			l_pNewBlock->m_BlockSize = 0;
			continue;
		}

		l_ParamOffset = 0;
		for( auto l_ParamIt = it->second.begin() ; l_ParamIt != it->second.end() ; ++l_ParamIt )
		{
			ProgramParamDesc *l_pNewParam = new ProgramParamDesc();

			std::string l_ParamName(l_ParamIt->second.get<std::string>("<xmlattr>.name"));
			assert( !l_ParamName.empty() );
			l_pNewBlock->m_ParamDesc[l_ParamName] = l_pNewParam;

			auto l_ParamAttr = l_ParamIt->second.get_child("<xmlattr>");
			ShaderParamType::Key l_ParamType = ShaderParamType::fromString(l_ParamAttr.get<std::string>("type"));
			unsigned int l_ParamSize = GDEVICE()->getParamAlignmentSize(l_ParamType);
			unsigned int l_ArraySize = l_ParamAttr.get<unsigned int>("size", 1);
			if( l_ArraySize > 1 )
			{
				for( unsigned int i=0 ; i<l_ArraySize ; ++i )
				{	
					std::string l_ArrayParam(wxString::Format(wxT("%s[%d]"), l_ParamName.c_str(), i).c_str());

					ProgramParamDesc *l_pNewParam = new ProgramParamDesc();
					l_pNewBlock->m_ParamDesc[l_ArrayParam] = l_pNewParam;

					l_pNewParam->m_Describe = l_ParamAttr.get<std::string>("desc", "");
					l_pNewParam->m_Type = l_ParamType;
					l_pNewParam->m_pDefault = new char[l_ParamSize];
					parseInitValue(l_ParamType, l_ParamAttr, l_pNewParam->m_pDefault);

					l_pNewParam->m_Offset = initParamOffset(l_ParamOffset, l_ParamType);
				}
			}
			else
			{
				ProgramParamDesc *l_pNewParam = new ProgramParamDesc();
				l_pNewBlock->m_ParamDesc[l_ParamName] = l_pNewParam;

				l_pNewParam->m_Describe = l_ParamAttr.get<std::string>("desc", "");
				l_pNewParam->m_Type = l_ParamType;
				l_pNewParam->m_pDefault = new char[l_ParamSize];
				parseInitValue(l_ParamType, l_ParamAttr, l_pNewParam->m_pDefault);

				l_pNewParam->m_Offset = initParamOffset(l_ParamOffset, l_ParamType);
			}
		}
		if( 0 != (l_ParamOffset % (sizeof(float) * 4)) ) l_ParamOffset += sizeof(float) * 4 - (l_ParamOffset % (sizeof(float) * 4));
		l_pNewBlock->m_BlockSize = l_ParamOffset;
	}

	init(a_Root);
}

void ShaderProgram::parseInitValue(ShaderParamType::Key a_Type, boost::property_tree::ptree &a_Src, char *a_pDst)
{
	if( 0 == a_Src.count("init") ) 
	{
		memset(a_pDst, 0, GDEVICE()->getParamAlignmentSize(a_Type));
		return;
	}

	switch( a_Type )
	{
		case ShaderParamType::int1:
			*(int *)a_pDst = a_Src.get<int>("init");
			break;

		case ShaderParamType::int2:
			sscanf(a_Src.get<std::string>("init").c_str(), "(%d,%d)"
				, a_pDst + sizeof(int) * 0, a_pDst + sizeof(int) * 1);
			break;

		case ShaderParamType::int3:
			sscanf(a_Src.get<std::string>("init").c_str(), "(%d,%d,%d)"
				, a_pDst + sizeof(int) * 0, a_pDst + sizeof(int) * 1, a_pDst + sizeof(int) * 2);
			break;
						
		case ShaderParamType::int4:
			sscanf(a_Src.get<std::string>("init").c_str(), "(%d,%d,%d,%d)"
				, a_pDst + sizeof(int) * 0, a_pDst + sizeof(int) * 1, a_pDst + sizeof(int) * 2, a_pDst + sizeof(int) * 3);
			break;

		case ShaderParamType::float1:
			*(float *)a_pDst = a_Src.get<float>("init");
			break;
						
		case ShaderParamType::float2:
			sscanf(a_Src.get<std::string>("init").c_str(), "(%f,%f)"
				, a_pDst + sizeof(float) * 0, a_pDst + sizeof(float) * 1);
			break;

		case ShaderParamType::float3:
			sscanf(a_Src.get<std::string>("init").c_str(), "(%f,%f,%f)"
				, a_pDst + sizeof(float) * 0, a_pDst + sizeof(float) * 1, a_pDst + sizeof(float) * 2);
			break;
						
		case ShaderParamType::float4:
			sscanf(a_Src.get<std::string>("init").c_str(), "(%f,%f,%f,%f)"
				, a_pDst + sizeof(float) * 0, a_pDst + sizeof(float) * 1, a_pDst + sizeof(float) * 2, a_pDst + sizeof(float) * 3);
			break;
						
		case ShaderParamType::float3x3:
			sscanf(a_Src.get<std::string>("init").c_str(), "(%f,%f,%f),(%f,%f,%f),(%f,%f,%f)"
				, a_pDst + sizeof(float) * 0, a_pDst + sizeof(float) * 1, a_pDst + sizeof(float) * 2
				, a_pDst + sizeof(float) * 3, a_pDst + sizeof(float) * 4, a_pDst + sizeof(float) * 5
				, a_pDst + sizeof(float) * 6, a_pDst + sizeof(float) * 7, a_pDst + sizeof(float) * 8);
			break;
						
		case ShaderParamType::float4x4:
			sscanf(a_Src.get<std::string>("init").c_str(), "(%f,%f,%f,%f),(%f,%f,%f,%f),(%f,%f,%f,%f),(%f,%f,%f,%f)"
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
	ProgramManager::singleton();// to init default program
}

ProgramManager& ProgramManager::singleton()
{
	static ProgramManager s_Inst;
	return s_Inst;
}

ProgramManager::ProgramManager()
	: SearchPathSystem(std::bind(&ProgramManager::loadFile, this, std::placeholders::_1, std::placeholders::_2), std::bind(&ProgramManager::allocator, this))
{	
	wxString l_Path(wxGetCwd());
	l_Path.Replace("\\", "/");
	if( !l_Path.EndsWith("/") ) l_Path += wxT("/");
	addSearchPath(l_Path + SHADER_PATH);

	initDefaultProgram();
}

ProgramManager::~ProgramManager()
{
	SAFE_DELETE(m_pShaderComponent);
}

void* ProgramManager::getShader(wxString a_Filename, ShaderStages::Key a_Stage, std::pair<int, int> a_Module, std::map<std::string, std::string> &a_ParamDefine)
{
	assert(nullptr != m_pShaderComponent);
	return m_pShaderComponent->getShader(a_Filename, a_Stage, a_Module, a_ParamDefine);
}

std::shared_ptr<ShaderProgram> ProgramManager::allocator()
{
	return std::shared_ptr<ShaderProgram>(m_pShaderComponent->newProgram());;
}

void ProgramManager::loadFile(std::shared_ptr<ShaderProgram> a_pInst, wxString a_Path)
{
	boost::property_tree::ptree l_XMLTree;
	boost::property_tree::xml_parser::read_xml((const char *)a_Path.c_str(), l_XMLTree);
	assert( !l_XMLTree.empty() );
	a_pInst->setup(l_XMLTree);
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

}