// MathExtend.h
//
// 2017/05/26 Ian Wu/Real0000
//

#ifndef _MATH_EXTEND_H_
#define _MATH_EXTEND_H_

#include <limits>

namespace glm
{

template<typename T>
struct tray;
typedef tray<float> ray;
typedef tray<double> dray;

template<typename T>
struct tplane;
typedef tplane<float> plane;
typedef tplane<double> dplane;

template<typename T>
struct tsphere;
typedef tsphere<float> sphere;
typedef tsphere<double> dsphere;

template<typename T>
struct taabb;
typedef taabb<float> aabb;
typedef taabb<double> daabb;

template<typename T>
struct tobb;
typedef tobb<float> obb;
typedef tobb<double> dobb;

template<typename T>
struct tfrustumface;
typedef tfrustumface<float> frustumface;
typedef tfrustumface<double> dfrustumface;

template<typename T>
struct tviewport;
typedef tviewport<float> viewport;
typedef tviewport<double> dviewport;

#pragma region ray
//
// ray
//
template<typename T>
struct tray
{
    tray()
		: m_Origin(0.0, 0.0, 0.0)
		, m_Direction(1.0, 0.0, 0.0)
	{
	}
    
	tray(tvec3<T> &a_Origin, tvec3<T> &a_Direction)
		: m_Origin(a_Origin)
		, m_Direction(normalize(a_Direction))
	{
	}

    tray(T a_OriginX, T a_OriginY, T a_OriginZ, T a_DirX, T a_DirY, T a_DirZ)
		: m_Origin(a_OriginX, a_OriginY, a_OriginZ)
		, m_Direction(normalize(vec3(a_DirX, a_DirY, a_DirZ)))
	{
	}

    ~tray()
	{
	}

	template<typename PlaneType>
    bool intersectPlane(PlaneType a_Plane, T &a_OutLength)
	{
		float l_Den = a_Plane.m_Plane.x * m_Direction.x + a_Plane.m_Plane.y * m_Direction.y + a_Plane.m_Plane.z * m_Direction.z;
		if( 0.0 == l_Den ) return false;

		float l_Num = -a_Plane.m_Plane.w - a_Plane.m_Plane.x * m_Origin.x - a_Plane.m_Plane.y * m_Origin.y - a_Plane.m_Plane.z * m_Origin.z;
		a_OutLength = l_Num / l_Den;
		return a_OutLength >= 0.0;
	}

	bool intersect(plane a_Plane, T &a_OutLength)
	{
		return intersectPlane<plane>(a_Plane, a_OutLength);
	}

	bool intersect(dplane a_Plane, T &a_OutLength)
	{
		return intersectPlane<dplane>(a_Plane, a_OutLength);
	}

	template<typename SphereType>
    bool intersectSphere(SphereType a_Sphere, T &a_OutLength)
	{
		T l_Sx = m_Origin.x - a_Sphere.m_Center.x;
		T l_Sy = m_Origin.y - a_Sphere.m_Center.y;
		T l_Sz = m_Origin.z - a_Sphere.m_Center.z;
		T l_A = m_Direction.x * m_Direction.x + m_Direction.y * m_Direction.y + m_Direction.z * m_Direction.z;
		T l_B = 2.0 * (l_Sx * m_Direction.x + l_Sy * m_Direction.y + l_Sz * m_Direction.z);
		T l_C = l_Sx * l_Sx + l_Sy * l_Sy + l_Sz * l_Sz - a_Sphere.m_Range * a_Sphere.m_Range;

		T l_B2_4AC = l_B * l_B - 4.0 * l_A * l_C;
		if( l_B2_4AC < 0 ) return false;

		T l_SqrtB24AC = sqrt(l_B2_4AC);
		T l_Length[2] = {(-l_B + l_SqrtB24AC) / (l_A * 2.0), (-l_B - l_SqrtB24AC) / (l_A * 2.0)};
		if( l_Length[0] < 0.0 || l_Length[1] < 0.0 ) a_OutLength = std::max(l_Length[0], l_Length[1]);
		else a_OutLength = std::min(l_Length[0], l_Length[1]);

		return true;
	}

