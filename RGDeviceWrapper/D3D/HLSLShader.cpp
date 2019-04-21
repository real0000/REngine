// HLSLShader.cpp
//
// 2014/08/27 Ian Wu/Real0000
//

#include "RGDeviceWrapper.h"

#include "wx/file.h"

namespace R
{

#pragma region HLSLProgram12 
//
// HLSLProgram12 
//
HLSLProgram12::HLSLProgram12()
	: m_pPipeline(nullptr)
	, m_pRegisterDesc(nullptr)
	, m_pIndirectFmt(nullptr)
	, m_Topology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST)
	, m_IndirectCmdSize(0)
{
}

HLSLProgram12::~HLSLProgram12()
{
	for( unsigned int i=0 ; i<ShaderRegType::UavBuffer+1 ; ++i )
	{
		for( auto it=m_RegMap[i].begin() ; it!=m_RegMap[i].end() ; ++it ) delete it->second;
		m_RegMap[i].clear();
	}

	SAFE_RELEASE(m_pPipeline)
	SAFE_RELEASE(m_pRegisterDesc)
}

void HLSLProgram12::init(boost::property_tree::ptree &a_Root)
{
	char l_Buff[256];
	snprintf(l_Buff, 256, "root.Shaders.Model_%d_%d", shaderInUse().first, shaderInUse().second);
	boost::property_tree::ptree &l_Shaders = a_Root.get_child(l_Buff);
	
	std::map<std::string, std::string> l_ParamDef;
	initRegister(l_Shaders, a_Root.get_child("root.ParamList"), l_ParamDef);
	if( !isCompute() ) initDrawShader(a_Root.get_child("root.ShaderSetting"), l_Shaders, l_ParamDef);
	else initComputeShader(l_Shaders, l_ParamDef);
}

std::pair<int, int> HLSLProgram12::getConstantSlot(std::string a_Name)
{
	auto it = m_RegMap[ShaderRegType::Constant].find(a_Name);
	if( m_RegMap[ShaderRegType::Constant].end() == it ) return std::make_pair(-1, -1);
	return std::make_pair(it->second->m_Slot, it->second->m_Offset);
}

void HLSLProgram12::assignIndirectVertex(unsigned int &a_Offset, char *a_pOutput, VertexBuffer *a_pVtx)
{
	D3D12Device *l_pDeviceOwner = TYPED_GDEVICE(D3D12Device);

	if( nullptr == a_pVtx )
	{
		for( unsigned int i=0 ; i<VTXSLOT_COUNT ; ++i )
		{
			int l_BufferID = getNullVertex()->getBufferID(i);
			memcpy(static_cast<char *>(a_pOutput) + a_Offset, &(l_pDeviceOwner->getVertexBufferView(l_BufferID)), sizeof(D3D12_VERTEX_BUFFER_VIEW));
			a_Offset += sizeof(D3D12_VERTEX_BUFFER_VIEW);
		}
	}
	else
	{
		for( unsigned int i=0 ; i<VTXSLOT_COUNT ; ++i )
		{
			int l_BufferID = a_pVtx->getBufferID(i);
			if( -1 == l_BufferID ) l_BufferID = getNullVertex()->getBufferID(i);
			memcpy(static_cast<char *>(a_pOutput) + a_Offset, &(l_pDeviceOwner->getVertexBufferView(l_BufferID)), sizeof(D3D12_VERTEX_BUFFER_VIEW));
			a_Offset += sizeof(D3D12_VERTEX_BUFFER_VIEW);
		}
	}
}

void HLSLProgram12::assignIndirectIndex(unsigned int &a_Offset, char *a_pOutput, IndexBuffer *a_pIndex)
{
	assert(nullptr != a_pIndex);
	D3D12Device *l_pDeviceOwner = TYPED_GDEVICE(D3D12Device);
	memcpy(static_cast<char *>(a_pOutput) + a_Offset, &(l_pDeviceOwner->getIndexBufferView(a_pIndex->getBufferID())), sizeof(D3D12_INDEX_BUFFER_VIEW));
	a_Offset += sizeof(D3D12_INDEX_BUFFER_VIEW);
}

void HLSLProgram12::assignIndirectBlock(unsigned int &a_Offset, char *a_pOutput, ShaderRegType::Key a_Type, std::vector<int> &a_IDList)
{
	D3D12Device *l_pDeviceOwner = TYPED_GDEVICE(D3D12Device);
	std::function<D3D12_GPU_VIRTUAL_ADDRESS(int)> l_Func = nullptr;
	if( ShaderRegType::ConstBuffer == a_Type ) l_Func = std::bind(&D3D12Device::getConstBufferGpuAddress, l_pDeviceOwner, std::placeholders::_1);
	else l_Func = std::bind(&D3D12Device::getUnorderAccessBufferGpuAddress, l_pDeviceOwner, std::placeholders::_1);
	
	for( unsigned int i=0 ; i<a_IDList.size() ; ++i )
	{
		D3D12_GPU_VIRTUAL_ADDRESS *l_pTarget = reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS *>((char *)a_pOutput + a_Offset);
		if( -1 == a_IDList[i] ) *l_pTarget = 0;
		else *l_pTarget = l_Func(a_IDList[i]);
	}
}

void HLSLProgram12::assignIndirectDrawComaand(unsigned int &a_Offset, char *a_pOutput, unsigned int a_IndexCount, unsigned int a_InstanceCount, unsigned int a_StartIndex, int a_BaseVertex, unsigned int a_StartInstance)
{
	D3D12_DRAW_INDEXED_ARGUMENTS *l_pArg = reinterpret_cast<D3D12_DRAW_INDEXED_ARGUMENTS *>(a_pOutput + a_Offset);
	l_pArg->IndexCountPerInstance = a_IndexCount;
    l_pArg->InstanceCount = a_InstanceCount;
    l_pArg->StartIndexLocation = a_StartIndex;
    l_pArg->BaseVertexLocation = a_BaseVertex;
    l_pArg->StartInstanceLocation = a_StartInstance;
}

