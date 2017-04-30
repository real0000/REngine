// Texture.h
//
// 2014/06/21 Ian Wu/Real0000
//

#ifndef _TEXTURE_H_
#define _TEXTURE_H_

namespace R
{

class TextureFiles;
class Texture1D;
class Texture2D;
class Texture2DArray;
class Texture3D;
class TextureManager;
class BaseCanvas;

enum
{
	TEXTURE_TYPE_1D = 0,
	TEXTURE_TYPE_2D,
	TEXTURE_TYPE_2DARRAY,
	TEXTURE_TYPE_3D,
	TEXTURE_TYPE_CUBE,
};
class TextureUnit
{
	friend class Texture1D;
	friend class Texture2D;
	friend class Texture2DArray;
	friend class Texture3D;
	friend class TextureManager;
public:
	TextureUnit();
	virtual ~TextureUnit();
	void release();

	virtual unsigned int textureType() = 0;

	unsigned int getTextureID(){ return m_TextureID; }
	wxString getName(){ return m_Name; }
	PixelFormat::Key getTextureFormat();
	unsigned int getPixelSize();
	glm::ivec3 getDimension();

protected:
	void setTextureID(unsigned int l_TexID){ m_TextureID = l_TexID; }

private:
	void setName(wxString l_Name){ m_Name = l_Name; }

	unsigned int m_TextureID;
	wxString m_Name;
};

class Texture1D : public TextureUnit
{
	friend class TextureManager;
private:
	Texture1D();
	virtual ~Texture1D();

public:
	virtual unsigned int textureType(){ return TEXTURE_TYPE_1D; }

	void init(unsigned int l_Width, PixelFormat::Key l_Format, void *l_pInitData = nullptr);
	void update(void *l_pData, unsigned int l_OffsetX, unsigned int l_Width);// nullptr as clean up

private:
};

class Texture2D : public TextureUnit
{
	friend class TextureManager;
private:
	Texture2D();
	virtual ~Texture2D();

public:
	virtual unsigned int textureType(){ return TEXTURE_TYPE_2D; }

	void init(wxString l_File);
	void init(unsigned int l_Width, unsigned int l_Height, PixelFormat::Key l_Format, void *l_pInitData = nullptr);
	void init(glm::ivec2 l_Size, PixelFormat::Key l_Format, void *l_pInitData = nullptr);
	void update(void *l_pData, unsigned int l_OffsetX, unsigned int l_OffsetY, unsigned int l_Width, unsigned int l_Height);
	void update(void *l_pData, glm::ivec2 l_Offset, glm::ivec2 l_Size);

private:
};

class Texture2DArray : public TextureUnit
{
	friend class TextureManager;
private:
	Texture2DArray();
	virtual ~Texture2DArray();

public:
	virtual unsigned int textureType(){ return TEXTURE_TYPE_2DARRAY; }

	void init(std::vector<wxString> l_Filelist);
	void init(unsigned int l_Width, unsigned int l_Height, unsigned int l_NumTexture, PixelFormat::Key l_Format, void *l_pInitData = nullptr);
	void init(glm::ivec2 l_Size, unsigned int l_NumTexture, PixelFormat::Key l_Format, void *l_pInitData = nullptr);
	void update(void *l_pData, unsigned int l_OffsetX, unsigned int l_OffsetY, unsigned int l_Idx, unsigned int l_Width, unsigned int l_Height);
	void update(void *l_pData, glm::ivec2 l_Offset, unsigned int l_Idx, glm::ivec2 l_Size);

private:
};

class Texture3D : public TextureUnit
{
	friend class TextureManager;
private:
	Texture3D();
	virtual ~Texture3D();

public:
	virtual unsigned int textureType(){ return TEXTURE_TYPE_3D; }

	//void init(wxString l_File);
	void init(unsigned int l_Width, unsigned int l_Height, unsigned int l_Depth, PixelFormat::Key l_Format, void *l_pInitData = nullptr);
	void init(glm::ivec3 l_Size, PixelFormat::Key l_Format, void *l_pInitData = nullptr);
	void update(void *l_pData, unsigned int l_OffsetX, unsigned int l_OffsetY, unsigned int l_OffsetZ, unsigned int l_Width, unsigned int l_Height, unsigned int l_Depth);
	void update(void *l_pData, glm::ivec3 l_Offset, glm::ivec3 l_Size);

private:
};


// per canvas, per instance
typedef wxSharedPtr<TextureUnit> TextureUnitPtr;
class TextureManager
{
public:
	static TextureManager& singleton();

    // create new empty texture, must be one of Texture[1-3]D
	TextureUnitPtr newTexture1D(wxString l_Name, unsigned int l_Width, PixelFormat::Key l_Format, void *l_pInitData = nullptr);
	TextureUnitPtr newTexture2D(wxString l_Filename);//2d rgba only for now
	TextureUnitPtr newTexture2D(wxString l_Name, unsigned int l_Width, unsigned int l_Height, PixelFormat::Key l_Format, void *l_pInitData = nullptr);
	TextureUnitPtr newTexture2D(wxString l_Name, glm::ivec2 l_Size, PixelFormat::Key l_Format, void *l_pInitData = nullptr);
	TextureUnitPtr newTexture2DArray(std::vector<wxString> l_FileList);
	TextureUnitPtr newTexture2DArray(wxString l_Name, unsigned int l_Width, unsigned int l_Height, unsigned int l_ArraySize, PixelFormat::Key l_Format, void *l_pInitData = nullptr);
	TextureUnitPtr newTexture2DArray(wxString l_Name, glm::ivec2 l_Size, unsigned int l_ArraySize, PixelFormat::Key l_Format, void *l_pInitData = nullptr);
	TextureUnitPtr newTexture3D(wxString l_Name, unsigned int l_Width, unsigned int l_Height, unsigned int l_Depth, PixelFormat::Key l_Format, void *l_pInitData = nullptr);
	TextureUnitPtr newTexture3D(wxString l_Name, glm::ivec3 l_Size, PixelFormat::Key l_Format, void *l_pInitData = nullptr);

	void recycle(wxString l_Name);
	TextureUnitPtr getTexture(wxString l_Name);
	void clearCache();

private:
	TextureManager();
	virtual ~TextureManager();
	TextureUnitPtr packNewTextureUnit(wxString l_PresetName, TextureUnit *l_pNewUnit);
	
	std::map<wxString, TextureUnitPtr> m_Textures;
};

}

#endif