	bool intersect(sphere a_Sphere, T &a_OutLength)
	{
		return intersectSphere<sphere>(a_Sphere, a_OutLength);
	}
	
	bool intersect(dsphere a_Sphere, T &a_OutLength)
	{
		return intersectSphere<dsphere>(a_Sphere, a_OutLength);
	}

	template<typename AABBType>
    bool intersectAABB(AABBType a_AABB, T &a_OutLength)
	{
		tplane<T> l_Plans[6] = {Plane(1.0, 0.0, 0.0, -a_AABB.getMaxX()),
							Plane(1.0, 0.0, 0.0, -a_AABB.getMinX()),
							Plane(0.0, 1.0, 0.0, -a_AABB.getMaxY()),
							Plane(0.0, 1.0, 0.0, -a_AABB.getMinY()),
							Plane(0.0, 0.0, 1.0, -a_AABB.getMaxZ()),
							Plane(0.0, 0.0, 1.0, -a_AABB.getMinZ())};
		tvec3<T> l_Min(a_AABB.getMin()), l_Max(a_AABB.getMax());

		a_OutLength = std::numeric_limits<T>::max();
		bool l_bIntersect = false;
		for( unsigned int i=0 ; i<6 ; ++i )
		{
			T l_Length = 0.0;
			if( intersect(l_Plans[i], l_Length) )
			{
				tvec3<T> l_Pt = m_Origin + l_Length * m_Direction;
				if( l_Pt.x >= l_Min.x && l_Pt.y >= l_Min.y && l_Pt.z >= l_Min.z &&
					l_Pt.x <= l_Max.x && l_Pt.y <= l_Max.y && l_Pt.z <= l_Max.z )
				{
					l_bIntersect = true;
					if( l_Length < a_OutLength ) a_OutLength = l_Length;
				}
			}
		}

		return l_bIntersect;
	}

	bool intersect(aabb a_AABB, T &a_OutLength)
	{
		return intersectAABB<aabb>(a_AABB, a_OutLength);
	}
	
	bool intersect(daabb a_AABB, T &a_OutLength)
	{
		return intersectAABB<daabb>(a_AABB, a_OutLength);
	}

	template<typename OBBType>
    bool intersectOBB(OBBType a_OBB, T &a_OutLength)
	{
		tmat4x4<T> l_Inverse(inverse(a_OBB.m_Transition));

		tvec4<T> l_NewOrigin(l_Inverse * tvec4<T>(m_Origin.x, m_Origin.y, m_Origin.z, 1.0));
		tvec4<T> l_NewDir(l_Inverse * tvec4<T>(m_Direction, 0.0));
		tray<T> l_Temp(l_NewOrigin.x, l_NewOrigin.y, l_NewOrigin.z, l_NewDir.x, l_NewDir.y, l_NewDir.z);
		return l_Temp.intersect(taabb<T>(tvec3<T>(0.0, 0.0, 0.0), a_OBB.m_Size), a_OutLength);
	}

	bool intersect(obb a_OBB, T &a_OutLength)
	{
		return intersectOBB<obb>(a_OBB, a_OutLength);
	}

	bool intersect(dobb a_OBB, T &a_OutLength)
	{
		return intersectOBB<dobb>(a_OBB, a_OutLength);
	}

	template<typename SphereType>
    T distance(SphereType a_Sphere, T &a_Radian, T &a_Z)
	{
		tvec3<T> l_Dir1(m_Direction);
		tvec3<T> l_Dir2(a_Sphere.m_Center - m_Origin);
		a_Radian = acos(dot(normalize(l_Dir1), normalize(l_Dir2)));
		T l_Length = length(l_Dir2);
		a_Z = l_Length * cos(a_Radian);
		return l_Length * sin(a_Radian);
	}

    T distance(sphere a_Sphere, T &a_Radian, T &a_Z)
	{
		return distance<sphere>(a_Sphere, a_Radian, a_Z);
	}
	
