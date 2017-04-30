// GLSLShader.cpp
//
// 2014/05/27 Ian Wu/Real0000
//

#include "CommonUtil.h"

#include "Module/Program/GLSLShader.h"
#include "Module/Texture/Texture.h"

namespace R
{

#pragma region GLSLCompiler
//
// GLSLCompiler
//
GLuint GLSLCompiler::compile(wxString l_Content, GLenum l_Type)
{
	std::string l_Program("");

#ifdef _DEBUG
	//printf("compiling shader : %s", l_File.c_str());
#endif

	std::vector<wxString> l_Lines;
    splitString(wxT('\n'), l_Content, l_Lines);
	for( unsigned int i=0 ; i<l_Lines.size() ; i++ )
    {
        if (l_Lines[i].find(wxT("#include")) != wxString::npos) l_Lines[i] = parseInclude(SHADER_PATH, l_Lines[i]);
    }
    l_Content.clear();
	for( unsigned int i=0 ; i<l_Lines.size() ; i++ )
	{
        l_Content += l_Lines[i];
        l_Content += wxT("\n");
	}
	
	GLuint l_ShaderID = glCreateShader(l_Type);

    const char *l_pAsciiCode = l_Content.ToAscii();
	glShaderSource(l_ShaderID, 1, (const GLchar **)&(l_pAsciiCode), NULL);
	glCompileShader(l_ShaderID);

#ifdef _DEBUG
	GLint l_Code;
	glGetShaderiv(l_ShaderID, GL_COMPILE_STATUS, &l_Code);

	if( GL_TRUE != l_Code )
	{
		wxMessageBox(wxT("glsl compile error:"));

		GLint l_LogLength = 0;
		glGetShaderiv(l_ShaderID, GL_INFO_LOG_LENGTH, &l_LogLength);
		if( 0 != l_LogLength )
		{
			char *l_Buff = new char[l_LogLength];
			GLint l_CharWritten = 0;
			glGetShaderInfoLog(l_ShaderID, l_LogLength, &l_CharWritten, l_Buff);
			wxMessageBox(l_Buff);

			delete[] l_Buff;
		}

		return 0;
	}

#endif

	return l_ShaderID;
}

wxString GLSLCompiler::parseInclude(wxString l_Path, wxString l_Line)
{
	size_t l_Pos = l_Line.find(wxT("#include")) + wxStrlen(wxT("#include"));
	wxString l_IncludedFile("");

	size_t l_FPos = l_Line.find(wxT('\"'), l_Pos) + 1;
	size_t l_BPos = l_Line.find(wxT('\"'), l_FPos) - 1;
	if( l_FPos == wxString::npos || l_BPos == wxString::npos )
	{
		wxString l_Massage = wxString::Format("invalid include command : %s", (const char *)l_Line.c_str());
		wxMessageBox(l_Massage, wxT("glsl error"));
		return "";
	}

	for( unsigned int i=l_FPos ; i<=l_BPos ; i++ ) l_IncludedFile += l_Line[i];

	std::vector<wxString> l_Lines;
	splitString(wxT(' '), l_IncludedFile, l_Lines);
	if( 1 != l_Lines.size() )
	{
		wxString l_Massage = wxString::Format("invalid include command : %s", (const char *)l_Line.c_str());
		wxMessageBox(l_Massage, wxT("glsl error"));
		return "";
	}

	l_IncludedFile = getAbsolutePath(l_Path, l_Lines[0]);
	
	FILE *l_pFile = NULL;
	l_pFile = fopen(l_IncludedFile.c_str(), "rt");
	if( NULL == l_pFile )
	{
		wxString l_Massage = wxString::Format("invalid include command : %s", (const char *)l_Line.c_str());
		wxMessageBox(l_Massage, wxT("glsl error"));
		return "";
	}

	unsigned int l_FileLength = getFileLength(l_pFile);
	char *l_pFileBuff = new char [l_FileLength + 2];
	memset(l_pFileBuff, 0, l_FileLength + 2);
	fread(l_pFileBuff, sizeof(char), l_FileLength, l_pFile);
	fclose(l_pFile);

	wxString l_NestedPath = getFilePath(l_IncludedFile);

	wxString l_ShaderString = l_pFileBuff;
	delete[] l_pFileBuff;

	l_Lines.clear();
	splitString(wxT('\n'), l_ShaderString, l_Lines);
	for( unsigned int i=0 ; i<l_Lines.size() ; i++ )
		if( l_Lines[i].find(wxT("#include")) != wxString::npos )
			l_Lines[i] = parseInclude(l_Path, l_Lines[i]);

	l_ShaderString.clear();
	for( unsigned int i=0 ; i<l_Lines.size() ; i++ )
	{
		l_ShaderString += l_Lines[i];
		l_ShaderString += wxT("\n");
	}
	return l_ShaderString;
}
#pragma endregion

#pragma region GLSLProgram
//
// GLSLProgram
//
GLSLProgram::Param::Param()
	: m_Desc(wxT("")), m_BlockID(0), m_Offset(0), m_Size(0), m_pDefault(nullptr)
{
}

GLSLProgram::Param::~Param()
{
	SAFE_DELETE(m_pDefault)
}

template<typename T>
void GLSLProgram::Param::init(T l_Param)
{
	assert(nullptr == m_pDefault);
	m_Size = sizeof(T);
	m_pDefault = new char[m_Size];
	memcpy(m_pDefault, &l_Param, m_Size);
}

GLSLProgram::TextureParam::TextureParam()
	: m_Desc(wxT("")), m_pDefault(nullptr), m_Slot(1)
{
}

GLSLProgram::TextureParam::~TextureParam()
{
	if( nullptr != m_pDefault ) TextureManager::singleton().recycle(m_pDefault);
}

GLSLProgram::GLSLProgram()
	: m_Program(0)
    , m_bSkinned(false)
{
	m_Program = glCreateProgram();
}

GLSLProgram::~GLSLProgram()
{
	glDeleteProgram(m_Program);
	for( auto it = m_TextureLinkage.begin() ; it != m_TextureLinkage.end() ; ++it ) delete it->second;
	m_TextureLinkage.clear();

	for( auto it = m_Params.begin() ; it != m_Params.end() ; ++it ) delete it->second;
	m_Params.clear();

	for( auto it = m_StorageBuffer.begin() ; it != m_StorageBuffer.end() ; ++it ) glDeleteBuffers(1, &it->first);
	m_StorageBuffer.clear();
}

void GLSLProgram::init(wxString l_SettingFile)
{
	tinyxml2::XMLDocument l_Doc;
	tinyxml2::XMLError l_Res = l_Doc.LoadFile(l_SettingFile);
    assert(l_Res == tinyxml2::XML_NO_ERROR);

    tinyxml2::XMLElement *l_pRootElement = l_Doc.RootElement();
    m_bSkinned = l_pRootElement->BoolAttribute("skinned");

	tinyxml2::XMLElement *l_pShaderSetting = nullptr;
    tinyxml2::XMLElement *l_pBlockSetting = nullptr;
	tinyxml2::XMLElement *l_pTextureSetting = nullptr;
    for (tinyxml2::XMLElement *l_pChild = l_pRootElement->FirstChildElement(); nullptr != l_pChild; l_pChild = l_pChild->NextSiblingElement())
	{
		if( 0 == wxStrcmp(wxT("Shaders"), l_pChild->Value()) ) l_pShaderSetting = l_pChild;
		else if( 0 == wxStrcmp(wxT("Uniforms"), l_pChild->Value()) ) l_pBlockSetting = l_pChild;
        else if( 0 == wxStrcmp(wxT("Texture"), l_pChild->Value()) ) l_pTextureSetting = l_pChild;
	}

	wxString l_PreDef(wxT("#version 430\n\n"));
	if( nullptr != l_pTextureSetting )
	{
		unsigned int l_Slot = 1;
		const std::map<wxString, wxString> c_FormatMap = {
			std::make_pair(wxT("Unit1D"), wxT("sampler1D​")),
			std::make_pair(wxT("Unit2D"), wxT("sampler2D​")),
			std::make_pair(wxT("Unit3D"), wxT("sampler3D​")),
			std::make_pair(wxT("UnitCube"), wxT("samplerCube​")),
			std::make_pair(wxT("Unit1DShadow"), wxT("sampler1DShadow​​")),
			std::make_pair(wxT("Unit2DShadow"), wxT("sampler2DShadow​​")),
			std::make_pair(wxT("UnitCubeShadow"), wxT("samplerCubeShadow​​")) };
		for( auto it = l_pTextureSetting->FirstChildElement() ; nullptr != it ; it = it->NextSiblingElement() )
		{
			wxString l_Name(it->Attribute("name"));
			TextureParam *l_pNewTexUnit = new TextureParam();
			l_pNewTexUnit->m_Desc = it->Attribute("desc");
			l_pNewTexUnit->m_Slot = l_Slot;
			++l_Slot;
			m_TextureLinkage[l_Name] = l_pNewTexUnit;

			auto l_TexTypeIt = c_FormatMap.find(it->Name());
			assert(l_TexTypeIt != c_FormatMap.end());
			l_PreDef += wxString::Format(wxT("uniform %s %s;\n"), l_TexTypeIt->second, l_Name);
		}
	}
	l_PreDef += wxT("\n");

	unsigned int l_Offset = 0;
	wxString l_UniformBlocks(wxT(""));
	unsigned int l_BlockIdx = 0;
	for( auto it = l_pBlockSetting->FirstChildElement() ; it != nullptr ; it = it->NextSiblingElement() )
	{
		GLuint l_BufferID = 0;
		glGenBuffers(1, &l_BufferID);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, l_BufferID);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, l_BlockIdx, l_BufferID);
		
