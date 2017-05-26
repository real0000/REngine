// MathExtend.h
//
// 2017/05/26 Ian Wu/Real0000
//

#ifndef _MATH_EXTEND_H_
#define _MATH_EXTEND_H_

namespace glm
{

struct Ray;
struct Plane;
struct Sphere;
struct AABB;
struct OBB;
struct Viewport;

struct Ray
{
    Ray();
    Ray(vec3 &l_Origin, vec3 &l_Direction);
    Ray(float l_OriginX, float l_OriginY, float l_OriginZ, float l_DirX, float l_DirY, float l_DirZ);
    ~Ray();

    bool intersect(Plane l_Plane, float &l_OutLength);
    bool intersect(Sphere l_Sphere, float &l_OutLength);
    bool intersect(AABB l_AABB, float &l_OutLength);
    bool intersect(OBB l_OBB, float &l_OutLength);

    float distance(Sphere l_Sphere, float &l_Radian, float &l_Z);

    vec3 m_Origin;
    vec3 m_Direction;
};

struct Plane
{
    Plane();
    Plane(vec4 &l_Plane);
    Plane(float l_A, float l_B, float l_C, float l_D);
    Plane(vec3 &l_Point, vec3 &l_Normal);
    ~Plane();

    vec4 m_Plane;//ax + by + cz + d = 0
};

struct Sphere
{
    Sphere();
    Sphere(vec3 &l_Center, float l_Range);
    Sphere(float l_CenterX, float l_CenterY, float l_CenterZ, float l_Range);
    Sphere(AABB &l_AABB);
    Sphere(OBB &l_OBB);
    ~Sphere();

    vec3 m_Center;
    float m_Range;//(x - a) ^ 2 + (y - b) ^ 2 + (z - c) ^ 2 = r ^ 2
};

struct AABB
{
    AABB();
    AABB(vec3 l_Center, vec3 l_Size);
    AABB(OBB &l_Box);
    ~AABB();

    float getMinX();
    float getMinY();
    float getMinZ();
    glm::vec3 getMin();
    float getMaxX();
    float getMaxY();
    float getMaxZ();
    glm::vec3 getMax();

    void translate(mat4x4 &l_World);
    bool intersect(AABB &l_Box);
    bool inRange(AABB &l_Box);

    vec3 m_Center;
    float m_Padding1;
    vec3 m_Size;
    float m_Padding2;
};

struct OBB
{
    OBB();
    OBB(vec3 &l_Size, mat4x4 &l_Transition);
    ~OBB();

    vec3 m_Size;
    mat4x4 m_Transition;
};

// alias of D3D12_VIEWPORT or MTLViewport
struct Viewport
{
	Viewport();
	Viewport(float l_StartX, float l_StartY, float l_Width, float l_Height, float l_MinDepth, float l_MaxDepth);
	Viewport(vec2 l_Start, vec2 l_Size, vec2 l_MinMaxDepth);
	~Viewport();

#ifdef USE_METAL
	dvec2 m_Start;
    dvec2 m_Size;
    dvec2 m_MinMaxDepth;
#else
	vec2 m_Start;
    vec2 m_Size;
    vec2 m_MinMaxDepth;
#endif
};

}

#endif