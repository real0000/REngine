// TextureAsset.h
//
// 2019/11/07 Ian Wu/Real0000
//

#ifndef _TEXTURE_ASSET_H_
#define _TEXTURE_ASSET_H_

#include "AssetBase.h"

namespace R
{

class TextureAsset : public AssetComponent
{
public:
	TextureAsset();
	virtual ~TextureAsset();

	// for asset factory
	static void validImportExt(std::vector<wxString> &a_Output);
	static wxString validAssetKey();

	virtual wxString getAssetExt(){ return TextureAsset::validAssetKey(); }
	virtual void importFile(wxString a_File);
	virtual void loadFile(boost::property_tree::ptree &a_Src);
	virtual void saveFile(boost::property_tree::ptree &a_Dst);

	void initTexture(glm::ivec2 a_Size, PixelFormat::Key a_Format, unsigned int a_ArraySize, bool a_bCube, ...);// void *[a_ArraySize]
	void initTexture(glm::ivec3 a_Size, PixelFormat::Key a_Format, void *a_pInitData = nullptr);
	void initRenderTarget(glm::ivec2 a_Size, PixelFormat::Key a_Format, unsigned int a_ArraySize = 1, bool a_bCube = false);
	void initRenderTarget(glm::ivec3 a_Size, PixelFormat::Key a_Format);
	void resizeRenderTarget(glm::ivec2 a_Size);
	void updateTexture(unsigned int a_MipmapLevel, glm::ivec2 a_Size, glm::ivec2 a_Offset, unsigned int a_Idx, void *a_pSrcData);
	void updateTexture(unsigned int a_MipmapLevel, glm::ivec3 a_Size, glm::ivec3 a_Offset, void *a_pSrcData);

	int getSamplerID();
	int getTextureID(){ return m_TextureID; }
	PixelFormat::Key getTextureFormat();
	glm::ivec3 getDimension();
	TextureType getTextureType();
	bool isReady(){ return m_bReady; }
	void generateMipmap(unsigned int a_Level, std::shared_ptr<ShaderProgram> a_pProgram);

private:
	void importThread(wxString a_Path);
	void loadThread(glm::ivec3 a_Dim, TextureType a_Type, PixelFormat::Key a_Fmt);
	void updateSampler();

	// texture part
	int m_TextureID;
	bool m_bRenderTarget;
	bool m_bReady;
	std::vector<std::string> m_RawFile;

	// sampler part
	int m_SamplerID;
	Filter::Key m_Filter;
	AddressMode::Key m_AddressMode[3];
	float m_MipLodBias;
	unsigned int m_MaxAnisotropy;
	CompareFunc::Key m_Func;
	float m_MinLod;
	float m_MaxLod;
	float m_Border[4];
	bool m_bSamplerDirty;
};

}
#endif