		l_Offset = 0;
		l_UniformBlocks += wxString::Format(wxT("struct Block%d\n"), l_BlockIdx);
		l_UniformBlocks += wxT("{\n");
		for( auto blockIt = it->FirstChildElement() ; nullptr != blockIt ; blockIt = blockIt->NextSiblingElement() )
		{
			createNewParam(blockIt, false, l_Offset, l_UniformBlocks);
		}
		l_UniformBlocks += wxT("};\n");
		++l_BlockIdx;

		if( 0 != (l_Offset % 16) ) l_Offset += 16 - (l_Offset % 16);
		m_Uniforms.back().second = l_Offset;
	}

	for( unsigned int i=0 ; i<l_BlockIdx ; ++i )
	{
		l_UniformBlocks += wxString::Format(wxT("layout(std140) uniform _UniformBlock%d\n"), i);
		l_UniformBlocks += wxT("{\n");
		l_UniformBlocks += wxString::Format(wxT("\tBlock%d m_Data[];\n"), i);
		l_UniformBlocks += wxString::Format("} UniformBlock%d\n", l_BlockIdx);
	}
	l_UniformBlocks += wxT("\n");

    assert(nullptr != l_pShaderSetting);
    {//init program item
        m_Program = glCreateProgram();

        const std::map<wxString, GLenum> c_ShaderAttrName = {
			std::make_pair(wxT("vertex"), GL_VERTEX_SHADER),
			std::make_pair(wxT("pixel"), GL_FRAGMENT_SHADER),
			std::make_pair(wxT("geometry"), GL_GEOMETRY_SHADER),
			std::make_pair(wxT("tessCtrl"), GL_TESS_CONTROL_SHADER),
			std::make_pair(wxT("tessEval"), GL_TESS_EVALUATION_SHADER),
			std::make_pair(wxT("compute"), GL_COMPUTE_SHADER) };
        for( auto it = l_pShaderSetting->FirstChildElement() ; nullptr != it ; it = it->NextSiblingElement() )
		{
			wxString l_ShaderName = it->Name();
			auto l_TypeIt = c_ShaderAttrName.find(l_ShaderName);
			assert(c_ShaderAttrName.end() != l_TypeIt);

			tinyxml2::XMLText *l_pShaderContent = it->FirstChild()->ToText();
			assert(nullptr != l_pShaderContent);

			wxString l_ShaderContent(l_pShaderContent->Value());
			switch( l_TypeIt->second )
			{
				case GL_VERTEX_SHADER:{
					l_ShaderContent = l_PreDef + l_SharedDef + l_UniformBlocks +
									wxT("in vec3 a_Position;\n")
									wxT("in vec4 a_UVs;\n")
									wxT("in vec3 a_Normal;\n")
									wxT("in vec3 a_Tangent;\n")
									wxT("in vec3 a_Binormal;\n")
									wxT("in vec4 a_BoneID;\n")
									wxT("in vec4 a_BoneWeight;\n")
									wxT("in vec4 a_Color;\n")
									wxT("in int a_DrawID;\n\n") + l_ShaderContent;
					
					GLuint l_Shader = GLSLCompiler::compile(l_ShaderContent, GL_VERTEX_SHADER);
					glAttachShader(m_Program, l_Shader);

					const char *l_pVtxAttrName[] = {"a_Position", "a_UVs", "a_Normal", "a_Tangent", "a_Binormal", "a_BoneID", "a_BoneWeight", "a_DrawID"};
					const unsigned int l_NumVtxAttr = sizeof(l_pVtxAttrName) / sizeof(const char *);
					for( unsigned int i=0 ; i<l_NumVtxAttr ; ++i ) glBindAttribLocation(m_Program, i, l_pVtxAttrName[i]);
					}break;

				case GL_FRAGMENT_SHADER:
				case GL_GEOMETRY_SHADER:
				case GL_TESS_CONTROL_SHADER:
				case GL_TESS_EVALUATION_SHADER:
				case GL_COMPUTE_SHADER:{
					l_ShaderContent = l_PreDef + l_SharedDef + l_UniformBlocks + l_ShaderContent;
					
					GLuint l_Shader = GLSLCompiler::compile(l_ShaderContent, l_TypeIt->second);
					glAttachShader(m_Program, l_Shader);
					}break;
			}
        }


        glLinkProgram(m_Program);
    }

	{// init block and sampler
		char l_Buff[256];
		glUseProgram(m_Program);
		GLint l_UniformBlock = glGetUniformBlockIndex(m_Program, "_GlobalBuffer");
		if( -1 != l_UniformBlock ) glUniformBlockBinding(m_Program, l_UniformBlock, 0);
		l_UniformBlock = glGetUniformBlockIndex(m_Program, "_SharedBlock");
		if( -1 != l_UniformBlock ) glUniformBlockBinding(m_Program, l_UniformBlock, 1);

		if( !m_Uniforms.empty() )
		{
			for( unsigned int i=0 ; i<m_Uniforms.size()-1 ; ++i )
			{
				char l_Buff[256];
				snprintf(l_Buff, 256, "");
			}
		}
		for( int i=UNIFORM_CUSTOM_BLOCK_BASE ; i<l_MaxBlock ; ++i )
		{
			snprintf(l_Buff, 256, "UniformBlock[%d]", i - UNIFORM_CUSTOM_BLOCK_BASE);
			l_UniformBlock = glGetUniformBlockIndex(m_Program, l_Buff);
			if( -1 != l_UniformBlock ) glUniformBlockBinding(m_Program, l_UniformBlock, i);
			else break;
		}
		GLint l_SamplerLocation = glGetUniformLocation(m_Program, "u_WorldTexture");
		if( -1 != l_SamplerLocation ) glUniform1d(l_SamplerLocation, 0);
		for( int i=1 ; i+1<l_MaxTexture ; ++i )
		{
			snprintf(l_Buff, 256, "u_DynamicTexture[%d]", 1);
			l_SamplerLocation = glGetUniformLocation(m_Program, l_Buff);
			if( -1 != l_SamplerLocation ) glUniform1d(l_SamplerLocation, i);
		}
	}

	l_Doc.Clear();
}