    T distance(dsphere a_Sphere, T &a_Radian, T &a_Z)
	{
		return distance<dsphere>(a_Sphere, a_Radian, a_Z);
	}

    tvec3<T> m_Origin;
    tvec3<T> m_Direction;
};
#pragma endregion

#pragma region plane
//
// plane
//
template<typename T>
struct tplane
{
    tplane()
		: m_Plane(1.0, 0.0, 0.0, -1.0)
	{
	}

    tplane(tvec4<T> &a_Plane)
		: m_Plane(a_Plane)
	{
	}

    tplane(T a_A, T a_B, T a_C, T a_D)
		: m_Plane(a_A, a_B, a_C, a_D)
	{
		assert(0.0f != a_A && 0.0f != a_B && 0.0f != a_C);
	}

    tplane(tvec3<T> &a_Point, tvec3<T> &a_Normal)
	{
		assert(0.0f != a_Normal.x && 0.0f != a_Normal.y && 0.0f != a_Normal.z);

		m_Plane.x = a_Normal.x;
		m_Plane.y = a_Normal.y;
		m_Plane.z = a_Normal.z;
		m_Plane.w = -a_Normal.x * a_Point.x - a_Normal.y * a_Point.y - a_Normal.z * a_Point.z;
	}

    ~tplane()
	{
	}

	T distance(tvec3<T> a_Pt)
	{
		tvec3<T> l_Normal(m_Plane.x, m_Plane.y, m_Plane.z);
		return (dot(l_Normal, a_Pt) + m_Plane.w) / length(l_Normal);
	}

	bool side(tvec3<T> a_Pt)
	{
		return m_Plane.x * a_Pt.x + m_Plane.y * a_Pt.y + m_Plane.z * a_Pt.z + m_Plane.w >= 0.0;
	}

    tvec4<T> m_Plane;//ax + by + cz + d = 0
};
#pragma endregion

#pragma region sphere
//
// sphere
//
template<typename T>
struct tsphere
{
    tsphere()
		: m_Center(zero< tvec3<T> >())
		, m_Range(1.0)
	{
	}

    tsphere(tvec3<T> &a_Center, T a_Range)
		: m_Center(a_Center)
		, m_Range(a_Range)
	{
	}
    
	tsphere(T a_CenterX, T a_CenterY, T a_CenterZ, T a_Range)
		: m_Center(a_CenterX, a_CenterY, a_CenterZ)
		, m_Range(a_Range)
	{
		assert(m_Range > 0.0);
	}

    tsphere(taabb<T> &a_AABB)
		: m_Center(a_AABB.m_Center)
		, m_Range(std::max(a_AABB.m_Size.z, std::max(a_AABB.m_Size.x, a_AABB.m_Size.y)) * 0.5)
	{
		assert(m_Range > 0.0);
	}

    tsphere(tobb<T> &a_OBB)
		: m_Range(std::max(a_OBB.m_Size.z, std::max(a_OBB.m_Size.x, a_OBB.m_Size.y)) * 0.5)
		, m_Center(a_OBB.m_Transition[0].w, a_OBB.m_Transition[1].w, a_OBB.m_Transition[2].w)
	{
		assert(m_Range > 0.0);
	}

    ~tsphere()
	{
	}

    tvec3<T> m_Center;
    T m_Range;//(x - a) ^ 2 + (y - b) ^ 2 + (z - c) ^ 2 = r ^ 2
};
#pragma endregion

#pragma region aabb
//
// AABB
//
template<typename T>
struct taabb
{
    taabb()
		: m_Center( zero<tvec3<T> >())
		, m_Size(1.0, 1.0, 1.0)
	{
	}

    taabb(tvec3<T> a_Center, tvec3<T> a_Size)
		: m_Center(a_Center)
		, m_Size(a_Size)
	{
	}
    
	taabb(tobb<T> &a_Box)
		: m_Center(zero< tvec3<T> >())
		, m_Size(a_Box.m_Size)
	{
		translate(a_Box.m_Transition);
	}

