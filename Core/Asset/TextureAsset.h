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

	virtual void importFile(wxString a_File);
	virtual void loadFile(wxString a_File);
	virtual void saveFile(wxString a_File);

private:
	unsigned int m_Dimention;
	glm::ivec3 m_Size;

};

}
#endif