void HLSLProgram12::initRegister(boost::property_tree::ptree &a_ShaderDesc, boost::property_tree::ptree &a_ParamDesc, std::map<std::string, std::string> &a_ParamOutput)
{
	std::map<std::string, ShaderParamType::Key> l_ConstTypeMap;
	std::map<std::string, RegisterInfo *> l_ParamMap[ShaderRegType::UavBuffer+1][ShaderStages::NumStage];
	{
		std::map<std::string, D3D12_SHADER_VISIBILITY> l_RegVisibleMaps;
		for( auto it = a_ShaderDesc.begin() ; it != a_ShaderDesc.end() ; ++it )
		{
			if( "<xmlattr>" == it->first ) continue;
			unsigned int l_VisibleFlag = ShaderStages::fromString(it->first);
			
			std::string l_UsedParam(it->second.get("", ""));
			std::replace_if(l_UsedParam.begin(), l_UsedParam.end(), [](char a_Char){ return !isgraph((int)a_Char); }, ' ');
			std::vector<wxString> l_ParamList;
			splitString(' ', l_UsedParam, l_ParamList);

			for( unsigned int i=0 ; i<l_ParamList.size() ; ++i )
			{
				std::string l_TempCString(l_ParamList[i].c_str());
				auto l_RegIt = l_RegVisibleMaps.find(l_TempCString);
				if( l_RegVisibleMaps.end() == l_RegIt ) l_RegVisibleMaps.insert(std::make_pair(l_TempCString, (D3D12_SHADER_VISIBILITY)l_VisibleFlag));
				else l_RegIt->second = D3D12_SHADER_VISIBILITY_ALL;
			}
		}

		for( auto it=a_ParamDesc.begin() ; it!=a_ParamDesc.end() ; ++it )
		{
			std::string l_Name(it->second.get<std::string>("<xmlattr>.name"));
			ShaderRegType::Key l_RegType = ShaderRegType::fromString(it->first);
			std::map<std::string, RegisterInfo *> &l_TargetMap = l_ParamMap[l_RegType][l_RegVisibleMaps[l_Name]];

			RegisterInfo *l_pNewInfo = new RegisterInfo();
			l_pNewInfo->m_bReserved = (it->second.get("<xmlattr>.reserved", "false") == "true") || l_RegType == ShaderRegType::UavBuffer;
			l_pNewInfo->m_Type = l_RegType;
			l_TargetMap.insert(std::make_pair(l_Name, l_pNewInfo));

			if( ShaderRegType::Constant == l_RegType ) l_ConstTypeMap[l_Name] = ShaderParamType::fromString(it->second.get<std::string>("<xmlattr>.type"));
		}
	}

	std::vector<D3D12_ROOT_PARAMETER> l_RegCollect;
	std::vector<D3D12_DESCRIPTOR_RANGE> l_RegRangeCollect;
	std::map<ShaderRegType::Key, std::vector<std::string> > l_TempRegCount;
	std::map<std::string, ProgramTextureDesc *> &l_TextuerMap = getTextureDesc();

	// srv
	unsigned int l_Slot = 0;
	{
		const ShaderRegType::Key c_SrvSerial[] = {ShaderRegType::Srv2D, ShaderRegType::Srv3D, ShaderRegType::Srv2DArray, ShaderRegType::SrvCube, ShaderRegType::SrvCubeArray}; 
		const unsigned int c_NumType = sizeof(c_SrvSerial) / sizeof(const ShaderRegType::Key);
		for( unsigned int i=0 ; i<c_NumType ; ++i )
		{
			ShaderRegType::Key l_Type = c_SrvSerial[i];
			for( unsigned int l_Visibility = 0 ; l_Visibility < ShaderStages::NumStage ; ++l_Visibility )
			{
				std::map<std::string, RegisterInfo *> &l_ParamList = l_ParamMap[l_Type][l_Visibility];
				if( l_ParamList.empty() ) continue;
				
				D3D12_ROOT_PARAMETER l_TempReg;
				D3D12_DESCRIPTOR_RANGE l_TempRange;

				for( auto it=l_ParamList.begin() ; it!=l_ParamList.end() ; ++it )
				{
					it->second->m_RootIndex = l_RegCollect.size();;
					it->second->m_Slot = l_Slot;
					l_TextuerMap[it->first]->m_pRegInfo = it->second;
					m_RegMap[l_Type][it->first] = it->second;

					l_TempRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
					l_TempRange.NumDescriptors = (l_Type == ShaderRegType::Srv2DArray || l_Type == ShaderRegType::SrvCubeArray) ? TEXTURE_ARRAY_SIZE : 1;
					l_TempRange.BaseShaderRegister = l_Slot;
					l_TempRange.RegisterSpace = 0;
					l_TempRange.OffsetInDescriptorsFromTableStart = 0;
					l_RegRangeCollect.push_back(l_TempRange);
					
					l_TempReg.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
					l_TempReg.DescriptorTable.NumDescriptorRanges = 1;
					l_TempReg.DescriptorTable.pDescriptorRanges = &(l_RegRangeCollect.back());
					l_TempReg.ShaderVisibility = (D3D12_SHADER_VISIBILITY)l_Visibility;
					l_RegCollect.push_back(l_TempReg);

					m_TextureStageMap.push_back(l_Slot);

					++l_Slot;
				}
			}
		}
	}

	// uav(storage)
	l_Slot = 0;
	std::vector<ProgramBlockDesc *> &l_UavBlockDesc = getBlockDesc(ShaderRegType::UavBuffer);
	for( unsigned int l_Visibility = 0 ; l_Visibility < ShaderStages::NumStage ; ++l_Visibility )
	{
		std::map<std::string, RegisterInfo *> &l_ParamList = l_ParamMap[ShaderRegType::UavBuffer][l_Visibility];
		if( l_ParamList.empty() ) continue;

		for( auto it=l_ParamList.begin() ; it!=l_ParamList.end() ; ++it )
		{
			it->second->m_RootIndex = l_RegCollect.size();;
			it->second->m_Slot = l_Slot;
			m_RegMap[ShaderRegType::UavBuffer][it->first] = it->second;
			
			D3D12_ROOT_PARAMETER l_TempReg;
			l_TempReg.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
			l_TempReg.Descriptor.ShaderRegister = l_Slot;
			l_TempReg.Descriptor.RegisterSpace = 0;
			l_TempReg.ShaderVisibility = (D3D12_SHADER_VISIBILITY)l_Visibility;
			l_RegCollect.push_back(l_TempReg);

			std::string l_BlockName(it->first);	
			auto l_UavIt = std::find_if(l_UavBlockDesc.begin(), l_UavBlockDesc.end(), [l_BlockName](ProgramBlockDesc *a_pDesc)->bool
			{
				return a_pDesc->m_Name == l_BlockName;
			});
			assert(l_UavIt != l_UavBlockDesc.end());
			ProgramBlockDesc *l_pNewBlock = *l_UavIt;
			l_pNewBlock->m_pRegInfo = it->second;
			
			m_UavStageMap.push_back(l_Slot);

			++l_Slot;
		}
	}

	// const buffer
	l_Slot = 0;
	for( unsigned int l_Visibility = 0 ; l_Visibility < ShaderStages::NumStage ; ++l_Visibility )
	{
		std::map<std::string, RegisterInfo *> &l_ParamList = l_ParamMap[ShaderRegType::ConstBuffer][l_Visibility];
		if( l_ParamList.empty() ) continue;

		for( auto it=l_ParamList.begin() ; it!=l_ParamList.end() ; ++it )
		{
			it->second->m_RootIndex = l_RegCollect.size();
			it->second->m_Slot = l_Slot;
			m_RegMap[ShaderRegType::ConstBuffer][it->first] = it->second;
			
			D3D12_ROOT_PARAMETER l_TempReg;
			l_TempReg.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			l_TempReg.Descriptor.ShaderRegister = l_Slot;
			l_TempReg.Descriptor.RegisterSpace = 0;
			l_TempReg.ShaderVisibility = (D3D12_SHADER_VISIBILITY)l_Visibility;
			l_RegCollect.push_back(l_TempReg);

			m_ConstStageMap.push_back(l_Slot);

			++l_Slot;
		}
	}
	
	// constant
	for( unsigned int l_Visibility = 0 ; l_Visibility < ShaderStages::NumStage ; ++l_Visibility )
	{
		std::map<std::string, RegisterInfo *> &l_ParamList = l_ParamMap[ShaderRegType::Constant][l_Visibility];
		if( l_ParamList.empty() ) continue;
		char l_Temp[256];

		snprintf(l_Temp, 256, "Const32[%d]", l_Visibility);
		ProgramBlockDesc *l_pNewConstBlock = newConstBlockDesc();
		l_pNewConstBlock->m_Name = l_Temp;

		for( auto it=l_ParamList.begin() ; it!=l_ParamList.end() ; ++it )
		{
			ProgramParamDesc *l_pNewParam = new ProgramParamDesc();
			l_pNewParam->m_Type = l_ConstTypeMap[it->first];
			l_pNewParam->m_Offset = ProgramManager::singleton().calculateParamOffset(l_pNewConstBlock->m_BlockSize, l_pNewParam->m_Type);
			unsigned int l_ParamSize = l_pNewConstBlock->m_BlockSize - l_pNewParam->m_Offset;
			l_pNewParam->m_pDefault = new char[l_ParamSize];
			memset(l_pNewParam->m_pDefault, 0, l_ParamSize);
			l_pNewParam->m_pRegInfo = it->second;
			l_pNewConstBlock->m_ParamDesc.insert(std::make_pair(it->first, l_pNewParam));

			it->second->m_RootIndex = l_RegCollect.size();
			it->second->m_Slot = l_Slot;
			it->second->m_Offset = l_pNewParam->m_Offset;
			it->second->m_Size = l_ParamSize;
			m_RegMap[ShaderRegType::Constant][it->first] = it->second;
		}

		D3D12_ROOT_PARAMETER l_TempReg;
		l_TempReg.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		l_TempReg.Constants.ShaderRegister = l_Slot;
		l_TempReg.Constants.RegisterSpace = 0;
		l_TempReg.Constants.Num32BitValues = l_pNewConstBlock->m_BlockSize / 4;
		l_TempReg.ShaderVisibility = (D3D12_SHADER_VISIBILITY)l_Visibility;
		l_RegCollect.push_back(l_TempReg);
		
		++l_Slot;
	}

	enum
	{
		LINEAR_CLAMP_SAMPLER = 0,
		POINT_CLAMP_SAMPLER,
		ANISOTROPIC_CLAMP_SAMPLER,

		NUM_SAMPLER
	};
	D3D12_STATIC_SAMPLER_DESC s_SamplerSetting[NUM_SAMPLER];
	{// s0
		s_SamplerSetting[LINEAR_CLAMP_SAMPLER].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		s_SamplerSetting[LINEAR_CLAMP_SAMPLER].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		s_SamplerSetting[LINEAR_CLAMP_SAMPLER].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		s_SamplerSetting[LINEAR_CLAMP_SAMPLER].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		s_SamplerSetting[LINEAR_CLAMP_SAMPLER].MipLODBias = 0;
		s_SamplerSetting[LINEAR_CLAMP_SAMPLER].MaxAnisotropy = 16;
		s_SamplerSetting[LINEAR_CLAMP_SAMPLER].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		s_SamplerSetting[LINEAR_CLAMP_SAMPLER].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		s_SamplerSetting[LINEAR_CLAMP_SAMPLER].MinLOD = 0.0f;
		s_SamplerSetting[LINEAR_CLAMP_SAMPLER].MaxLOD = D3D12_FLOAT32_MAX;
		s_SamplerSetting[LINEAR_CLAMP_SAMPLER].ShaderRegister = LINEAR_CLAMP_SAMPLER;
		s_SamplerSetting[LINEAR_CLAMP_SAMPLER].RegisterSpace = 0;
		s_SamplerSetting[LINEAR_CLAMP_SAMPLER].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	}

	{// s1
		s_SamplerSetting[POINT_CLAMP_SAMPLER] = s_SamplerSetting[LINEAR_CLAMP_SAMPLER];
		s_SamplerSetting[POINT_CLAMP_SAMPLER].Filter = D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_POINT;
		s_SamplerSetting[POINT_CLAMP_SAMPLER].ShaderRegister = POINT_CLAMP_SAMPLER;
	}

	{// s2
		s_SamplerSetting[ANISOTROPIC_CLAMP_SAMPLER] = s_SamplerSetting[LINEAR_CLAMP_SAMPLER];
		s_SamplerSetting[ANISOTROPIC_CLAMP_SAMPLER].Filter = D3D12_FILTER_ANISOTROPIC;
		s_SamplerSetting[ANISOTROPIC_CLAMP_SAMPLER].ShaderRegister = ANISOTROPIC_CLAMP_SAMPLER;
	}

	D3D12_ROOT_SIGNATURE_DESC l_RegisterDesc = {};
	l_RegisterDesc.NumParameters = l_RegCollect.size();
	l_RegisterDesc.pParameters = l_RegCollect.data();
	l_RegisterDesc.NumStaticSamplers = NUM_SAMPLER;
	l_RegisterDesc.pStaticSamplers = s_SamplerSetting;
	l_RegisterDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	ID3DBlob *l_BinSignature = nullptr;
	ID3DBlob *l_Error = nullptr;
	D3D12SerializeRootSignature(&l_RegisterDesc, D3D_ROOT_SIGNATURE_VERSION_1, &l_BinSignature, &l_Error);
	if( nullptr != l_Error )
	{
		wxMessageBox(wxString::Format(wxT("Failed to compile shader :\n%s"), static_cast<const char *>(l_Error->GetBufferPointer())), wxT("HLSLProgram::initRegister"));
		SAFE_RELEASE(l_Error);
		return ;
	}

	ID3D12Device *l_pRefDevice = TYPED_GDEVICE(D3D12Device)->getDeviceInst();
	l_pRefDevice->CreateRootSignature(0, l_BinSignature->GetBufferPointer(), l_BinSignature->GetBufferSize(), IID_PPV_ARGS(&m_pRegisterDesc));
	SAFE_RELEASE(l_BinSignature)
}

