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
	void* getPixels(unsigned int a_Surface){ return m_RefSurfacePtr[a_Surface]; }
	PixelFormat::Key getFormat(){ return m_Format; }
	glm::ivec3 getSize(){ return m_Dim; }
	TextureType getType(){ return m_Type; }

private:
	void loadDDS(wxString a_Filepath);
	void loadGeneral(wxString a_Filepath);
	void clear();// destructor or load failed

	bool m_bReady;
	unsigned char *m_pImageData;
	std::vector<unsigned char *> m_RefSurfacePtr;
	PixelFormat::Key m_Format;
	glm::ivec3 m_Dim;
	TextureType m_Type;
};

class ImageManager : public SearchPathSystem<ImageData>
{
public:
	static ImageManager& singleton();

private:
	ImageManager();
	virtual ~ImageManager();

	void loadFile(std::shared_ptr<ImageData> a_pInst, wxString a_Path);
};

}

#endif