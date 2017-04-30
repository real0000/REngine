// GLSLShader.h
//
// 2014/05/27 Ian Wu/Real0000
//

#ifndef _GLSLSHADER_H_
#define _GLSLSHADER_H_

namespace R
{

class BaseCanvas;
class GLSLProgram;
class GLSLMaterial;
class GLSLProgramManager;
class Texture1D;
class Texture2D;
class TextureUnit;
class SceneNode;

class GLSLCompiler
{
public:
	static GLuint compile(wxString l_Content, GLenum l_Type);

private:
	static wxString parseInclude(wxString l_Path, wxString l_Line);
};

//
// 1 Block - 1 uniform buffer, n uniform, n instance
// name - uniform(block id, block offset
//

class GLSLProgram
{
	friend class GLSLMaterial;
	friend class GLSLProgramManager;
public:
	struct Param
	{
		Param();
		~Param();
		
		template<typename T> void init(T l_Param);

		wxString m_Desc;
		unsigned int m_Offset;
		unsigned int m_Size;
		unsigned int m_BlockID;
		void *m_pDefault;
	};
	struct TextureParam
	{
		TextureParam();
		~TextureParam();

		wxString m_Desc;
		TextureUnit *m_pDefault;
		GLenum m_Slot;
		GLenum m_Type;
	};
public:

	void init(wxString l_SettingFile);
	void use();
    bool isSkinned(){ return m_bSkinned; }
	
private:
	GLSLProgram();
	virtual ~GLSLProgram();

	void createNewParam(tinyxml2::XMLElement *l_pSrc, bool l_bShared, unsigned int &l_Offset, wxString &l_Line);

	GLuint m_Program;
    bool m_bSkinned;
	std::map<GLuint, unsigned int> m_StorageBuffer;//buffer id, stride
	std::map<wxString, TextureParam *> m_TextureLinkage;//texture slot map
	std::map<wxString, Param *> m_Params;
};

typedef std::pair<GLSLProgram::Param *, char *> GLMaterialParam;
typedef std::pair<GLSLProgram::TextureParam *, TextureUnit *> GLMaterialTexutre;
class GLSLMaterial
{
public:
	GLSLMaterial(GLSLProgram *l_pOwner);
	virtual ~GLSLMaterial();

	template<typename T>
	void set(wxString l_Name, T l_Param)
	{
		assert(m_Params.find(l_Name) != m_Params.end());
		std::pair<GLSLProgram::Param *, char *> &l_Data = m_Params[l_Name];
		assert(l_Data.first->m_Size == sizeof(T));
		memcpy(l_Data.second, &l_Param, l_Data.first->m_Size);
	}
	void set(wxString l_Name, TextureUnit *l_pTexture);
	void setDefault(wxString l_Name);
	void flushParam(int l_Block);
	const std::map<wxString, GLMaterialTexutre>& getTextureMap(){ return m_Textures; }

private:
	std::map<wxString, GLMaterialParam> m_Params;
	std::map<wxString, GLMaterialTexutre> m_Textures;
	GLSLProgram *m_pRefOwner;
};

/*
*	per context per manager, per manager per uniform block
*	uniform block size and usage :
*		0 : 512 byte, base view, project, view projection, invert view, camera param, time, random
*		1 : 512 byte, custom shared variable(as shadow map usage)
*		2... : -- byte, dynamic shader binding
*/
#define STD_UNIFORM_BLOCK_SIZE 512
#define WORLD_PER_INSTANCE 32
#define WORLD_TEXTURE_SIZE 256
class GLSLProgramManager
{
public:
	enum
	{
		STD_BLOCK_GLOBAL = 0,

		STD_BLOCK_COUNT
	};
public:
	static GLSLProgramManager& singleton();
	static void purge();

	//
	// program part
	//
	void init();
	void clearPrograms();
	GLSLProgram* getProgram(unsigned int l_ID);
	unsigned int addProgram(wxString l_SettingFile);
	void updateGlobalUniform(void *l_pSrc, unsigned int l_Size);
	void updateWorldTexture(void *l_pSrc, unsigned int l_Instance, unsigned int l_Count);

	GLint getMaxUniformSize(){ return m_MaxUniformSize; }
	GLint getMaxTextureStage(){ return m_MaxTextureStage; }
	GLint getMaxInstanceCount(){ return 512; }

private:
	GLSLProgramManager();
	virtual ~GLSLProgramManager();

	GLuint m_StdUniformBlock[STD_BLOCK_COUNT];
	GLint m_MaxUniformSize;
	GLint m_MaxTextureStage;
    Texture2D *m_pWorldTexture;

	std::map<wxString, int> m_ProgramMap;
	std::vector<GLSLProgram *> m_Programs;

	static GLSLProgramManager *m_pInstance;
};

}

#endif