void GLSLProgram::use()
{
	glUseProgram(m_Program);
}

void GLSLProgram::createNewParam(tinyxml2::XMLElement *l_pSrc, bool l_bShared, unsigned int &l_Offset, wxString &l_Line)
{
	wxString l_ParamName = l_pSrc->Attribute("name");
	assert( !l_ParamName.empty() );

	Param *l_pNewParam = new Param();
	l_pNewParam->m_BlockID = m_Uniforms.size();
	l_pNewParam->m_Desc = l_pSrc->Attribute("desc");
    unsigned int l_Alignment = 0;
	assert( 0 == wxStrcmp(l_pSrc->Value(), wxT("Param")) );
	
	wxString l_TypeStr = l_pSrc->Attribute("type");
	assert(!l_TypeStr.empty());
	if( 0 == wxStrcmp(l_TypeStr, "int1") )
	{
		l_pNewParam->init(atoi(l_pSrc->Attribute("x")));
		l_Line = wxString::Format(wxT("\tint %s;\n"), l_ParamName);
		l_Alignment = 4;
	}
	else if( 0 == wxStrcmp(l_TypeStr, "int2") )
	{
		glm::ivec2 l_Init(atoi(l_pSrc->Attribute("x")), atoi(l_pSrc->Attribute("y")));
		l_pNewParam->init(l_Init);
		l_Line = wxString::Format(wxT("\tivec2 %s;\n"), l_ParamName);
		l_Alignment = 8;
	}
	else if( 0 == wxStrcmp(l_TypeStr, "int3") )
	{
		glm::ivec3 l_Init(atoi(l_pSrc->Attribute("x")), atoi(l_pSrc->Attribute("y")), atoi(l_pSrc->Attribute("z")));
		l_pNewParam->init(l_Init);
		l_Line = wxString::Format(wxT("\tivec3 %s;\n"), l_ParamName);
		l_Alignment = 16;
	}
	else if( 0 == wxStrcmp(l_TypeStr, "int4") )
	{
		glm::ivec4 l_Init(atoi(l_pSrc->Attribute("x")), atoi(l_pSrc->Attribute("y")), atoi(l_pSrc->Attribute("z")), atoi(l_pSrc->Attribute("w")));
		l_pNewParam->init(l_Init);
		l_Line = wxString::Format(wxT("\tivec4 %s;\n"), l_ParamName);
		l_Alignment = 16;
	}
	else if( 0 == wxStrcmp(l_TypeStr, "float1") )
	{
		l_pNewParam->init(atof(l_pSrc->Attribute("x")));
		l_Line = wxString::Format(wxT("\tfloat %s;\n"), l_ParamName);
		l_Alignment = 4;
	}
	else if( 0 == wxStrcmp(l_TypeStr, "float2") )
	{
		glm::vec2 l_Init(atof(l_pSrc->Attribute("x")), atof(l_pSrc->Attribute("y")));
		l_pNewParam->init(l_Init);
		l_Line = wxString::Format(wxT("\tvec2 %s;\n"), l_ParamName);
		l_Alignment = 8;
	}
	else if( 0 == wxStrcmp(l_TypeStr, "float3") )
	{
		glm::vec3 l_Init(atof(l_pSrc->Attribute("x")), atof(l_pSrc->Attribute("y")), atof(l_pSrc->Attribute("z")));
		l_pNewParam->init(l_Init);
		l_Line = wxString::Format(wxT("\tvec3 %s;\n"), l_ParamName);
		l_Alignment = 16;
	}
	else if( 0 == wxStrcmp(l_TypeStr, "float4") )
	{
		glm::vec4 l_Init(atof(l_pSrc->Attribute("x")), atof(l_pSrc->Attribute("y")), atof(l_pSrc->Attribute("z")), atof(l_pSrc->Attribute("w")));
		l_pNewParam->init(l_Init);
		l_Line = wxString::Format(wxT("\tvec4 %s;\n"), l_ParamName);
		l_Alignment = 16;
	}
	else if( 0 == wxStrcmp(l_TypeStr, "mat4x4") )
	{
		glm::mat4x4 l_Init(atof(l_pSrc->Attribute("m11")), atof(l_pSrc->Attribute("m12")), atof(l_pSrc->Attribute("m13")), atof(l_pSrc->Attribute("m14"))
						, atof(l_pSrc->Attribute("m21")), atof(l_pSrc->Attribute("m22")), atof(l_pSrc->Attribute("m23")), atof(l_pSrc->Attribute("m24"))
						, atof(l_pSrc->Attribute("m31")), atof(l_pSrc->Attribute("m32")), atof(l_pSrc->Attribute("m33")), atof(l_pSrc->Attribute("m34"))
						, atof(l_pSrc->Attribute("m41")), atof(l_pSrc->Attribute("m42")), atof(l_pSrc->Attribute("m43")), atof(l_pSrc->Attribute("m44")));
		l_pNewParam->init(l_Init);
		l_Line = wxString::Format(wxT("\tmat4 %s;\n"), l_ParamName);
		l_Alignment = 16;
	}
	else
	{
		wxString l_Msg(wxString::Format(wxT("Unkown param type at param name \"%s\" with type \"%s\""), l_Name, l_TypeStr));
		wxMessageBox(l_Msg, wxT("Shader Param error"));
	}

	if( 0 != (l_Offset % l_Alignment) ) l_Offset += l_Alignment - (l_Offset % l_Alignment);
		
    l_pNewParam->m_Offset = l_Offset;
    l_Offset += l_pNewParam->m_Size;

	if( l_bShared ) m_Shared[l_ParamName] = l_pNewParam;
	else
	{
		l_Line = wxT("\t") + l_Line;
		m_Params[l_ParamName] = l_pNewParam;
	}
}
#pragma endregion

