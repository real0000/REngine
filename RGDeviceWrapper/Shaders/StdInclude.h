#define SCENE_GROUP_TYPE_COUNT 6
#define MATH_PI 3.1415926535898

#define VTXFLAG_POSITION	0x01
#define VTXFLAG_TEXCOORD	0x02
#define VTXFLAG_NORMAL		0x04
#define VTXFLAG_TANGENT		0x08
#define VTXFLAG_BINORMAL	0x10
#define VTXFLAG_BONE		0x20
#define VTXFLAG_WEIGHT		0x40
#define VTXFLAG_COLOR		0x80

#define USE_WORLD_MAT		0x100
#define USE_SKIN			0x200
#define USE_NORMAL_TEXTURE	0x400
#define WITHOUT_VP_MAT		0x800

#ifdef GLSL
	#define float2 vec2
	#define float3 vec3
	#define float4 vec4
	#define vout_Position gl_Position 

	#define vs_main()			\
		in float3 vin_Position;	\
		in float3 vin_Texcoord0;\
		in float3 vin_Texcoord1;\
		in float3 vin_Texcoord2;\
		in float3 vin_Texcoord3;\
		in float3 vin_Texcoord4;\
		in float3 vin_Texcoord5;\
		in float3 vin_Texcoord6;\
		in float3 vin_Texcoord7;\
		in float3 vin_Normal;	\
		in float3 vin_Tangent;	\
		in float3 vin_Binormal;	\
		in int4 vin_BoneID;		\
		in float4 vin_Weight;	\
		in float4 vin_Color;	\
								\
		out float3 vout_Texcoord0;	\
		out float3 vout_Texcoord1;	\
		out float3 vout_Texcoord2;	\
		out float3 vout_Texcoord3;	\
		out float3 vout_Texcoord4;	\
		out float3 vout_Texcoord5;	\
		out float3 vout_Texcoord6;	\
		out float3 vout_Texcoord7;	\
		out float4 vout_Normal;		\
		out float4 vout_Tangent;	\
		out float4 vout_Binormal;	\
		out float4 vout_Color;		\
		void main()


#else
	#define vs_main()									\
		void main(in float3 vin_Position	: POSITION0	\
				, in float3 vin_Texcoord0	: TEXCOORD0	\
				, in float3 vin_Texcoord1	: TEXCOORD1	\
				, in float3 vin_Texcoord2	: TEXCOORD2	\
				, in float3 vin_Texcoord3	: TEXCOORD3	\
				, in float3 vin_Texcoord4	: TEXCOORD4	\
				, in float3 vin_Texcoord5	: TEXCOORD5	\
				, in float3 vin_Texcoord6	: TEXCOORD6	\
				, in float3 vin_Texcoord7	: TEXCOORD7	\
				, in float3 vin_Normal		: NORMAL0	\
				, in float3 vin_Tangent		: TANGENT0	\
				, in float3 vin_Binormal	: BINORMAL0	\
				, in int4 vin_BoneID		: BLENDINDICES0	\
				, in float4 vin_Weight		: BLENDWEIGHT0	\
				, in float4 vin_Color		: COLOR0	\
				, out float3 vout_Texcoord0	: TEXCOORD0	\
				, out float3 vout_Texcoord1	: TEXCOORD1	\
				, out float3 vout_Texcoord2	: TEXCOORD2	\
				, out float3 vout_Texcoord3	: TEXCOORD3	\
				, out float3 vout_Texcoord4	: TEXCOORD4	\
				, out float3 vout_Texcoord5	: TEXCOORD5	\
				, out float3 vout_Texcoord6	: TEXCOORD6	\
				, out float3 vout_Texcoord7	: TEXCOORD7	\
				, out float4 vout_Normal	: NORMAL0	\
				, out float4 vout_Tangent	: TANGENT0	\
				, out float4 vout_Binormal	: BINORMAL0	\
				, out float4 vout_Color		: COLOR0	\
				, out float4 vout_Position	: SV_POSITION)
#endif

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

/*cbuffer VtxFlag : register(b1)
{
	uint m_VtxFlag;
};

cbuffer CameraBuffer : register(b2)
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

