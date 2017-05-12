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

#ifdef GLSL
	#define TAG(p)
	#define vs_main()			\
		in float3 vin_Position;	\
		in float4 vin_Texcoord01;\
		in float4 vin_Texcoord23;\
		in float4 vin_Texcoord45;\
		in float4 vin_Texcoord67;\
		in float3 vin_Normal;	\
		in float3 vin_Tangent;	\
		in float3 vin_Binormal;	\
		in int4 vin_BoneID;		\
		in float4 vin_Weight;	\
		in float4 vin_Color;	\
								\
		out float4 vout_Texcoord01;	\
		out float4 vout_Texcoord23;	\
		out float4 vout_Texcoord45;	\
		out float4 vout_Texcoord67;	\
		out float4 vout_Normal;		\
		out float4 vout_Tangent;	\
		out float4 vout_Binormal;	\
		out float4 vout_Color;		\
		void main()


#else
	#define TAG(p) :p
	#define vs_main()									\
		void main(in float3 vin_Position	: POSITION0	\
				, in float4 vin_Texcoord01	: TEXCOORD0	\
				, in float4 vin_Texcoord23	: TEXCOORD1	\
				, in float4 vin_Texcoord45	: TEXCOORD2	\
				, in float4 vin_Texcoord67	: TEXCOORD3	\
				, in float3 vin_Normal		: NORMAL0	\
				, in float3 vin_Tangent		: TANGENT0	\
				, in float3 vin_Binormal	: BINORMAL0	\
				, in int4 vin_BoneID		: BLENDINDICES0	\
				, in float4 vin_Weight		: BLENDWEIGHT0	\
				, in float4 vin_Color		: COLOR0	\
				, out float3 vout_Texcoord01: TEXCOORD0	\
				, out float3 vout_Texcoord23: TEXCOORD1	\
				, out float3 vout_Texcoord45: TEXCOORD2	\
				, out float3 vout_Texcoord67: TEXCOORD3	\
				, out float4 vout_Normal	: NORMAL0	\
				, out float4 vout_Tangent	: TANGENT0	\
				, out float4 vout_Binormal	: BINORMAL0	\
				, out float4 vout_Color		: COLOR0	\
				, out float4 vout_Position	: SV_POSITION)
#endif

struct PS_CommonInput
{
	float4 m_UV TAG(TEXCOORD0);
	float4 m_Normal TAG(NORMAL0);
	float4 m_Tangent TAG(TANGENT0);
	float4 m_Binormal TAG(BINORMAL0);
	float4 m_Color TAG(COLOR0);
};

struct PS_CommonOutput
{
	float4 m_Target0 TAG(SV_TARGET0);// rgb : world normal, a : distance/height map flag
	float4 m_Target1 TAG(SV_TARGET1);// r : metallic, g : specular, b : roughness, a : shaing mode id(7 bit), decal mask(1 bit)
	float4 m_Target2 TAG(SV_TARGET2);// rgb : diffuse, (a : ao?)
	float4 m_Target3 TAG(SV_TARGET3);// r : opacity, gba : mask g-buffer(ssao, subsurface scattering, wet surface mask, skylight mask ...)
	float4 m_Target4 TAG(SV_TARGET4);// pre compute shader factor
	float2 m_Target5 TAG(SV_TARGET5);// motion blur
};

// for 32-bit constant block
auto_bind_constant32_block0;
auto_bind_constant32_block1;
auto_bind_constant32_block2;
auto_bind_constant32_block3;
auto_bind_constant32_block4;
auto_bind_constant32_block5;

cbuffer CameraBuffer auto_bind_CameraBuffer
{
	float4x4 m_View;
	float4x4 m_Projection;
	float4x4 m_ViewProjection;
	float4x4 m_InvView;
	float4x4 m_InvProjection;
	float4x4 m_InvViewProjection;
	float4 m_CameraParam;// screen width, screen height, near, far
	float3 m_Time;//time, sin(Time), cos(Time), 
	float m_Random;// 0 ~ 1.0
};
/*
cbuffer VertexTransition : register(b3)
{
	float4x4 m_World;
};*/

float3 encodeNormal(float3 l_Normal)
{
	return l_Normal * 0.5 + 0.5;
}

float3 decodeNormal(float3 l_EncNormal)
{
	return l_EncNormal * 2.0 - 1.0;
}

float3 getWorldPosition(float l_Depth, float2 l_ViewPos)
{
	double4 l_Res = float4(l_ViewPos, l_Depth, 1.0) * float4(2.0, 2.0, 2.0, 1.0) - float4(1.0, 1.0, 1.0, 0.0);
    l_Res = mul(m_InvViewProjection, l_Res);
    
	return float3(l_Res.xyz / l_Res.w);
}

