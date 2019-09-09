// Texture.h
//
// 2014/06/21 Ian Wu/Real0000
//

#ifndef _TEXTURE_H_
#define _TEXTURE_H_

namespace R
{

class ShaderProgram;
class TextureManager;

class TextureUnit
{
	friend class TextureManager;
public:
	TextureUnit();
	virtual ~TextureUnit();
	void release();

	int getTextureID(){ return m_TextureID; }
	wxString getName(){ return m_Name; }
	PixelFormat::Key getTextureFormat();
	glm::ivec3 getDimension();
	TextureType getTextureType();
	bool isReady(){ return m_bReady; }
	void generateMipmap(unsigned int a_Level, std::shared_ptr<ShaderProgram> a_pProgram);

private:
	void setName(wxString a_Name){ m_Name = a_Name; }
	void setTextureID(int a_TexID){ m_TextureID = a_TexID; }
	void setRenderTarget(){ m_bRenderTarget = true; }
	void setReady(){ m_bReady = true; }

	int m_TextureID;
	bool m_bRenderTarget;
	wxString m_Name;
	bool m_bReady;
};

class TextureManager
{
public:
	static TextureManager& singleton();
	
	std::shared_ptr<TextureUnit> createTexture(wxString a_Filename, bool a_bAsync = false, std::function<void(std::shared_ptr<TextureUnit>)> a_Callback = nullptr);
	std::shared_ptr<TextureUnit> createTexture(wxString a_Name, glm::ivec2 a_Size, PixelFormat::Key a_Format, unsigned int a_ArraySize, bool a_bCube, ...);// void *[a_ArraySize]
	std::shared_ptr<TextureUnit> createTexture(wxString a_Name, glm::ivec3 a_Size, PixelFormat::Key a_Format, void *a_pInitData = nullptr);
	std::shared_ptr<TextureUnit> createRenderTarget(wxString a_Name, glm::ivec2 a_Size, PixelFormat::Key a_Format, unsigned int a_ArraySize = 1, bool a_bCube = false);
	std::shared_ptr<TextureUnit> createRenderTarget(wxString a_Name, glm::ivec3 a_Size, PixelFormat::Key a_Format);
	void copyTexture(std::shared_ptr<TextureUnit> a_pDst, std::shared_ptr<TextureUnit> a_pSrc);

	void recycle(wxString a_Name);
	std::shared_ptr<TextureUnit> getTexture(wxString a_Name);
	void clearCache();

private:
	TextureManager();
	virtual ~TextureManager();

	void asyncFunc(std::shared_ptr<TextureUnit> a_Texutre, std::function<void(std::shared_ptr<TextureUnit>)> a_Callback);
	void loadTextureFile(std::shared_ptr<TextureUnit> a_Target);
	std::shared_ptr<TextureUnit> createUniqueTexture(wxString &a_Name);
	
	std::mutex m_FileLock;
	std::map<wxString, std::shared_ptr<TextureUnit> > m_Textures;
};

}

#endif