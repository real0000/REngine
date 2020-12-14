#define CURR_BOX		g_Indicies[0].m_Params.x
#define CURR_HARMONIC	g_Indicies[0].m_Params.y
#define CURR_SAMPLE		g_Indicies[0].m_Params.z
#define MAX_DEPTH		((g_Indicies[0].m_Params.w & 0xff000000) >> 24)
#define MAX_SAMPLE		(g_Indicies[0].m_Params.w & 0x00ffffff)
#define LIGHT_OFFSET	g_Indicies[1].m_Params.x
#define NEXT_BOX		g_Indicies[1].m_Params.y
#define NEXT_HARMONIC	g_Indicies[1].m_Params.z
#define VALID_THREAD	g_Indicies[1].m_Params.w

#define LIGHTMAP_STATE_IDLE		0
#define LIGHTMAP_STATE_SCATTER	(1*0x00000100)
#define LIGHTMAP_STATE_THROUGH	(2*0x00000100)
#define LIGHTMAP_STATE_TOSKY	(3*0x00000100)
#define LIGHTMAP_STATE_CHILDBOX	(4*0x00000100)
#define LIGHTMAP_STATE_NEXTBOX	(5*0x00000100)
#define LIGHTMAP_STATE_TREADEND	(6*0x00000100)

#define HARMONIC_SCALE 16777216.0

#ifdef _ENCODE_
float intersectRayPlane(float3 a_RayOrigin, float3 a_RayDir, float4 a_Plane)
{
	float l_Res = -FLT_MAX;
	float l_Den = dot(a_Plane.xyz, a_RayDir);
	if( 0.0 != l_Den )
	{
		float l_Num = -a_Plane.w - dot(a_Plane.xyz, a_RayOrigin);
		l_Res = l_Num / l_Den;
	}
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
			float3 l_Pt = a_RayOrigin + l_Length * a_RayDir;
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
		if( dot(l_Pt - a_Pos1, a_Pos2 - a_Pos1) < 0.0 
			|| dot(l_Pt - a_Pos2, a_Pos3 - a_Pos2) < 0.0
			|| dot(l_Pt - a_Pos3, a_Pos1 - a_Pos3) < 0.0 ) l_OutLength = -FLT_MAX;
	}
	else l_OutLength = -FLT_MAX;
	return l_OutLength;
}

float intersectRaySphere(float3 a_RayOrigin, float3 a_RayDir, float3 a_Center, float a_Range)
{
	float3 l_S = a_RayOrigin - a_Center;
	float l_A = dot(a_RayDir, a_RayDir);
	float l_B = 2.0 * dot(l_S, a_RayDir);
	float l_C = dot(l_S, l_S) - a_Range * a_Range;

	float l_B2_4AC = l_B * l_B - 4.0 * l_A * l_C;
	if( l_B2_4AC < 0 ) return -FLT_MAX;

	float l_SqrtB24AC = sqrt(l_B2_4AC);
	float2 l_Length = float2((-l_B + l_SqrtB24AC) / (l_A * 2.0), (-l_B - l_SqrtB24AC) / (l_A * 2.0));
	float l_Res = 0.0;
	if( l_Length.x < 0.0 || l_Length.y < 0.0 ) l_Res = max(l_Length.x, l_Length.y);
	else l_Res = min(l_Length.x, l_Length.y);
	return l_Res;
}

float2 sphereRandom(float a_Seed)
{
	float2 l_Res;

	a_Seed = a_Seed * 9301.0 + 49297.0;
	a_Seed -= 233280.0 * floor(a_Seed / 233280.0);
	float l_Rnd = a_Seed / 233280.0;

	l_Res.x = l_Rnd * MATH_PI * 2.0;

	a_Seed = a_Seed * 9301.0 + 49297.0;
	a_Seed -= 233280.0 * floor(a_Seed / 233280.0);
	l_Rnd = a_Seed / 233280.0;

	l_Res.y = (l_Rnd - 0.5) * MATH_PI;

	return l_Res;
}