#pragma region GLSLMaterial
//
// GLSLMaterial
//
GLSLMaterial::GLSLMaterial(GLSLProgram *l_pOwner)
	: m_pRefOwner(l_pOwner)
{
	for( auto it = m_pRefOwner->m_Params.begin() ; it != m_pRefOwner->m_Params.end() ; ++it )
	{
		std::pair<GLSLProgram::Param *, char *> &l_NewParam = m_Params[it->first] = std::make_pair(it->second, new char[it->second->m_Size]);
		memcpy(l_NewParam.second, it->second->m_pDefault, it->second->m_Size);
	}

	for( auto it = m_pRefOwner->m_TextureLinkage.begin() ; it != m_pRefOwner->m_TextureLinkage.end() ; ++it )
	{
		m_Textures[it->first] = std::make_pair(it->second, nullptr);
	}
}

GLSLMaterial::~GLSLMaterial()
{
	for( auto it = m_Params.begin() ; it != m_Params.end() ; ++it ) delete[] it->second.second;
	m_Params.clear();
}

void GLSLMaterial::set(wxString l_Name, TextureUnit *l_pTexture)
{
	assert(m_Textures.find(l_Name) != m_Textures.end());
	assert(m_Textures[l_Name].first->m_pDefault->textureType() == l_pTexture->textureType());
	m_Textures[l_Name].second = l_pTexture;
}

