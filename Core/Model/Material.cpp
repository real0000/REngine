// Material.cpp
//
// 2015/11/12 Ian Wu/Real0000
//
#include "CommonUtil.h"
#include "RGDeviceWrapper.h"
#include "Core.h"
#include "Material.h"
#include "Texture/Texture.h"

namespace R
{

#pragma region MaterialBlock
//
// MaterialBlock
//
MaterialBlock::MaterialBlock()
{
}

MaterialBlock::~MaterialBlock()
{
}

void MaterialBlock::addParam(std::string a_Name, ShaderParamType::Key a_Type, char *a_pInitVal)
{
}

void MaterialBlock::init(ShaderRegType::Key a_Type, unsigned int a_NumSlot)
{
	assert(0 != a_NumSlot);
}

void MaterialBlock::bind(GraphicCommander *a_pBinder)
{

}
#pragma endregion

}