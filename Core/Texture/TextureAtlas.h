// TextureAtlas.h
//
// 2014/04/10 Ian Wu/Real0000
//

#ifndef _TEXTURE_ATLAS_H_
#define _TEXTURE_ATLAS_H_

#include "RGDeviceWrapper.h"

namespace R
{

class TextureManager;
class TextureUnit;

class RenderTextureAtlas
{
public:
	RenderTextureAtlas(glm::ivec2 a_Size, PixelFormat::Key a_Format = PixelFormat::rgba8_unorm, unsigned int a_InitArraySize = 1, bool a_bCube = false);
	virtual ~RenderTextureAtlas();

	unsigned int allocate(glm::ivec2 a_Size);
	void release(unsigned int a_ID);
	void releaseAll();
	
	std::shared_ptr<TextureUnit> getTexture(){ return m_pTexture; }
	void setExtendSize(unsigned int a_Extend);

private:
	ImageAtlas m_Atlas;
	std::shared_ptr<TextureUnit> m_pTexture;
	bool m_bCube;
};

}

#endif