void GLSLMaterial::setDefault(wxString l_Name)
{
	assert(m_Params.find(l_Name) != m_Params.end());
	std::pair<GLSLProgram::Param *, char *> &l_Data = m_Params[l_Name];
	memcpy(l_Data.second, l_Data.first->m_pDefault, l_Data.first->m_Size);
}

void GLSLMaterial::flushParam(int l_Block)
{
	GLSLProgramManager &l_Mgr = GLSLProgramManager::singleton();
	for( auto it = m_Params.begin() ; it != m_Params.end() ; ++it )
	{
		l_Mgr.updateUniform(it->second.second, it->second.first->m_Offset, it->second.first->m_Size, l_Block);
	}
}
#pragma endregion

#pragma region GLSLProgramManager
//
// GLSLProgramManager
//
GLSLProgramManager *GLSLProgramManager::m_pInstance = nullptr;
GLSLProgramManager& GLSLProgramManager::singleton()
{
	if( nullptr == m_pInstance ) m_pInstance = new GLSLProgramManager();
	return *m_pInstance;
}

void GLSLProgramManager::purge()
{
	SAFE_DELETE(m_pInstance);
}

GLSLProgramManager::GLSLProgramManager()
	: m_MaxUniformSize(0)
	, m_MaxTextureStage(0)
    , m_pWorldTexture(nullptr)
{
	memset(m_StdUniformBlock, 0xffffffff, sizeof(GLint) * STD_BLOCK_COUNT);

	glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &m_MaxUniformSize);
	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &m_MaxTextureStage);
}

