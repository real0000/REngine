#define SCENE_GROUP_TYPE_COUNT 6
#define MATH_PI 3.1415926535898

#define VTXFLAG_POSITION	0x01
#define VTXFLAG_TEXCOORD01	0x02
#define VTXFLAG_TEXCOORD23	0x04
#define VTXFLAG_TEXCOORD45	0x08
#define VTXFLAG_TEXCOORD67	0x10
#define VTXFLAG_NORMAL		0x20
#define VTXFLAG_TANGENT		0x40
#define VTXFLAG_BINORMAL	0x80
#define VTXFLAG_BONE		0x100
#define VTXFLAG_WEIGHT		0x200
#define VTXFLAG_COLOR		0x400

#define USE_WORLD_MAT		0x10000
#define USE_SKIN			0x20000
#define USE_NORMAL_TEXTURE	0x40000
#define WITHOUT_VP_MAT		0x80000

#ifdef COMPUTE_SHADER

#else
	struct VS_CommonInput
	{
		float3 m_Position : POSITION0;
		float4 m_Texcoord0 : TEXCOORD0;
		float4 m_Texcoord23 : TEXCOORD1;
		float4 m_Texcoord45 : TEXCOORD2;
		float4 m_Texcoord67 : TEXCOORD3;
		float3 m_Normal : NORMAL0;
		float3 m_Tangent : TANGENT0;
		float3 m_Binormal : BINORMAL0;
		int4 m_BoneID : BLENDINDICES0;
		float4 m_Weight : BLENDWEIGHT0;
		float4 m_Color : COLOR0;
	};

	struct PS_CommonInput
	{
		float4 m_UV : TEXCOORD0;
		float4 m_Normal : NORMAL0;
		float4 m_Tangent : TANGENT0;
		float4 m_Binormal : BINORMAL0;
		float4 m_Color : COLOR0;
	};

	struct PS_CommonOutput
	{
		float4 m_Target0 : SV_TARGET0;// rgb : world normal, a : distance/height map flag
		float4 m_Target1 : SV_TARGET1;// r : metallic, g : specular, b : roughness, a : shaing mode id(7 bit), decal mask(1 bit)
		float4 m_Target2 : SV_TARGET2;// rgb : diffuse, (a : ao?)
		float4 m_Target3 : SV_TARGET3;// r : opacity, gba : mask g-buffer(ssao, subsurface scattering, wet surface mask, skylight mask ...)
		float4 m_Target4 : SV_TARGET4;// pre compute shader factor
		float2 m_Target5 : SV_TARGET5;// motion blur
	};
	
	struct CameraBuffer
	{
		float4x4 m_View;
		float4x4 m_Projection;
		float4x4 m_ViewProjection;
		float4x4 m_InvView;
		float4 m_CameraParam;// screen width, screen height, near, far
	};

	float3 encodeNormal(float3 a_Normal)
	{
		return a_Normal * 0.5 + 0.5;
	}

	float3 decodeNormal(float3 a_EncNormal)
	{
		return a_EncNormal * 2.0 - 1.0;
	}

	float3 getWorldPosition(float a_Depth, float2 a_ViewPos)
	{
		double4 l_Res = float4(a_ViewPos, a_Depth, 1.0) * float4(2.0, 2.0, 2.0, 1.0) - float4(1.0, 1.0, 1.0, 0.0);
		l_Res = mul(m_InvViewProjection, l_Res);
    
		return float3(l_Res.xyz / l_Res.w);
	}
#endif

// for 32-bit constant block
auto_bind_constant32_block0;
auto_bind_constant32_block1;
auto_bind_constant32_block2;
auto_bind_constant32_block3;
auto_bind_constant32_block4;
auto_bind_constant32_block5;

// static samplers
SamplerState s_LinearSampler : register(s0);
SamplerState s_PointSampler : register(s1);
SamplerState s_AnisotropicSampler : register(s2);