    ~taabb()
	{
	}

    T getMinX()
	{
		return m_Center.x - 0.5 * m_Size.x;
	}
    
	T getMinY()
	{
		return m_Center.y - 0.5 * m_Size.y;
	}
    
	T getMinZ()
	{
		return m_Center.z - 0.5 * m_Size.z;
	}
    
	tvec3<T> getMin()
	{
		return tvec3<T>(getMinX(), getMinY(), getMinZ());
	}

    T getMaxX()
	{
		return m_Center.x + 0.5 * m_Size.x;
	}
    
	T getMaxY()
	{
		return m_Center.y + 0.5 * m_Size.y;
	}
    
	T getMaxZ()
	{
		return m_Center.z + 0.5 * m_Size.z;
	}
    
	tvec3<T> getMax()
	{
		return tvec3<T>(getMaxX(), getMaxY(), getMaxZ());
	}

    void translate(tmat4x4<T> &a_World)
	{
		tvec4<T> l_NewCenter = a_World * vec4(m_Center.x, m_Center.y, m_Center.z, 1.0);
		m_Size = tvec3<T>(abs(a_World[0].x) * m_Size.x + abs(a_World[0].y) * m_Size.y + abs(a_World[0].z) * m_Size.z
						, abs(a_World[1].x) * m_Size.x + abs(a_World[1].y) * m_Size.y + abs(a_World[1].z) * m_Size.z
						, abs(a_World[2].x) * m_Size.x + abs(a_World[2].y) * m_Size.y + abs(a_World[2].z) * m_Size.z);
		m_Center = tvec3<T>(l_NewCenter.x, l_NewCenter.y, l_NewCenter.z);
	}

    bool intersect(taabb<T> &a_Box)
	{
		for( unsigned int i=0 ; i<3 ; ++i )
		{
			if( std::abs(m_Center[i] - a_Box.m_Center[i]) > (m_Size[i] + a_Box.m_Size[i]) * 0.5 ) return false;
		}
		return true;
	}

	template<typename T2>
	bool intersect(tsphere<T2> &a_Sphere)
	{
		tvec3<T> l_Nearest(glm::max(getMin(), glm::min(a_Sphere.m_Center, getMax())));
		return glm::distance(l_Nearest, a_Sphere) < a_Sphere.m_Range;
	}