void HLSLProgram12::initDrawShader(boost::property_tree::ptree &a_ShaderSetting, boost::property_tree::ptree &a_Shaders, std::map<std::string, std::string> &a_ParamDefine)
{
	D3D12Device *l_pRefDevice = TYPED_GDEVICE(D3D12Device);
	ID3D12Device *l_pDeviceInst = l_pRefDevice->getDeviceInst();

	{// shader setting part
		D3D12_GRAPHICS_PIPELINE_STATE_DESC l_PsoDesc = {};
		l_PsoDesc.pRootSignature = m_pRegisterDesc;
		l_PsoDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;

		Topology::Key l_Topology = Topology::fromString(a_ShaderSetting.get("<xmlattr>.topology", "triangle"));
		m_Topology = (D3D_PRIMITIVE_TOPOLOGY)l_pRefDevice->getTopology(l_Topology);
		// setup 
		switch( l_Topology )
		{
			case Topology::point_list:
				l_PsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
				break;

			case Topology::line_list:
			case Topology::line_strip:
			case Topology::line_list_adj:
			case Topology::line_strip_adj:
				l_PsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
				break;

			case Topology::triangle_list:
			case Topology::triangle_strip:
			case Topology::triangle_list_adj:
			case Topology::triangle_strip_adj:
				l_PsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
				break;
				
			case Topology::point_patch_list_1:
			case Topology::point_patch_list_2:
			case Topology::point_patch_list_3:
			case Topology::point_patch_list_4:
			case Topology::point_patch_list_5:
			case Topology::point_patch_list_6:
			case Topology::point_patch_list_7:
			case Topology::point_patch_list_8:
			case Topology::point_patch_list_9:
			case Topology::point_patch_list_10:
			case Topology::point_patch_list_11:
			case Topology::point_patch_list_12:
			case Topology::point_patch_list_13:
			case Topology::point_patch_list_14:
			case Topology::point_patch_list_15:
			case Topology::point_patch_list_16:
			case Topology::point_patch_list_17:
			case Topology::point_patch_list_18:
			case Topology::point_patch_list_19:
			case Topology::point_patch_list_20:
			case Topology::point_patch_list_21:
			case Topology::point_patch_list_22:
			case Topology::point_patch_list_23:
			case Topology::point_patch_list_24:
			case Topology::point_patch_list_25:
			case Topology::point_patch_list_26:
			case Topology::point_patch_list_27:
			case Topology::point_patch_list_28:
			case Topology::point_patch_list_29:
			case Topology::point_patch_list_30:
			case Topology::point_patch_list_31:
			case Topology::point_patch_list_32:
				l_PsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
				break;

			default:
				assert(false && "invalid topology type");
				break;
		}

		l_PsoDesc.NodeMask = 0;
		l_PsoDesc.CachedPSO.pCachedBlob = nullptr;
		l_PsoDesc.CachedPSO.CachedBlobSizeInBytes = 0;
		
		const D3D12_INPUT_ELEMENT_DESC c_InputLayout[] = {
			{"POSITION"		, 0, DXGI_FORMAT_R32G32B32_FLOAT	, VTXSLOT_POSITION	, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"TEXCOORD0"	, 0, DXGI_FORMAT_R32G32B32_FLOAT	, VTXSLOT_TEXCOORD01, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"TEXCOORD1"	, 0, DXGI_FORMAT_R32G32B32_FLOAT	, VTXSLOT_TEXCOORD23, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"TEXCOORD2"	, 0, DXGI_FORMAT_R32G32B32_FLOAT	, VTXSLOT_TEXCOORD45, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"TEXCOORD3"	, 0, DXGI_FORMAT_R32G32B32_FLOAT	, VTXSLOT_TEXCOORD67, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"NORMAL"		, 0, DXGI_FORMAT_R32G32B32_FLOAT	, VTXSLOT_NORMAL	, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"TANGENT"		, 0, DXGI_FORMAT_R32G32B32_FLOAT	, VTXSLOT_TANGENT	, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"BINORMAL"		, 0, DXGI_FORMAT_R32G32B32_FLOAT	, VTXSLOT_BINORMAL	, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"BLENDINDICES"	, 0, DXGI_FORMAT_R32G32B32A32_SINT	, VTXSLOT_BONE		, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"BLENDWEIGHT"	, 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, VTXSLOT_WEIGHT	, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"COLOR"		, 0, DXGI_FORMAT_R8G8B8A8_UNORM		, VTXSLOT_COLOR		, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};
		const unsigned int c_NumInputElement = sizeof(c_InputLayout) / sizeof(const D3D12_INPUT_ELEMENT_DESC);

		l_PsoDesc.InputLayout.pInputElementDescs = c_InputLayout;
		l_PsoDesc.InputLayout.NumElements = c_NumInputElement;
		l_PsoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

		sscanf(a_ShaderSetting.get("<xmlattr>.sampleMask", "ffffffff").c_str(), "%x", &l_PsoDesc.SampleMask);
		l_PsoDesc.SampleDesc.Count = atoi(a_ShaderSetting.get("<xmlattr>.sampleCount", "1").c_str());
		l_PsoDesc.SampleDesc.Quality = atoi(a_ShaderSetting.get("<xmlattr>.sampleQuality", "0").c_str());
		
		std::map<unsigned int, ID3DBlob *> l_ShaderUsage;
		{// compile shader code
			for( unsigned int i=0 ; i<ShaderStages::NumStage ; ++i )
			{
				if( i == ShaderStages::Compute ) continue;// draw shader won't have computer stage

				char l_Buff[256];
				snprintf(l_Buff, 256, "%s.<xmlattr>.file", static_cast<const char*>(ShaderStages::toString((ShaderStages::Key)i).c_str()));
				wxString l_Filename(a_Shaders.get<std::string>(l_Buff));
				
				if( l_Filename.IsEmpty() ) l_ShaderUsage[i] = nullptr;
				else l_ShaderUsage[i] = static_cast<ID3DBlob *>(ProgramManager::singleton().getShader(this, l_Filename, (ShaderStages::Key)i, shaderInUse(), a_ParamDefine));
			}

			ID3DBlob *l_pShader = l_ShaderUsage[ShaderStages::Vertex];
			if( nullptr != l_pShader )
			{
				l_PsoDesc.VS.BytecodeLength = l_pShader->GetBufferSize();
				l_PsoDesc.VS.pShaderBytecode = l_pShader->GetBufferPointer();
			}

			l_pShader = l_ShaderUsage[ShaderStages::Fragment];
			if( nullptr != l_pShader )
			{
				l_PsoDesc.PS.BytecodeLength = l_pShader->GetBufferSize();
				l_PsoDesc.PS.pShaderBytecode = l_pShader->GetBufferPointer();
			}

			l_pShader = l_ShaderUsage[ShaderStages::TessEval];
			if( nullptr != l_pShader )
			{
				l_PsoDesc.DS.BytecodeLength = l_pShader->GetBufferSize();
				l_PsoDesc.DS.pShaderBytecode = l_pShader->GetBufferPointer();
			}

			l_pShader = l_ShaderUsage[ShaderStages::TessCtrl];
			if( nullptr != l_pShader )
			{
				l_PsoDesc.HS.BytecodeLength = l_pShader->GetBufferSize();
				l_PsoDesc.HS.pShaderBytecode = l_pShader->GetBufferPointer();
			}

			l_pShader = l_ShaderUsage[ShaderStages::Geometry];
			if( nullptr != l_pShader )
			{
				l_PsoDesc.GS.BytecodeLength = l_pShader->GetBufferSize();
				l_PsoDesc.GS.pShaderBytecode = l_pShader->GetBufferPointer();	
			}
		}

		{//setup blend state
			unsigned int l_Idx = 0;
			boost::property_tree::ptree &l_BlendRoot = a_ShaderSetting.get_child("BlendState");
			for( auto l_BlendElement = l_BlendRoot.begin() ; l_BlendElement != l_BlendRoot.end() ; ++l_BlendElement, ++l_Idx )
			{
				if( "<xmlattr>" == l_BlendElement->first ) continue;

				boost::property_tree::ptree &l_Attr = l_BlendElement->second.get_child("<xmlattr>");
				l_PsoDesc.BlendState.RenderTarget[l_Idx].BlendEnable = l_Attr.get("enableBlend", "false") == "true";
				l_PsoDesc.BlendState.RenderTarget[l_Idx].LogicOpEnable = l_Attr.get("enableOP", "false") == "true";
				l_PsoDesc.BlendState.RenderTarget[l_Idx].SrcBlend = (D3D12_BLEND)l_pRefDevice->getBlendKey(BlendKey::fromString(l_Attr.get("srcBlend", "one")));
				l_PsoDesc.BlendState.RenderTarget[l_Idx].DestBlend = (D3D12_BLEND)l_pRefDevice->getBlendKey(BlendKey::fromString(l_Attr.get("dstBlend", "zero")));
				l_PsoDesc.BlendState.RenderTarget[l_Idx].BlendOp = (D3D12_BLEND_OP)l_pRefDevice->getBlendOP(BlendOP::fromString(l_Attr.get("blendOP", "add")));
				l_PsoDesc.BlendState.RenderTarget[l_Idx].SrcBlendAlpha = (D3D12_BLEND)l_pRefDevice->getBlendKey(BlendKey::fromString(l_Attr.get("srcBlendAlpha", "one")));
				l_PsoDesc.BlendState.RenderTarget[l_Idx].DestBlendAlpha = (D3D12_BLEND)l_pRefDevice->getBlendKey(BlendKey::fromString(l_Attr.get("dstBlendAlpha", "zero")));
				l_PsoDesc.BlendState.RenderTarget[l_Idx].BlendOpAlpha = (D3D12_BLEND_OP)l_pRefDevice->getBlendOP(BlendOP::fromString(l_Attr.get("alphaBlendOP", "add")));
				l_PsoDesc.BlendState.RenderTarget[l_Idx].LogicOp = (D3D12_LOGIC_OP)l_pRefDevice->getBlendLogic(BlendLogic::fromString(l_Attr.get("logic", "none_")));

				unsigned char l_Channel = 0;
				if( l_Attr.get("red", "true") == "true" )	l_Channel |= D3D12_COLOR_WRITE_ENABLE_RED;
				if( l_Attr.get("green", "true") == "true" )	l_Channel |= D3D12_COLOR_WRITE_ENABLE_GREEN;
				if( l_Attr.get("blue", "true") == "true" ) 	l_Channel |= D3D12_COLOR_WRITE_ENABLE_BLUE;
				if( l_Attr.get("alpha", "true") == "true" )	l_Channel |= D3D12_COLOR_WRITE_ENABLE_ALPHA;
				l_PsoDesc.BlendState.RenderTarget[l_Idx].RenderTargetWriteMask = l_Channel;
			}

			for( ; l_Idx<8 ; ++l_Idx )
			{
				l_PsoDesc.BlendState.RenderTarget[l_Idx].BlendEnable = false;
				l_PsoDesc.BlendState.RenderTarget[l_Idx].LogicOpEnable = false;
				l_PsoDesc.BlendState.RenderTarget[l_Idx].SrcBlend = D3D12_BLEND_ONE;
				l_PsoDesc.BlendState.RenderTarget[l_Idx].DestBlend = D3D12_BLEND_ZERO;
				l_PsoDesc.BlendState.RenderTarget[l_Idx].BlendOp = D3D12_BLEND_OP_ADD;
				l_PsoDesc.BlendState.RenderTarget[l_Idx].SrcBlendAlpha = D3D12_BLEND_ONE;
				l_PsoDesc.BlendState.RenderTarget[l_Idx].DestBlendAlpha = D3D12_BLEND_ZERO;
				l_PsoDesc.BlendState.RenderTarget[l_Idx].BlendOpAlpha = D3D12_BLEND_OP_ADD;
				l_PsoDesc.BlendState.RenderTarget[l_Idx].LogicOp = D3D12_LOGIC_OP_NOOP;
				l_PsoDesc.BlendState.RenderTarget[l_Idx].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			}
		}

		{// setup rasterizer state
			boost::property_tree::ptree &l_Attr = a_ShaderSetting.get_child("RasterizerState.<xmlattr>");

			l_PsoDesc.RasterizerState.FillMode = l_Attr.get("filled", "true") == "true" ? D3D12_FILL_MODE_SOLID : D3D12_FILL_MODE_WIREFRAME;
			l_PsoDesc.RasterizerState.CullMode = (D3D12_CULL_MODE)l_pRefDevice->getCullMode(CullMode::fromString(l_Attr.get("cull", "back")));
			l_PsoDesc.RasterizerState.FrontCounterClockwise = l_Attr.get("ccFront", "true") == "true";
			l_PsoDesc.RasterizerState.DepthBias = atoi(l_Attr.get("depthBias", "0").c_str());
			l_PsoDesc.RasterizerState.SlopeScaledDepthBias = atof(l_Attr.get("slopeDepthBias", "0.0").c_str());
			l_PsoDesc.RasterizerState.DepthBiasClamp = atof(l_Attr.get("depthBiasClamp", "0.0").c_str());
			l_PsoDesc.RasterizerState.DepthClipEnable = l_Attr.get("useDepthClip", "true") == "true";
			l_PsoDesc.RasterizerState.MultisampleEnable = l_Attr.get("useMultiSample", "false") == "true";
			l_PsoDesc.RasterizerState.AntialiasedLineEnable = l_Attr.get("lineaa", "false") == "true";
			l_PsoDesc.RasterizerState.ForcedSampleCount = atoi(l_Attr.get("forceSampleCount", "0").c_str());
			l_PsoDesc.RasterizerState.ConservativeRaster = l_Attr.get("conservative", "false") == "true" ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
		}

		{// setup depth stencil state
			boost::property_tree::ptree &l_DepthAttr = a_ShaderSetting.get_child("DepthStencil.Depth.<xmlattr>");
			{
				l_PsoDesc.DepthStencilState.DepthEnable = l_DepthAttr.get("enable", "true") == "true";
				l_PsoDesc.DepthStencilState.DepthWriteMask = l_DepthAttr.get("write", "true") == "true" ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
				l_PsoDesc.DepthStencilState.DepthFunc = (D3D12_COMPARISON_FUNC)l_pRefDevice->getComapreFunc(CompareFunc::fromString(l_DepthAttr.get("func", "less")));
			}

			boost::property_tree::ptree &l_StencilAttr = a_ShaderSetting.get_child("DepthStencil.Stencil.<xmlattr>");
			{
				l_PsoDesc.DepthStencilState.StencilEnable = l_StencilAttr.get("enable", "false") == "true";
					
				unsigned int l_Mask = 0;
				sscanf(l_StencilAttr.get("readMask", "ff").c_str(), "%x", &l_Mask);
				l_PsoDesc.DepthStencilState.StencilReadMask = (unsigned char)l_Mask;

				sscanf(l_StencilAttr.get("writeMask", "ff").c_str(), "%x", &l_Mask);
				l_PsoDesc.DepthStencilState.StencilWriteMask = (unsigned char)l_Mask;

				boost::property_tree::ptree &l_FrontAttr = l_StencilAttr.get_child("DepthStencil.Stencil.Front.<xmlattr>");
				l_PsoDesc.DepthStencilState.FrontFace.StencilFunc = (D3D12_COMPARISON_FUNC)l_pRefDevice->getComapreFunc(CompareFunc::fromString(l_FrontAttr.get("func", "always")));
				l_PsoDesc.DepthStencilState.FrontFace.StencilDepthFailOp = (D3D12_STENCIL_OP)l_pRefDevice->getStencilOP(StencilOP::fromString(l_FrontAttr.get("depthFailOP", "keep")));
				l_PsoDesc.DepthStencilState.FrontFace.StencilPassOp = (D3D12_STENCIL_OP)l_pRefDevice->getStencilOP(StencilOP::fromString(l_FrontAttr.get("passOP", "keep")));
				l_PsoDesc.DepthStencilState.FrontFace.StencilFailOp = (D3D12_STENCIL_OP)l_pRefDevice->getStencilOP(StencilOP::fromString(l_FrontAttr.get("failOP", "keep")));

				boost::property_tree::ptree &l_BackAttr = l_StencilAttr.get_child("DepthStencil.Stencil.Back.<xmlattr>");
				l_PsoDesc.DepthStencilState.BackFace.StencilFunc = (D3D12_COMPARISON_FUNC)l_pRefDevice->getComapreFunc(CompareFunc::fromString(l_BackAttr.get("func", "always")));
				l_PsoDesc.DepthStencilState.BackFace.StencilDepthFailOp = (D3D12_STENCIL_OP)l_pRefDevice->getStencilOP(StencilOP::fromString(l_BackAttr.get("depthFailOP", "keep")));
				l_PsoDesc.DepthStencilState.BackFace.StencilPassOp = (D3D12_STENCIL_OP)l_pRefDevice->getStencilOP(StencilOP::fromString(l_BackAttr.get("passOP", "keep")));
				l_PsoDesc.DepthStencilState.BackFace.StencilFailOp = (D3D12_STENCIL_OP)l_pRefDevice->getStencilOP(StencilOP::fromString(l_BackAttr.get("failOP", "keep")));
			}
		}

		{// setup render target state
			boost::property_tree::ptree &l_RenderTargetRoot = a_ShaderSetting.get_child("RenderTarget");
			l_PsoDesc.DSVFormat = (DXGI_FORMAT)l_pRefDevice->getPixelFormat(PixelFormat::fromString(l_RenderTargetRoot.get("<xmlattr>.depthStencil", "d24_unorm_s8_uint")));

			std::vector<DXGI_FORMAT> l_RTFmtList;
			for( auto l_RTElement = l_RenderTargetRoot.begin() ; l_RTElement != l_RenderTargetRoot.end() ; ++l_RTElement )
			{
				l_RTFmtList.push_back((DXGI_FORMAT)l_pRefDevice->getPixelFormat(PixelFormat::fromString(l_RTElement->first)));
			}

			l_PsoDesc.NumRenderTargets = l_RTFmtList.size();
			unsigned int l_Idx = 0;
			for( ; l_Idx<l_RTFmtList.size() ; ++l_Idx ) l_PsoDesc.RTVFormats[l_Idx] = l_RTFmtList[l_Idx];
			for( ; l_Idx<8 ; ++l_Idx ) l_PsoDesc.RTVFormats[l_Idx] = DXGI_FORMAT_UNKNOWN;
		}

		HRESULT l_Res = l_pDeviceInst->CreateGraphicsPipelineState(&l_PsoDesc, IID_PPV_ARGS(&m_pPipeline));
		for( auto it = l_ShaderUsage.begin() ; it != l_ShaderUsage.end() ; ++it ) SAFE_RELEASE(it->second)
		l_ShaderUsage.clear();

		if( S_OK != l_Res )
		{
			wxMessageBox(wxT("Failed to create pipline object"), wxT("D3D12HLSLComponent::initDrawShader"));
			return;
		}
	}
	
	bool l_bUseIndirectDraw = a_ShaderSetting.get("<xmlattr>.useIndirectDraw", "true") == "true";
	if( l_bUseIndirectDraw )
	{// init indirect command signature
		std::vector<D3D12_INDIRECT_ARGUMENT_DESC> l_IndirectCmdDesc;
		for( unsigned int i=0 ; i<VTXSLOT_COUNT ; ++i )
		{
			D3D12_INDIRECT_ARGUMENT_DESC l_VtxSlot = {};
			l_VtxSlot.Type = D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW;
			l_VtxSlot.VertexBuffer.Slot = i;
			l_IndirectCmdDesc.push_back(l_VtxSlot);

			m_IndirectCmdSize += sizeof(D3D12_VERTEX_BUFFER_VIEW);
		}

		{
			D3D12_INDIRECT_ARGUMENT_DESC l_IdxSlot = {};
			l_IdxSlot.Type = D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW;
			l_IndirectCmdDesc.push_back(l_IdxSlot);
			
			m_IndirectCmdSize += sizeof(D3D12_INDEX_BUFFER_VIEW);
		}

		for( auto it = m_RegMap[ShaderRegType::Constant].begin() ; it != m_RegMap[ShaderRegType::Constant].end() ; ++it )
		{
			if( it->second->m_bReserved ) continue;

			D3D12_INDIRECT_ARGUMENT_DESC l_SrvSlot = {};
			l_SrvSlot.Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
			l_SrvSlot.Constant.DestOffsetIn32BitValues = it->second->m_Offset / 4;
			l_SrvSlot.Constant.Num32BitValuesToSet = it->second->m_Size / 4;
			l_SrvSlot.Constant.RootParameterIndex = it->second->m_RootIndex;
			l_IndirectCmdDesc.push_back(l_SrvSlot);

			m_IndirectCmdSize += sizeof(unsigned int);
		}

		for( auto it = m_RegMap[ShaderRegType::ConstBuffer].begin() ; it != m_RegMap[ShaderRegType::ConstBuffer].end() ; ++it )
		{
			if( it->second->m_bReserved ) continue;

			D3D12_INDIRECT_ARGUMENT_DESC l_CbvSlot = {};
			l_CbvSlot.Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW;
			l_CbvSlot.ConstantBufferView.RootParameterIndex = it->second->m_RootIndex;
			l_IndirectCmdDesc.push_back(l_CbvSlot);
			
			m_IndirectCmdSize += sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
		}
		
		for( auto it = m_RegMap[ShaderRegType::UavBuffer].begin() ; it != m_RegMap[ShaderRegType::UavBuffer].end() ; ++it )
		{
			if( it->second->m_bReserved ) continue;

			D3D12_INDIRECT_ARGUMENT_DESC l_CbvSlot = {};
			l_CbvSlot.Type = D3D12_INDIRECT_ARGUMENT_TYPE_UNORDERED_ACCESS_VIEW;
			l_CbvSlot.ConstantBufferView.RootParameterIndex = it->second->m_RootIndex;
			l_IndirectCmdDesc.push_back(l_CbvSlot);
			
			m_IndirectCmdSize += sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
		}

		{// indirect draw command
			D3D12_INDIRECT_ARGUMENT_DESC l_DrawCmd = {};
			l_DrawCmd.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;
			l_IndirectCmdDesc.push_back(l_DrawCmd);
			
			m_IndirectCmdSize += sizeof(D3D12_DRAW_INDEXED_ARGUMENTS);
		}

		D3D12_COMMAND_SIGNATURE_DESC l_CmdSignatureDesc = {};
		l_CmdSignatureDesc.pArgumentDescs = &(l_IndirectCmdDesc.front());
		l_CmdSignatureDesc.NumArgumentDescs = l_IndirectCmdDesc.size();
		l_CmdSignatureDesc.ByteStride = m_IndirectCmdSize;

		HRESULT l_Res = l_pDeviceInst->CreateCommandSignature(&l_CmdSignatureDesc, m_pRegisterDesc, IID_PPV_ARGS(&m_pIndirectFmt));
		if( S_OK != l_Res )
		{
			wxMessageBox(wxT("command signature init failed"), wxT("HLSLProgram::init"));
			return;
		}
	}
}

void HLSLProgram12::initComputeShader(boost::property_tree::ptree &a_Shaders, std::map<std::string, std::string> &a_ParamDefine)
{
	D3D12Device *l_pRefDevice = TYPED_GDEVICE(D3D12Device);
	ID3D12Device *l_pDeviceInst = l_pRefDevice->getDeviceInst();
	
	char l_Buff[256];
	snprintf(l_Buff, 256, "%s.<xmlattr>.file", static_cast<const char*>(ShaderStages::toString(ShaderStages::Compute).c_str()));
	wxString l_Filename(a_Shaders.get<std::string>(l_Buff));
				
	assert( !l_Filename.empty() );
	a_ParamDefine.insert(std::make_pair("COMPUTE_SHADER", "1"));
	ID3DBlob *l_pBinaryCode = static_cast<ID3DBlob *>(ProgramManager::singleton().getShader(this, l_Filename, ShaderStages::Compute, shaderInUse(), a_ParamDefine));

	D3D12_COMPUTE_PIPELINE_STATE_DESC l_PsoDesc = {};
	l_PsoDesc.pRootSignature = m_pRegisterDesc;
	l_PsoDesc.CS.pShaderBytecode = l_pBinaryCode->GetBufferPointer();
	l_PsoDesc.CS.BytecodeLength = l_pBinaryCode->GetBufferSize();

	HRESULT l_Res = l_pDeviceInst->CreateComputePipelineState(&l_PsoDesc, IID_PPV_ARGS(&m_pPipeline));
	assert(S_OK == l_Res);
	SAFE_RELEASE(l_pBinaryCode)
}
#pragma endregion

#pragma region HLSLComponent
//
// HLSLComponent
//
HLSLComponent::HLSLComponent()
{
}

HLSLComponent::~HLSLComponent()
{
}
	
HRESULT __stdcall HLSLComponent::Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID *ppData, UINT *pBytes)
{
	if( 0 == strcmp("autoBinder.h", pFileName) )
	{
		char *l_pFileBuff = new char[m_ParamDefine.length() + 1];
		l_pFileBuff[m_ParamDefine.length()] = '\0';
		strcpy(l_pFileBuff, m_ParamDefine.c_str());

		*ppData = l_pFileBuff;
		*pBytes = m_ParamDefine.length();

		return S_OK;
	}

	wxString l_Filepath(ProgramManager::singleton().findFullPath(pFileName));

	wxFile l_File;
	if( !l_File.Open(l_Filepath) ) return E_FAIL;

	wxFileOffset l_FileLength = l_File.Length();
	if( 0 == l_FileLength )
	{
		l_File.Close();
		return E_FAIL;
	}

	char *l_pFileBuff = new char[l_FileLength + 1];
	l_File.Read(l_pFileBuff, l_FileLength);
	l_pFileBuff[l_FileLength] = '\0';
	l_File.Close();

	*ppData = l_pFileBuff;
	*pBytes = l_FileLength;

	return S_OK;
}

