// Material.cpp
//
// 2015/11/12 Ian Wu/Real0000
//


#pragma region MaterialStructures
//
// MaterialParam
//
MaterialParam::MaterialParam()
	: m_pRefDesc(nullptr)
	, m_pRefVal(nullptr)
{
}

MaterialParam::~MaterialParam()
{
}

//
// MaterialBlock
//
MaterialBlock::MaterialBlock()
{
}

MaterialBlock::~MaterialBlock()
{
	for( auto it=m_Param.begin() ; it != m_Param.end() ; ++it ) delete it->second;
	m_Param.clear();
}

//
// MaterialTexture
//
MaterialTexture::MaterialTexture()
	: m_pRefDesc(nullptr)
	, m_Texture(nullptr)
{
}

MaterialTexture::~MaterialTexture()
{
	if( nullptr != m_Texture ) TextureManager::singleton().recycle(m_Texture);
}

//
// MaterialExternBlock
//
MaterialExternBlock::MaterialExternBlock()
	: m_Block(nullptr)
{
}

MaterialExternBlock::~MaterialExternBlock()
{
}

void MaterialExternBlock::addParam(wxString l_Name, MaterialParam *l_pParam)
{
}
#pragma endregion

#pragma region Material
//
// Material
//
Material::Material()
{
}

Material::~Material()
{
}

void Material::setProgram(ProgramBase *l_pRefProgram)
{
}

ProgramBase* Material::getProgram()
{
}

void Material::setExternalBlock(MaterialExternBlock *l_pExternalBlock)
{

}

/*
	ProgramBase *m_pRefProgram;
	std::vector<MaterialBlock *> m_BlockList;
	std::map<wxString, MaterialTexture *> m_TextureList;
};*/
#pragma endregion