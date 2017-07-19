// HLSLShader.h
//
// 2014/08/13 Ian Wu/Real0000
//

#ifndef _HLSLSHADER_H_
#define _HLSLSHADER_H_

#include "ShaderBase.h"
#include <d3dcompiler.h>

namespace R
{

class HLSLProgram;
class HLSLProgram12;
class HLSLComponent;

class HLSLProgram : public ShaderProgram
{
public:
	HLSLProgram();
	virtual ~HLSLProgram();

protected:
	virtual unsigned int initParamOffset(unsigned int &a_Offset, ShaderParamType::Key a_Type);
};

class HLSLProgram12 : public HLSLProgram
{
	friend class HLSLComponent;
protected:
	virtual void init(boost::property_tree::ptree &a_Root);

private:
	HLSLProgram12();
	virtual ~HLSLProgram12();
	
public:
	ID3D12PipelineState* getPipeline(){ return m_pPipeline; }
	ID3D12RootSignature* getRegDesc(){ return m_pRegisterDesc; }
	ID3D12CommandSignature* getCommandSignature(){ return m_pIndirectFmt; }
	int getTextureSlot(unsigned int a_Stage){ return a_Stage >= m_TextureStageMap.size() ? -1 : (int)m_TextureStageMap[a_Stage]; }
	std::pair<int, int> getConstantSlot(std::string a_Name);
	int getConstBufferSlot(int a_Stage){ return (int)m_ConstStageMap.size() <= a_Stage ? -1 : (int)m_ConstStageMap[a_Stage]; }
	int getUavSlot(int a_Stage){ return (int)m_UavStageMap.size() <= a_Stage ? -1 : (int)m_UavStageMap[a_Stage]; }

private:
	void initRegister(boost::property_tree::ptree &a_Root, boost::property_tree::ptree &a_ParamDesc, std::map<std::string, std::string> &a_ParamOutput);
	void initDrawShader(boost::property_tree::ptree &a_ShaderSetting, boost::property_tree::ptree &a_Shaders, std::map<std::string, std::string> &a_ParamDefine);
	void initComputeShader(boost::property_tree::ptree &a_Shaders, std::map<std::string, std::string> &a_ParamDefine);
	void assignCmd(unsigned int &a_Offset, unsigned int a_Size);

	ID3D12PipelineState *m_pPipeline;
	ID3D12RootSignature *m_pRegisterDesc;
	ID3D12CommandSignature *m_pIndirectFmt;
	D3D_PRIMITIVE_TOPOLOGY m_Topology;
	std::map<std::string, RegisterInfo *> m_RegMap[ShaderRegType::StorageBuffer+1];
	std::vector<unsigned int> m_TextureStageMap;// stage(t#) : root slot
	std::vector<unsigned int> m_ConstStageMap;// stage(b#) : root slot
	std::vector<unsigned int> m_UavStageMap;// stage(u#) : root slot

	unsigned int m_IndirectCmdSize;
	std::vector<unsigned int> m_CmdAlignmentOffset;
};

class HLSLComponent : public ProgramManagerComponent, ID3DInclude
{
public:
	HLSLComponent();
	virtual ~HLSLComponent();

	HRESULT __stdcall Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID *ppData, UINT *pBytes);
	HRESULT __stdcall Close(LPCVOID a_pData);
	
	virtual void* getShader(wxString a_Filename, ShaderStages::Key a_Stage, std::pair<int, int> a_Module, std::map<std::string, std::string> &a_ParamDefine);
	virtual ShaderProgram* newProgram();

private:
	std::string getCompileTarget(ShaderStages::Key a_Stage, std::pair<int, int> a_Module);

	std::map<wxString, ID3DBlob*> m_ShaderMap;
};

}
#endif