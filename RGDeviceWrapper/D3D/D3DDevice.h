// D3DDevice.h
//
// 2015/11/12 Ian Wu/Real0000
//

#ifndef _D3DDEVICE_H_
#define _D3DDEVICE_H_

#include "DeviceWrapper.h"

namespace R
{
	
class Dircet3DDevice : public GraphicDevice
{
public:
	Dircet3DDevice();
	virtual ~Dircet3DDevice();
	
	// converter part( *::Key -> d3d, vulkan var )
	virtual unsigned int getPixelFormat(PixelFormat::Key a_Key);
	virtual unsigned int getParamAlignment(ShaderParamType::Key a_Key);
};

}

#endif