HRESULT __stdcall HLSLComponent::Close(LPCVOID a_pData)
{
	char *l_pBuff = const_cast<char*>(static_cast<const char *>(a_pData));
    delete[] l_pBuff;
    return S_OK;
}

void* HLSLComponent::getShader(ShaderProgram *a_pProgrom, wxString a_Filename, ShaderStages::Key a_Stage, std::pair<int, int> a_Module, std::map<std::string, std::string> &a_ParamDefine)
{
	wxString l_ShaderName(ProgramManager::singleton().findFullPath(a_Filename));
	setupParamDefine(a_pProgrom, a_Stage);

	D3D_SHADER_MACRO *l_Macros = nullptr;
	if( !a_ParamDefine.empty() )
	{
		new D3D_SHADER_MACRO[a_ParamDefine.size()];
		unsigned int l_Idx = 0;
		for( auto it = a_ParamDefine.begin() ; it != a_ParamDefine.end() ; ++it, ++l_Idx )
		{
			l_Macros[l_Idx].Name = it->first.c_str();
			l_Macros[l_Idx].Definition = it->second.c_str();
		}
	}
	
	ID3DBlob *l_pShader = nullptr, *l_pErrorMsg = nullptr;
	D3DCompileFromFile(l_ShaderName.c_str(), l_Macros, this, "main", getCompileTarget(a_Stage, a_Module).c_str()
#ifdef _DEBUG
		, D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_ENABLE_UNBOUNDED_DESCRIPTOR_TABLES
#else
		, D3DCOMPILE_ENABLE_UNBOUNDED_DESCRIPTOR_TABLES
#endif
		, 0, &l_pShader, &l_pErrorMsg);
	if( nullptr != l_Macros ) delete [] l_Macros;

	if( nullptr != l_pErrorMsg )
	{
		wxString l_Msg(static_cast<const char *>(l_pErrorMsg->GetBufferPointer()));
		if( wxNOT_FOUND != l_Msg.Find(wxT("error")) )
		{
			wxMessageBox(wxString::Format(wxT("Failed to compile shader :\n%s"), l_Msg.c_str()), wxT("HLSLContainer::compile"));
			SAFE_RELEASE(l_pErrorMsg)
			return nullptr;
		}
		SAFE_RELEASE(l_pErrorMsg)
	}
	
	return l_pShader;
}

