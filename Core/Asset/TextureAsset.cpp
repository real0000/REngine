// TextureAsset.cpp
//
// 2019/11/10 Ian Wu/Real0000
//

#include "CommonUtil.h"
#include "RGDeviceWrapper.h"

#include "TextureAsset.h"

namespace R
{

#pragma region TextureAsset
//
// TextureAsset
//
TextureAsset::TextureAsset()
{
}

TextureAsset::~TextureAsset()
{
}

void TextureAsset::validImportExt(std::vector<wxString> &a_Output)
{
	const wxString c_ExtList[] = {
		wxT("dds"),
		wxT("png"),
		wxT("bmp"),
		wxT("jpg"),
		wxT("jpeg"),
		wxT("tga"),
	};
	const unsigned int c_NumExt = sizeof(c_ExtList) / sizeof(wxString);
	for( unsigned int i=0 ; i<c_NumExt ; ++i ) a_Output.push_back(c_ExtList[i]);
}

wxString TextureAsset::validAssetKey()
{
	return wxT("Image");
}

void TextureAsset::importFile(wxString a_File)
{
}

void TextureAsset::loadFile(wxString a_File)
{
}

void TextureAsset::saveFile(wxString a_File)
{
}
#pragma endregion

}