	template<typename T2>
	bool intersect(tvec3<T2> &a_Pos1, tvec3<T2> &a_Pos2, tvec3<T2> &a_Pos3)
	{
		{
			tvec3<T> l_BoxMin(l_Box.getMin());
			tvec3<T> l_BoxMax(l_Box.getMax());
			tvec3<T2> l_TriangleMax(glm::max(glm::max(a_Pos1, a_Pos2), a_Pos3));
			tvec3<T2> l_TriangleMin(glm::min(glm::min(a_Pos1, a_Pos2), a_Pos3));
			for( unsigned int i=0 ; i<3 ; ++i )
			{
				if( l_TriangleMin[i] > l_BoxMax[i] || l_TriangleMax[i] < l_BoxMin[i] ) return false;
			}
		}

		tvec3<T> l_BoxVtx[8] = {
			tvec3<T>(m_Center + m_Size * tvec3<T>(-0.5, -0.5, -0.5)),
			tvec3<T>(m_Center + m_Size * tvec3<T>(-0.5, -0.5, 0.5)),
			tvec3<T>(m_Center + m_Size * tvec3<T>(-0.5, 0.5, -0.5)),
			tvec3<T>(m_Center + m_Size * tvec3<T>(-0.5, 0.5, 0.5)),
			tvec3<T>(m_Center + m_Size * tvec3<T>(0.5, -0.5, -0.5)),
			tvec3<T>(m_Center + m_Size * tvec3<T>(0.5, -0.5, 0.5)),
			tvec3<T>(m_Center + m_Size * tvec3<T>(0.5, 0.5, -0.5)),
			tvec3<T>(m_Center + m_Size * tvec3<T>(0.5, 0.5, 0.5))};
		tvec3<T2> l_TriangleEdge[3] = {a_Pos2 - a_Pos1, a_Pos3 - a_Pos2, a_Pos1 - a_Pos3};
		{
			glm::tvec3<T2> l_TriangleNormal(glm::normalize(glm::cross(l_TriangleEdge[0], -l_TriangleEdge[2])));
			T l_TrianglePos = glm::dot(l_TriangleNormal, a_Pos1);
			glm::tvec2<T> l_MinMax(std::numeric_limits<T>(), -std::numeric_limits<T>());
			for( unsigned int i=0 ; i<8 ; ++i )
			{
				T l_Project = glm::dot(l_TriangleNormal, l_BoxVtx[i]);
				l_MinMax.x = std::min(l_Project, l_MinMax.x);
				l_MinMax.y = std::max(l_Project, l_MinMax.y);
			}
			if( l_MinMax.x > l_TrianglePos || l_MinMax.y < l_TrianglePos ) return false;
		}
		
		{
			tvec3<T2> l_TrianglePos[3] = {a_Pos1, a_Pos2, a_Pos3};
			tvec3<T> l_Axis;
			for( unsigned int axis=0 ; axis<3 ; ++axis )
			{
				l_Axis = glm::zero<T>();
				l_Axis[axis] = 1.0;
				for( unsigned int i=0 ; i<3 ; ++i )
				{
					glm::tvec3<T> l_Vec(glm::cross(l_TriangleEdge[i], l_Axis));
					glm::tvec2<T> l_TriangleMinMax(std::numeric_limits<T>(), -std::numeric_limits<T>());
					glm::tvec2<T> l_BoxMinMax(std::numeric_limits<T>(), -std::numeric_limits<T>());
					for( unsigned int j=0 ; j<8 ; ++j )
					{
						T l_Project = glm::dot(l_BoxVtx[j], l_Vec);
						l_BoxMinMax.x = std::min(l_Project, l_BoxMinMax.x);
						l_BoxMinMax.y = std::max(l_Project, l_BoxMinMax.y);
					}

					for( unsigned int j=0 ; j<3 ; ++j )
					{
						T l_Project = glm::dot(l_TrianglePos[j], l_Vec);
						l_TriangleMinMax.x = std::min(l_Project, l_TriangleMinMax.x);
						l_TriangleMinMax.y = std::max(l_Project, l_TriangleMinMax.y);
					}

					if( l_TriangleMinMax.x > l_BoxMinMax.y || l_TriangleMinMax.y < l_BoxMinMax.x ) return false;
				}
			}
		}

		return true;
	}

    bool inRange(taabb<T> &a_Box)
	{
		for( unsigned int i=0 ; i<3 ; ++i )
		{
			if( 0.5 * (m_Size[i] - a_Box.m_Size[i]) < std::abs(m_Center[i] - a_Box.m_Center[i]) ) return false;
		}
		return true;
	}

    tvec3<T> m_Center;
    tvec3<T> m_Size;
};
#pragma endregion

#pragma region obb
//
// obb
//
template<typename T>
struct tobb
{
	tobb()
	{
	}

	tobb(tvec3<T> &a_Size, tmat4x4<T> &a_Transition)
		: m_Size(a_Size)
		, m_Transition(a_Transition)
	{
	}

	~tobb()
	{
	}

    tvec3<T> m_Size;
    tmat4x4<T> m_Transition;
};
#pragma endregion

#pragma region frustumface
//
// tfrustumface
//
template<typename T>
struct tfrustumface
{
	enum
	{
		FRUSTUM_LEFT = 0,
		FRUSTUM_RIGHT,
		FRUSTUM_TOP,
		FRUSTUM_BOTTOM,
		FRUSTUM_NEAR,
		FRUSTUM_FAR,

		FRUSTUM_NUM_PLANE
	};

	tfrustumface()
	{
		fromAABB(taabb<T>());
	}