GLSLProgramManager::~GLSLProgramManager()
{
	clearPrograms();
	if( -1 != m_StdUniformBlock[0] ) glDeleteBuffers(STD_BLOCK_COUNT, m_StdUniformBlock);
}

void GLSLProgramManager::init()
{
	glGenBuffers(STD_BLOCK_COUNT, m_StdUniformBlock);

	for( unsigned int i=0 ; i<STD_BLOCK_COUNT ; ++i )
	{
		glBindBuffer(GL_UNIFORM_BUFFER, m_StdUniformBlock[i]);
		glBufferData(GL_UNIFORM_BUFFER, STD_UNIFORM_BLOCK_SIZE, nullptr, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, i, m_StdUniformBlock[i]);
	}

	m_pWorldTexture = TextureManager::singleton().createNew<Texture2D>();
	m_pWorldTexture->init(WORLD_TEXTURE_SIZE, WORLD_TEXTURE_SIZE, GL_RGBA32F, nullptr, GL_NEAREST, GL_NEAREST);
}

void GLSLProgramManager::clearPrograms()
{
	for( unsigned int i=0 ; i<m_Programs.size() ; i++ ) delete m_Programs[i];
	m_Programs.clear();
	m_ProgramMap.clear();
}

GLSLProgram* GLSLProgramManager::getProgram(unsigned int l_ID)
{
	assert(l_ID < m_Programs.size());
	return m_Programs[l_ID];
}