void encodeHarmonic(LightmapIntersectResult a_Res)
{
	float x = a_Res.m_OriginNormal.x;
    float y = a_Res.m_OriginNormal.y;
    float z = a_Res.m_OriginNormal.z;
    float x2 = x*x;
    float y2 = y*y;
    float z2 = z*z;

	double l_Basis[16];
	l_Basis[0]  = 1.0 / 2.0 * sqrt(1.0 / MATH_PI);
    l_Basis[1]  = sqrt(3.0 / (4.0 * MATH_PI))*z;
    l_Basis[2]  = sqrt(3.0 / (4.0 * MATH_PI))*y;
    l_Basis[3]  = sqrt(3.0 / (4.0 * MATH_PI))*x;
    l_Basis[4]  = 1.0 / 2.0 * sqrt(15.0 / MATH_PI) * x * z;
    l_Basis[5]  = 1.0 / 2.0 * sqrt(15.0 / MATH_PI) * z * y;
    l_Basis[6]  = 1.0 / 4.0 * sqrt(5.0 / MATH_PI) * (-x2 - z2 + 2 * y2);
    l_Basis[7]  = 1.0 / 2.0 * sqrt(15.0 / MATH_PI) * y * x;
    l_Basis[8]  = 1.0 / 4.0 * sqrt(15.0 / MATH_PI) * (x2 - z2);
    l_Basis[9]  = 1.0 / 4.0 * sqrt(35.0 / (2.f*MATH_PI))*(3 * x2 - z2)*z;
    l_Basis[10] = 1.0 / 2.0 * sqrt(105.0 / MATH_PI)*x*z*y;
    l_Basis[11] = 1.0 / 4.0 * sqrt(21.0 / (2.f * MATH_PI))*z*(4 * y2 - x2 - z2);
    l_Basis[12] = 1.0 / 4.0 * sqrt(7.0 / MATH_PI)*y*(2 * y2 - 3 * x2 - 3 * z2);
    l_Basis[13] = 1.0 / 4.0 * sqrt(21.0 / (2.f * MATH_PI))*x*(4 * y2 - x2 - z2);
    l_Basis[14] = 1.0 / 4.0 * sqrt(105.0 / MATH_PI)*(x2 - z2)*y;
    l_Basis[15] = 1.0 / 4.0 * sqrt(35.0 / (2 * MATH_PI))*(x2 - 3 * z2)*x;

	double weight = 4.0 * MATH_PI;
    double factor = weight / MAX_SAMPLE;
    for(int n=0; n<16 ; ++n)
    {
		double3 l_Res = a_Res.m_Emissive * l_Basis[n] * factor * HARMONIC_SCALE;
		int4 l_Add = int4(l_Res.x, l_Res.y, l_Res.z, 0);
		InterlockedAdd(Harmonics[a_Res.m_HarmonicsID.x + n].m_Params.x, l_Add.x);
		InterlockedAdd(Harmonics[a_Res.m_HarmonicsID.x + n].m_Params.y, l_Add.y);
		InterlockedAdd(Harmonics[a_Res.m_HarmonicsID.x + n].m_Params.z, l_Add.z);
    }
}

#else

float4 decodeHarmonic(float3 a_Normal, int a_HarmonicOffset)
{
	float x = a_Normal.x;
    float y = a_Normal.y;
    float z = a_Normal.z;
    float x2 = x*x;
    float y2 = y*y;
    float z2 = z*z;
	
	double l_Basis[16];
	l_Basis[0]  = 1.0 / 2.0 * sqrt(1.0 / MATH_PI);
    l_Basis[1]  = sqrt(3.0 / (4.0 * MATH_PI))*z;
    l_Basis[2]  = sqrt(3.0 / (4.0 * MATH_PI))*y;
    l_Basis[3]  = sqrt(3.0 / (4.0 * MATH_PI))*x;
    l_Basis[4]  = 1.0 / 2.0 * sqrt(15.0 / MATH_PI) * x * z;
    l_Basis[5]  = 1.0 / 2.0 * sqrt(15.0 / MATH_PI) * z * y;
    l_Basis[6]  = 1.0 / 4.0 * sqrt(5.0 / MATH_PI) * (-x2 - z2 + 2 * y2);
    l_Basis[7]  = 1.0 / 2.0 * sqrt(15.0 / MATH_PI) * y * x;
    l_Basis[8]  = 1.0 / 4.0 * sqrt(15.0 / MATH_PI) * (x2 - z2);
    l_Basis[9]  = 1.0 / 4.0 * sqrt(35.0 / (2.f*MATH_PI))*(3 * x2 - z2)*z;
    l_Basis[10] = 1.0 / 2.0 * sqrt(105.0 / MATH_PI)*x*z*y;
    l_Basis[11] = 1.0 / 4.0 * sqrt(21.0 / (2.f * MATH_PI))*z*(4 * y2 - x2 - z2);
    l_Basis[12] = 1.0 / 4.0 * sqrt(7.0 / MATH_PI)*y*(2 * y2 - 3 * x2 - 3 * z2);
    l_Basis[13] = 1.0 / 4.0 * sqrt(21.0 / (2.f * MATH_PI))*x*(4 * y2 - x2 - z2);
    l_Basis[14] = 1.0 / 4.0 * sqrt(105.0 / MATH_PI)*(x2 - z2)*y;
    l_Basis[15] = 1.0 / 4.0 * sqrt(35.0 / (2 * MATH_PI))*(x2 - 3 * z2)*x;

	float4 l_Res = float4(0.0, 0.0, 0.0, 1.0);
    for( int i=0 ; i<16 ; ++i )
    {
        l_Res.xyz += Harmonics[a_HarmonicOffset + i].m_Params.xyz / HARMONIC_SCALE * l_Basis[i];
    }
    
    return l_Res;
}

#endif