	tfrustumface(tmat4x4<T> &a_ViewProjection)
	{
		fromViewProjection(a_ViewProjection);
	}
	tfrustumface(taabb<T> &a_Box)
	{
		fromAABB(a_Box);
	}

	~tfrustumface()
	{
	}

	void fromViewProjection(tmat4x4<T> &a_ViewProjection)
	{
		/* Frustum Culling in OpenGL
		By Mark Morley
		December 2000*/
	
		glm::vec4 l_Columns[4] = {glm::vec4(a_ViewProjection[0][0], a_ViewProjection[1][0], a_ViewProjection[2][0], a_ViewProjection[3][0])
								, glm::vec4(a_ViewProjection[0][1], a_ViewProjection[1][1], a_ViewProjection[2][1], a_ViewProjection[3][1])
								, glm::vec4(a_ViewProjection[0][2], a_ViewProjection[1][2], a_ViewProjection[2][2], a_ViewProjection[3][2])
								, glm::vec4(a_ViewProjection[0][3], a_ViewProjection[1][3], a_ViewProjection[2][3], a_ViewProjection[3][3])};

		/* Extract the numbers for the LEFT plane */
		m_Frustum[FRUSTUM_LEFT].m_Plane = l_Columns[3] + l_Columns[0];
		m_Frustum[FRUSTUM_LEFT].m_Plane /= glm::length(glm::vec3(m_Frustum[FRUSTUM_LEFT].m_Plane.x, m_Frustum[FRUSTUM_LEFT].m_Plane.y, m_Frustum[FRUSTUM_LEFT].m_Plane.z));

		/* Extract the numbers for the RIGHT plane */
		m_Frustum[FRUSTUM_RIGHT].m_Plane = l_Columns[3] - l_Columns[0];
		m_Frustum[FRUSTUM_RIGHT].m_Plane /= glm::length(glm::vec3(m_Frustum[FRUSTUM_RIGHT].m_Plane.x, m_Frustum[FRUSTUM_RIGHT].m_Plane.y, m_Frustum[FRUSTUM_RIGHT].m_Plane.z));
	
		/* Extract the TOP plane */
		m_Frustum[FRUSTUM_TOP].m_Plane = l_Columns[3] - l_Columns[1];
		m_Frustum[FRUSTUM_TOP].m_Plane /= glm::length(glm::vec3(m_Frustum[FRUSTUM_TOP].m_Plane.x, m_Frustum[FRUSTUM_TOP].m_Plane.y, m_Frustum[FRUSTUM_TOP].m_Plane.z));

		/* Extract the BOTTOM plane */
		m_Frustum[FRUSTUM_BOTTOM].m_Plane = l_Columns[3] + l_Columns[1];
		m_Frustum[FRUSTUM_BOTTOM].m_Plane /= glm::length(glm::vec3(m_Frustum[FRUSTUM_BOTTOM].m_Plane.x, m_Frustum[FRUSTUM_BOTTOM].m_Plane.y, m_Frustum[FRUSTUM_BOTTOM].m_Plane.z));

		/* Extract the NEAR plane */
		m_Frustum[FRUSTUM_NEAR].m_Plane = l_Columns[3] + l_Columns[2];
		m_Frustum[FRUSTUM_NEAR].m_Plane /= glm::length(glm::vec3(m_Frustum[FRUSTUM_NEAR].m_Plane.x, m_Frustum[FRUSTUM_NEAR].m_Plane.y, m_Frustum[FRUSTUM_NEAR].m_Plane.z));

		/* Extract the FAR plane */
		m_Frustum[FRUSTUM_FAR].m_Plane = l_Columns[3] - l_Columns[2];
		m_Frustum[FRUSTUM_FAR].m_Plane /= glm::length(glm::vec3(m_Frustum[FRUSTUM_FAR].m_Plane.x, m_Frustum[FRUSTUM_FAR].m_Plane.y, m_Frustum[FRUSTUM_FAR].m_Plane.z));
	}