ShaderProgram* HLSLComponent::newProgram()
{
	if( nullptr != TYPED_GDEVICE(D3D12Device) ) return new HLSLProgram12();
	else ;

	return nullptr;
}

unsigned int HLSLComponent::calculateParamOffset(unsigned int &a_Offset, ShaderParamType::Key a_Type, unsigned int a_ArraySize)
{
	unsigned int l_Res = 0;
	unsigned int l_LineSize = sizeof(float) * 4;
	if( a_ArraySize > 1 )
	{
		if( 0 != (a_Offset % l_LineSize) ) a_Offset = (a_Offset + l_LineSize - 1) & ~l_LineSize;
		l_Res = a_Offset;
		a_Offset += a_ArraySize * l_LineSize;
	}
	else
	{
		switch( a_Type )
		{
			case ShaderParamType::int1:
			case ShaderParamType::float1:
				l_Res = a_Offset;
				a_Offset += sizeof(float) * a_ArraySize;
				break;

			case ShaderParamType::int2:
			case ShaderParamType::float2:{
				int l_Size = sizeof(float) * 2;
				if( (unsigned int)l_Size > l_LineSize - (a_Offset % l_LineSize) )
				{
					l_Res = (a_Offset + l_LineSize - 1) & ~l_LineSize;
					a_Offset = l_Res + l_Size;
				}
				else
				{
					l_Res = a_Offset;
					a_Offset += l_Size;
				}
				}break;

			case ShaderParamType::int3:
			case ShaderParamType::float3:{
				int l_Size = sizeof(float) * 3;
				if( (unsigned int)l_Size > l_LineSize - (a_Offset % l_LineSize) )
				{
					l_Res = (a_Offset + l_LineSize - 1) & ~l_LineSize;
					a_Offset = l_Res + l_Size;
				}
				else
				{
					l_Res = a_Offset;
					a_Offset += l_Size;
				}
				}break;

			case ShaderParamType::int4:
			case ShaderParamType::float4:
				if( 0 != (a_Offset % l_LineSize) ) a_Offset = (a_Offset + l_LineSize - 1) & ~l_LineSize;
				l_Res = a_Offset;
				a_Offset += l_LineSize;
				break;

			case ShaderParamType::float3x3:
				if( 0 != (a_Offset % l_LineSize) ) a_Offset = (a_Offset + l_LineSize - 1) & ~l_LineSize;
				l_Res = a_Offset;
				a_Offset += l_LineSize * 3;
				break;

			case ShaderParamType::float4x4:
				if( 0 != (a_Offset % l_LineSize) ) a_Offset = (a_Offset + l_LineSize - 1) & ~l_LineSize;
				l_Res = a_Offset;
				a_Offset += l_LineSize * 4;
				break;

			default:break;
		}
	}

	return l_Res;
}

