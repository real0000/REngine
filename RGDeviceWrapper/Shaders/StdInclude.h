#define SCENE_GROUP_TYPE_COUNT 6
#define MATH_PI 3.1415926535898
#define FLT_MAX 3.402823466e+38

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
#define USE_TESSELLATION	0x100000

#define LIGHT_TYPE_OMNI	2
#define LIGHT_TYPE_SPOT	3
#define LIGHT_TYPE_DIR	4

float intersectRayPlane(float3 a_RayOrigin, float3 a_RayDir, float4 a_Plane)
{
	float l_Den = dot(a_Plane, a_RayDir);
	if( 0.0 == l_Den ) return -FLT_MAX;

	float l_Num = -a_Plane.w - dot(a_Plane, a_RayOrigin);
	float l_Res = l_Num / l_Den;
	return l_Res;
}

float intersectRayAABB(float3 a_RayOrigin, float3 a_RayDir, float3 a_BoxCenter, float3 a_BoxSize)
{
	float3 l_Min = a_BoxCenter.xyz - 0.5 * a_BoxSize.xyz;
	float3 l_Max = a_BoxCenter.xyz + 0.5 * a_BoxSize.xyz;
	float4 l_Plans[6] = {float4(1.0, 0.0, 0.0, -l_Max.x)
						,float4(1.0, 0.0, 0.0, -l_Min.x)
						,float4(0.0, 1.0, 0.0, -l_Max.y)
						,float4(0.0, 1.0, 0.0, -l_Min.y)
						,float4(0.0, 0.0, 1.0, -l_Max.z)
						,float4(0.0, 0.0, 1.0, -l_Min.z)};

	bool l_bIntersect = false;
	float l_Res = FLT_MAX;
	for( unsigned int i=0 ; i<6 ; ++i )
	{
		float l_Length = intersectRayPlane(a_RayOrigin, a_RayDir, l_Plans[i]);
		if( l_Length >= 0.0 )
		{
			float3 l_Pt = a_RayOrigin + l_Length * m_Direction;
			if( l_Pt.x >= l_Min.x && l_Pt.y >= l_Min.y && l_Pt.z >= l_Min.z &&
				l_Pt.x <= l_Max.x && l_Pt.y <= l_Max.y && l_Pt.z <= l_Max.z )
			{
				l_bIntersect = true;
				if( l_Length < l_Res ) l_Res = l_Length;
			}
		}
	}
	if( !l_bIntersect ) return -FLT_MAX;
	return l_Res;
}

float intersectRayTriangle(float3 a_RayOrigin, float3 a_RayDir, float3 a_Pos1, float3 a_Pos2, float3 a_Pos3)
{
	float3 l_PlaneNormal = cross(a_Pos2 - a_Pos1, a_Pos3 - a_Pos1);
	float4 l_Plane = float4(l_PlaneNormal.x, l_PlaneNormal.y, l_PlaneNormal.z, - dot(l_PlaneNormal, a_Pos1));

	float l_OutLength = intersectRayPlane(a_RayOrigin, a_RayDir, l_Plane);
	if( l_OutLength >= 0 )
	{
		float3 l_Pt = float3(a_RayOrigin + l_OutLength * a_RayDir);
		if( dot(l_Pt - a_Pos1, a_Pos2 - a_Pos1) >= 0.0 
			&& dot(l_Pt - a_Pos2, a_Pos3 - a_Pos2) >= 0.0
			&& dot(l_Pt - a_Pos3, a_Pos1 - a_Pos3) >= 0.0 ) return l_OutLength;
	}
	return -FLT_MAX;
}

#ifdef COMPUTE_SHADER

#else
	struct VS_CommonInput
	{
		float3 m_Position : POSITION0;
		float4 m_Texcoord01 : TEXCOORD0;
		float4 m_Texcoord23 : TEXCOORD1;
		float4 m_Texcoord45 : TEXCOORD2;
		float4 m_Texcoord67 : TEXCOORD3;
		float3 m_Normal : NORMAL0;
		float3 m_Tangent : TANGENT0;
		float3 m_Binormal : BINORMAL0;
		uint4 m_BoneID : BLENDINDICES0;
		float4 m_Weight : BLENDWEIGHT0;
		float4 m_Color : COLOR0;
		uint4 m_InstanceID : TEXCOORD4;
	};
	struct VS_CommonOutput
	{
		float4 m_Texcoord01 : TEXCOORD0;
		float4 m_Texcoord23 : TEXCOORD1;
		float4 m_Texcoord45 : TEXCOORD2;
		float4 m_Texcoord67 : TEXCOORD3;
		float4 m_Normal : NORMAL0;
		float4 m_Tangent : TANGENT0;
		float4 m_Binormal : BINORMAL0;
		float4 m_Color : COLOR0;
		float4 m_Position : SV_Position;
		uint4 m_InstanceID : TEXCOORD4;
	};

	struct HS_TriangleFactor
	{
		float3 m_Edges : SV_TessFactor;
		float inside : SV_InsideTessFactor;
	};

	struct PS_CommonInput
	{
		float4 m_Texcoord01 : TEXCOORD0;
		float4 m_Texcoord23 : TEXCOORD1;
		float4 m_Texcoord45 : TEXCOORD2;
		float4 m_Texcoord67 : TEXCOORD3;
		float4 m_Normal : NORMAL0;
		float4 m_Tangent : TANGENT0;
		float4 m_Binormal : BINORMAL0;
		float4 m_Color : COLOR0;
	};

	struct PS_CommonOutput
	{
		float4 m_Target0 : SV_TARGET0;// rgb : world normal, a : distance/height map flag
		uint4 m_Target1 : SV_TARGET1;// r : metallic, g : specular, b : roughness, a : shaing mode id(7 bit), decal mask(1 bit)
		float4 m_Target2 : SV_TARGET2;// rgb : diffuse, (a : ao?)
		float4 m_Target3 : SV_TARGET3;// r : opacity, gba : mask g-buffer(ssao, subsurface scattering, wet surface mask, skylight mask ...)
		float4 m_Target4 : SV_TARGET4;// pre compute shader factor
		float2 m_Target5 : SV_TARGET5;// motion blur
	};
	
	float3 encodeNormal(float3 a_Normal)
	{
		return a_Normal * 0.5 + 0.5;
	}

	float3 decodeNormal(float3 a_EncNormal)
	{
		return a_EncNormal * 2.0 - 1.0;
	}

	/*float3 getWorldPosition(float a_Depth, float2 a_ViewPos)
	{
		double4 l_Res = float4(a_ViewPos, a_Depth, 1.0) * float4(2.0, 2.0, 2.0, 1.0) - float4(1.0, 1.0, 1.0, 0.0);
		l_Res = mul(m_InvViewProjection, l_Res);
    
		return float3(l_Res.xyz / l_Res.w);
	}*/
#endif
