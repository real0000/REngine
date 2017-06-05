// ImageImporter.h
//
// 2017/06/02 Ian Wu/Real0000
//
#ifndef _IMAGE_IMPORTER_H_
#define _IMAGE_IMPORTER_H_

namespace R
{

class ImageData
{
public:
	ImageData();
	virtual ~ImageData();

	void init(wxString a_Path);
	bool isReady(){ return m_bReady; }
	void* getPixels(){ return m_pPixelData; }
	PixelFormat::Key getFormat(){ return m_Format; }
	glm::ivec3 getSize(){ return m_Dim; }

private:
	bool m_bReady;
	char *m_pPixelData;
	PixelFormat::Key m_Format;
	glm::ivec3 m_Dim;
};

class ImageManager : public SearchPathSystem<ImageData>
{
public:
	static ImageManager& singleton();

private:
	ImageManager();
	virtual ~ImageManager();

	void loadFile(ImageData *a_pInst, wxString a_Path);
};

}

#endif