	void fromAABB(taabb<T> &a_Box)
	{
		tvec3<T> l_Extend(a_Box.m_Size.x * 0.5, a_Box.m_Size.y * 0.5, a_Box.m_Size.z * 0.5);
		m_Frustum[FRUSTUM_LEFT].m_Plane  = glm::vec4(1.0f, 0.0f, 0.0f, a_Box.m_Center.x - l_Extend.x);
		m_Frustum[FRUSTUM_RIGHT].m_Plane = glm::vec4(-1.0f, 0.0f, 0.0f, a_Box.m_Center.x + l_Extend.x);
		m_Frustum[FRUSTUM_TOP].m_Plane   = glm::vec4(0.0f, -1.0f, 0.0f, a_Box.m_Center.y + l_Extend.y);
		m_Frustum[FRUSTUM_BOTTOM].m_Plane= glm::vec4(0.0f, 1.0f, 0.0f, a_Box.m_Center.y - l_Extend.y);
		m_Frustum[FRUSTUM_NEAR].m_Plane  = glm::vec4(0.0f, 0.0f, 1.0f, a_Box.m_Center.z - l_Extend.z);
		m_Frustum[FRUSTUM_FAR].m_Plane   = glm::vec4(0.0f, 0.0f, -1.0f, a_Box.m_Center.z + l_Extend.z);
	}

	template<typename AABBType>
	bool boxCheck(AABBType &a_Box, bool &a_bInside)
	{
		a_bInside = true;

		tvec3<T> l_Min(a_Box.getMin()), l_Max(a_Box.getMax());
		tvec3<T> l_BoxPt[8] = {
			tvec3<T>(l_Min.x, l_Min.y, l_Min.z),
			tvec3<T>(l_Min.x, l_Min.y, l_Max.z),
			tvec3<T>(l_Min.x, l_Max.y, l_Min.z),
			tvec3<T>(l_Min.x, l_Max.y, l_Max.z),
			tvec3<T>(l_Max.x, l_Min.y, l_Min.z),
			tvec3<T>(l_Max.x, l_Min.y, l_Max.z),
			tvec3<T>(l_Max.x, l_Max.y, l_Min.z),
			tvec3<T>(l_Max.x, l_Max.y, l_Max.z)};
		for( unsigned int i=0 ; i<FRUSTUM_NUM_PLANE ; ++i )
		{
			int l_OutCount = 0;
			for( unsigned int j=0 ; j<8 ; ++j )
			{
				if( !m_Frustum[i].side(l_BoxPt[j]) ) ++l_OutCount;
			}

			if( 8 == l_OutCount ) return false;
			else if( 0 != l_OutCount ) a_bInside = false;
		}

		return true;
	}

	bool intersect(aabb &a_Box, bool &a_bInside)
	{
		return boxCheck<aabb>(a_Box, a_bInside);
	}

	bool intersect(daabb &a_Box, bool &a_bInside)
	{
		return boxCheck<daabb>(a_Box, a_bInside);
	}
	
	tplane<T> m_Frustum[FRUSTUM_NUM_PLANE];
};
#pragma endregion

#pragma region viewport
//
// viewport(alias of D3D12_VIEWPORT)
// 
template<typename T>
struct tviewport
{
	tviewport()
		: m_Start(0.0, 0.0)
		, m_Size(0.0, 0.0)
		, m_MinMaxDepth(0.0, 0.0)
	{	
	}

	tviewport(T a_StartX, T a_StartY, T a_Width, T a_Height, T a_MinDepth, T a_MaxDepth)
		: m_Start(a_StartX, a_StartY)
		, m_Size(a_Width, a_Height)
		, m_MinMaxDepth(a_MinDepth, a_MaxDepth)
	{
	}

	tviewport(tvec2<T> a_Start, tvec2<T> a_Size, tvec2<T> a_MinMaxDepth)
		: m_Start(a_Start)
		, m_Size(a_Size)
		, m_MinMaxDepth(a_MinMaxDepth)
	{
	}

	~tviewport()
	{	
	}

	tvec2<T> m_Start;
    tvec2<T> m_Size;
    tvec2<T> m_MinMaxDepth;
};
#pragma endregion

}

#endif