// MathExtend.cpp
//
// 2017/05/26 Ian Wu/Real0000
//

#include "CommonUtil.h"

namespace glm
{

#pragma region Ray
//
// Ray
//
Ray::Ray()
    : m_Origin(0.0f, 0.0f, 0.0f)
    , m_Direction(1.0f, 0.0f, 0.0f)
{}

Ray::Ray(vec3 &l_Origin, vec3 &l_Direction)
    : m_Origin(l_Origin)
    , m_Direction(normalize(l_Direction))
{
}

Ray::Ray(float l_OriginX, float l_OriginY, float l_OriginZ, float l_DirX, float l_DirY, float l_DirZ)
    : m_Origin(l_OriginX, l_OriginY, l_OriginZ)
    , m_Direction(normalize(vec3(l_DirX, l_DirY, l_DirZ)))
{
}

Ray::~Ray()
{}

bool Ray::intersect(Plane l_Plane, float &l_OutLength)
{
    float l_Den = l_Plane.m_Plane.x * m_Direction.x + l_Plane.m_Plane.y * m_Direction.y + l_Plane.m_Plane.z * m_Direction.z;
    if( 0.0f == l_Den ) return false;

    float l_Num = -l_Plane.m_Plane.w - l_Plane.m_Plane.x * m_Origin.x - l_Plane.m_Plane.y * m_Origin.y - l_Plane.m_Plane.z * m_Origin.z;
    l_OutLength = l_Num / l_Den;
    return l_OutLength >= 0.0f;
}

bool Ray::intersect(Sphere l_Sphere, float &l_OutLength)
{
    float l_Sx = m_Origin.x - l_Sphere.m_Center.x;
    float l_Sy = m_Origin.y - l_Sphere.m_Center.y;
    float l_Sz = m_Origin.z - l_Sphere.m_Center.z;
    float l_A = m_Direction.x * m_Direction.x + m_Direction.y * m_Direction.y + m_Direction.z * m_Direction.z;
    float l_B = 2.0f * (l_Sx * m_Direction.x + l_Sy * m_Direction.y + l_Sz * m_Direction.z);
    float l_C = l_Sx * l_Sx + l_Sy * l_Sy + l_Sz * l_Sz - l_Sphere.m_Range * l_Sphere.m_Range;

    float l_B2_4AC = l_B * l_B - 4.0f * l_A * l_C;
    if( l_B2_4AC < 0 ) return false;

    float l_SqrtB24AC = sqrt(l_B2_4AC);
    float l_Length[2] = {(-l_B + l_SqrtB24AC) / (l_A * 2.0f), (-l_B - l_SqrtB24AC) / (l_A * 2.0f)};
    if( l_Length[0] < 0.0f || l_Length[1] < 0.0f ) l_OutLength = std::max(l_Length[0], l_Length[1]);
    else l_OutLength = std::min(l_Length[0], l_Length[1]);

    return true;
}

bool Ray::intersect(AABB l_AABB, float &l_OutLength)
{
    Plane l_Plans[6] = {Plane(1.0f, 0.0f, 0.0f, -l_AABB.getMaxX()),
                        Plane(1.0f, 0.0f, 0.0f, -l_AABB.getMinX()),
                        Plane(0.0f, 1.0f, 0.0f, -l_AABB.getMaxY()),
                        Plane(0.0f, 1.0f, 0.0f, -l_AABB.getMinY()),
                        Plane(0.0f, 0.0f, 1.0f, -l_AABB.getMaxZ()),
                        Plane(0.0f, 0.0f, 1.0f, -l_AABB.getMinZ())};
    vec3 l_Min(l_AABB.getMin()), l_Max(l_AABB.getMax());

    l_OutLength = FLT_MAX;
    bool l_bIntersect = false;
    for( unsigned int i=0 ; i<6 ; ++i )
    {
        float l_Length = 0.0f;
        if( intersect(l_Plans[i], l_Length) )
        {
            vec3 l_Pt = m_Origin + l_Length * m_Direction;
            if( l_Pt.x >= l_Min.x && l_Pt.y >= l_Min.y && l_Pt.z >= l_Min.z &&
                l_Pt.x <= l_Max.x && l_Pt.y <= l_Max.y && l_Pt.z <= l_Max.z )
            {
                l_bIntersect = true;
                if( l_Length < l_OutLength ) l_OutLength = l_Length;
            }
        }
    }

    return l_bIntersect;
}

bool Ray::intersect(OBB l_OBB, float &l_OutLength)
{
    mat4x4 l_Inverse(inverse(l_OBB.m_Transition));

    vec4 l_NewOrigin(l_Inverse * vec4(m_Origin.x, m_Origin.y, m_Origin.z, 1.0f));
    vec4 l_NewDir(l_Inverse * vec4(m_Direction, 0.0f));
    Ray l_Temp(l_NewOrigin.x, l_NewOrigin.y, l_NewOrigin.z, l_NewDir.x, l_NewDir.y, l_NewDir.z);
    return l_Temp.intersect(AABB(vec3(0.0f, 0.0f, 0.0f), l_OBB.m_Size), l_OutLength);
}

float Ray::distance(Sphere l_Sphere, float &l_Radian, float &l_Z)
{
    vec3 l_Dir1(m_Direction);
    vec3 l_Dir2(l_Sphere.m_Center - m_Origin);
    l_Radian = acos(dot(normalize(l_Dir1), normalize(l_Dir2)));
    float l_Length = length(l_Dir2);
    l_Z = l_Length * cos(l_Radian);
    return l_Length * sin(l_Radian);
}
#pragma endregion

#pragma region Plane
//
// Plane
//
Plane::Plane()
    : m_Plane(1.0f, 0.0f, 0.0, -1.0f)
{
}

Plane::Plane(vec4 &l_Plane)
    : m_Plane(l_Plane)
{
}

Plane::Plane(float l_A, float l_B, float l_C, float l_D)
    : m_Plane(l_A, l_B, l_C, l_D)
{
}

Plane::Plane(vec3 &l_Point, vec3 &l_Normal)
{
    m_Plane.x = l_Normal.x;
    m_Plane.y = l_Normal.y;
    m_Plane.z = l_Normal.z;
    m_Plane.w = -l_Normal.x * l_Point.x - l_Normal.y * l_Point.y - l_Normal.z * l_Point.z;
}

Plane::~Plane()
{
}
#pragma endregion

#pragma region Sphere
//
// Sphere
//
Sphere::Sphere()
{
}

Sphere::Sphere(vec3 &l_Center, float l_Range)
    : m_Center(l_Center)
    , m_Range(l_Range)
{
}

Sphere::Sphere(float l_CenterX, float l_CenterY, float l_CenterZ, float l_Range)
    : m_Center(l_CenterX, l_CenterY, l_CenterZ)
    , m_Range(l_Range)
{
    assert(m_Range > 0.0f);
}

Sphere::Sphere(AABB &l_AABB)
    : m_Center(l_AABB.m_Center)
    , m_Range(std::max(l_AABB.m_Size.z, std::max(l_AABB.m_Size.x, l_AABB.m_Size.y)) * 0.5f)
{
    assert(m_Range > 0.0f);
}

Sphere::Sphere(OBB &l_OBB)
    : m_Range(std::max(l_OBB.m_Size.z, std::max(l_OBB.m_Size.x, l_OBB.m_Size.y)) * 0.5f)
    , m_Center(l_OBB.m_Transition[3].x, l_OBB.m_Transition[3].y, l_OBB.m_Transition[3].z)
{
    assert(m_Range > 0.0f);
}

Sphere::~Sphere()
{
}
#pragma endregion

#pragma region AABB
//
// AABB
//
AABB::AABB()
{
}

AABB::AABB(vec3 l_Center, vec3 l_Size)
    : m_Center(l_Center)
    , m_Size(l_Size)
{
}

AABB::AABB(OBB &l_Box)
    : m_Center(0.0f, 0.0f, 0.0f)
    , m_Size(l_Box.m_Size)
{
    translate(l_Box.m_Transition);
}

AABB::~AABB()
{
}

float AABB::getMinX()
{
    return m_Center.x - 0.5f * m_Size.x;
}

float AABB::getMinY()
{
    return m_Center.y - 0.5f * m_Size.y;
}

float AABB::getMinZ()
{
    return m_Center.z - 0.5f * m_Size.z;
}

vec3 AABB::getMin()
{
    return vec3(getMinX(), getMinY(), getMinZ());
}

float AABB::getMaxX()
{
    return m_Center.x + 0.5f * m_Size.x;
}

float AABB::getMaxY()
{
    return m_Center.y + 0.5f * m_Size.y;
}

float AABB::getMaxZ()
{
    return m_Center.z + 0.5f * m_Size.z;
}

vec3 AABB::getMax()
{
    return vec3(getMaxX(), getMaxY(), getMaxZ());
}

void AABB::translate(mat4x4 &l_World)
{
    vec4 l_NewCenter = l_World * vec4(m_Center.x, m_Center.y, m_Center.z, 1.0f);
    m_Size = vec3(abs(l_World[0].x) * m_Size.x + abs(l_World[0].y) * m_Size.y + abs(l_World[0].z) * m_Size.z
                , abs(l_World[1].x) * m_Size.x + abs(l_World[1].y) * m_Size.y + abs(l_World[1].z) * m_Size.z
                , abs(l_World[2].x) * m_Size.x + abs(l_World[2].y) * m_Size.y + abs(l_World[2].z) * m_Size.z);
    m_Center = vec3(l_NewCenter.x, l_NewCenter.y, l_NewCenter.z);
}

bool AABB::intersect(AABB &l_Box)
{
    for( unsigned int i=0 ; i<3 ; ++i )
    {
        if( std::abs(m_Center[i] - l_Box.m_Center[i]) > (m_Size[i] + l_Box.m_Size[i]) * 0.5f ) return false;
    }
    return true;
}

bool AABB::inRange(AABB &l_Box)
{
    for( unsigned int i=0 ; i<3 ; ++i )
    {
        if( 0.5f * (m_Size[i] - l_Box.m_Size[i]) < std::abs(m_Center[i] - l_Box.m_Center[i]) ) return false;
    }
    return true;
}
#pragma endregion

#pragma region OBB
//
// OBB
//
OBB::OBB()
{
}

OBB::OBB(vec3 &l_Size, mat4x4 &l_Transition)
    : m_Size(l_Size)
    , m_Transition(l_Transition)
{
}

OBB::~OBB()
{
}
#pragma endregion

#pragma region Viewport
//
// Viewport
//
Viewport::Viewport()
	: m_Start(0.0f, 0.0f)
	, m_Size(0.0f, 0.0f)
    , m_MinMaxDepth(0.0f, 0.0f)
{	
}

Viewport::Viewport(float l_StartX, float l_StartY, float l_Width, float l_Height, float l_MinDepth, float l_MaxDepth)
	: m_Start(l_StartX, l_StartY)
	, m_Size(l_Width, l_Height)
    , m_MinMaxDepth(l_MinDepth, l_MaxDepth)
{
}

Viewport::Viewport(vec2 l_Start, vec2 l_Size, vec2 l_MinMaxDepth)
	: m_Start(l_Start)
	, m_Size(l_Size)
    , m_MinMaxDepth(l_MinMaxDepth)
{
}

Viewport::~Viewport()
{	
}
#pragma endregion

}