std::string HLSLComponent::getCompileTarget(ShaderStages::Key a_Stage, std::pair<int, int> a_Module)
{
	std::string l_Res("");
	switch( a_Stage )
	{
		case ShaderStages::Compute:	l_Res = "cs_"; break;
		case ShaderStages::Vertex:	l_Res = "vs_"; break;
		case ShaderStages::TessCtrl:l_Res = "hs_"; break;
		case ShaderStages::TessEval:l_Res = "ds_"; break;
		case ShaderStages::Geometry:l_Res = "gs_"; break;
		case ShaderStages::Fragment:l_Res = "ps_"; break;
		default:break;
	}
	assert(!l_Res.empty());

	char l_Buff[8];
	snprintf(l_Buff, 8, "%d_%d", a_Module.first, a_Module.second);
	return l_Res + l_Buff;
}

void HLSLComponent::setupParamDefine(ShaderProgram *a_pProgrom, ShaderStages::Key a_Stage)
{
	char l_Buff[256];
	bool l_bWriteValid = a_Stage == ShaderStages::Fragment || a_Stage == ShaderStages::Compute;
	m_ParamDefine.clear();

	const ShaderRegType::Key c_Keys[] = {ShaderRegType::ConstBuffer, ShaderRegType::Constant};
	for each( ShaderRegType::Key l_Key in c_Keys )
	{
		std::vector<ProgramBlockDesc *> &l_Blocks = a_pProgrom->getBlockDesc(l_Key);
		for( unsigned int i=0 ; i<l_Blocks.size() ; ++i )
		{
			ProgramBlockDesc *l_pBlock = l_Blocks[i];
			snprintf(l_Buff, 256, "cbuffer %s : register(b%d) {\n", l_pBlock->m_Name.c_str(), l_pBlock->m_pRegInfo->m_Slot);
			m_ParamDefine += l_Buff;

			for( auto it=l_pBlock->m_ParamDesc.begin() ; it!=l_pBlock->m_ParamDesc.end() ; ++it )
			{
				if( it->second->m_ArraySize > 1 ) snprintf(l_Buff, 256, "\t%s %s[%d];\n", ShaderParamType::toString(it->second->m_Type).ToStdString().c_str(), it->first.c_str(), it->second->m_ArraySize);
				else snprintf(l_Buff, 256, "\t%s %s;\n", ShaderParamType::toString(it->second->m_Type).ToStdString().c_str(), it->first.c_str());
				m_ParamDefine += l_Buff;
			}

			m_ParamDefine += "};\n\n";
		}
	}

	std::vector<ProgramBlockDesc *> &l_UavBlocks = a_pProgrom->getBlockDesc(ShaderRegType::UavBuffer);
	for( unsigned int i=0 ; i<l_UavBlocks.size() ; ++i )
	{
		ProgramBlockDesc *l_pBlock = l_UavBlocks[i];
		if( l_pBlock->m_StructureName.empty() ) l_pBlock->m_StructureName = "UAV" + l_pBlock->m_Name.substr(l_pBlock->m_Name.find_first_of('_'));
		m_ParamDefine += "struct "+ l_pBlock->m_StructureName + " {\n";

		for( auto it=l_pBlock->m_ParamDesc.begin() ; it!=l_pBlock->m_ParamDesc.end() ; ++it )
		{
			snprintf(l_Buff, 256, "\t%s %s;\n", ShaderParamType::toString(it->second->m_Type).ToStdString().c_str(), it->first.c_str());
			m_ParamDefine += l_Buff;
		}
		m_ParamDefine += "};\n";
		
		std::string l_FormatStr = (l_bWriteValid && l_pBlock->m_bWrite) ?
			"RWStructuredBuffer<%s> : register(u%d);\n\n" : "StructuredBuffer<%s> : register(u%d);\n\n";
		snprintf(l_Buff, 256, l_FormatStr.c_str(), l_pBlock->m_StructureName.c_str(), l_pBlock->m_pRegInfo->m_Slot);
		m_ParamDefine += l_Buff;
	}

	std::map<std::string, ProgramTextureDesc *> &l_TextureDesc = a_pProgrom->getTextureDesc();
	for( auto it=l_TextureDesc.begin() ; it!=l_TextureDesc.end() ; ++it )
	{
		std::string l_TypeStr("");
		switch( it->second->m_pRegInfo->m_Type )
		{
			case ShaderRegType::Srv2D:		l_TypeStr = "Texture2D"; break;
			case ShaderRegType::Srv2DArray: l_TypeStr = "Texture2DArray"; break;
			case ShaderRegType::Srv3D:		l_TypeStr = "Texture3D"; break;
			case ShaderRegType::SrvCube:
				l_TypeStr = "TextureCube";
				assert(!it->second->m_bWrite);
				break;

			case ShaderRegType::SrvCubeArray:
				l_TypeStr= "TextureCubeArray";
				assert(!it->second->m_bWrite);
				break;

			default:break;
		}

		if( it->second->m_bWrite && l_bWriteValid )
		{
			snprintf(l_Buff, 256, "RW%s<%s> %s : register(t%d);\n", l_TypeStr.c_str(), ShaderParamType::toString(it->second->m_Type).ToStdString().c_str(), it->first.c_str(), it->second->m_pRegInfo->m_Slot);
			m_ParamDefine += l_Buff;
		}
		else
		{
			snprintf(l_Buff, 256, "%s %s : register(t%d);\n", l_TypeStr.c_str(), it->first.c_str(), it->second->m_pRegInfo->m_Slot);
			m_ParamDefine += l_Buff;
		}
	}
}
#pragma endregion

}