unsigned int GLSLProgramManager::addProgram(wxString l_SettingFile)
{
	if( m_ProgramMap.end() != m_ProgramMap.find(l_SettingFile) ) return m_ProgramMap[l_SettingFile];

	unsigned int l_Res = m_Programs.size();

	GLSLProgram *l_pNewProgram = new GLSLProgram();
	l_pNewProgram->init(l_SettingFile);
	m_Programs.push_back(l_pNewProgram);
	m_ProgramMap[l_SettingFile] = l_Res;

	return l_Res;
}

void GLSLProgramManager::updateGlobalUniform(void *l_pSrc, unsigned l_Size)
{
	glBindBuffer(GL_UNIFORM_BUFFER, m_StdUniformBlock[STD_BLOCK_GLOBAL]);

	GLvoid *l_Target = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
	memcpy(l_Target, l_pSrc, l_Size);
	glUnmapBuffer(GL_UNIFORM_BUFFER);

	showOpenGLErrorCode(wxT("GLSLProgramManager::updateGlobalUniform"));
}

void GLSLProgramManager::updateWorldTexture(void *l_pSrc, unsigned int l_Instance, unsigned int l_Count)
{
	assert(l_Count <= WORLD_PER_INSTANCE);
	assert(nullptr != m_pWorldTexture);

	unsigned int l_OffsetX = l_Instance * WORLD_PER_INSTANCE * 4;
	unsigned int l_OffsetY = l_OffsetX / WORLD_TEXTURE_SIZE;
	l_OffsetX = l_OffsetX % WORLD_TEXTURE_SIZE;

	m_pWorldTexture->update(l_pSrc, l_OffsetX, l_OffsetY, l_Count * 4, 1, 0);
}
#pragma endregion

}