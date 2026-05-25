#pragma once
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Math/OBB3.hpp"
#include "Engine/Math/Plane3.hpp"
#include "Engine/Math/Convex2.hpp"


struct Sphere;
struct ZCylinder;

struct Raycast2D
{
	//ray cast information
	Vec2	rayStartPos = Vec2::ZERO;
	Vec2	rayFwdNormal = Vec2::ZERO;
	float	rayDist = 1.f;
};

struct RaycastResult2D
{
public:
	// Basic ray cast result information
	bool	m_didImpact = false;
	float	m_impactDist = 0.f;
	Vec2	m_impactPos = Vec2::ZERO;
	Vec2	m_impactNormal = Vec2::ZERO;

	// if the raycast did impact, we'll also save the exit point info
	bool	m_didExit = false;
	float   m_travelDistInShape = 0.f;
	Vec2	m_exitPos = Vec2::ZERO;
	Vec2	m_exitNormal = Vec2::ZERO;

	// Original ray cast information
	Vec2	m_rayFwdNormal = Vec2::ZERO;
	Vec2	m_rayStartPos = Vec2::ZERO;
	float	m_rayDist = 0.f;
};

struct Raycast3D
{
	Raycast3D(Vec3 start, Vec3 normal, float dist)
		: rayStart(start)
		, rayFwdNormal(normal)
		, rayDist(dist)
	{}
	~Raycast3D() {}

	//ray cast information
	Vec3	rayStart = Vec3::ZERO;
	Vec3	rayFwdNormal = Vec3::ZERO;
	float	rayDist = 1.f;
};

struct RaycastResult3D
{
	// Basic ray cast result information
	bool	m_didImpact = false;
	float	m_impactDist = 0.f;
	Vec3	m_impactPos = Vec3::ZERO;
	Vec3	m_impactNormal = Vec3::ZERO;

	// if the raycast did impact, we'll also save the exit point info
	bool	m_didExit = false;
	float   m_travelDistInShape = 0.f;
	Vec3	m_exitPos;
	Vec3	m_exitNormal = Vec3::ZERO;

	// Original ray cast information
	Vec3	m_rayFwdNormal = Vec3::ZERO;
	Vec3	m_rayStartPos = Vec3::ZERO;
	float	m_rayDist = 1.f;
};

struct Disc2
{
	Disc2(float radius, Vec2 center)
		: m_radius(radius)
		, m_center(center)
	{}

	float	m_radius = 1.f;
	Vec2	m_center = Vec2::ONE;
};

// raycast conversion: get a raycastRe
Raycast2D ConvertRaycast3DToRaycast2DOnXY(Raycast3D ray);

// 2D raycast 
RaycastResult2D RaycastVsDisc2D(Raycast2D ray, Disc2 disc);
RaycastResult2D RaycastVsDisc2D(Raycast2D ray, Vec2 discCenter, float discRadius);
RaycastResult2D RaycastVsDisc2D(Vec2 RayStartPoint, Vec2 fwdNormal, float RayDist, Vec2 discCenter, float discRadius);
RaycastResult2D RaycastVSLineSegment2D(Vec2 RayStart, Vec2 rayForwardNormal, float rayDist, Vec2 lineSegStart, Vec2 lineSegEnd);
RaycastResult2D RaycastVSAABB2(Vec2 RayStart, Vec2 rayForwardNormal, float rayDist, AABB2 targetBox);
RaycastResult2D RaycastVSAABB2(Raycast2D ray, AABB2 targetBox);
RaycastResult2D RaycastVSConvexPoly2(Raycast2D ray, ConvexPoly2 convex);
RaycastResult2D RaycastVSConvexHull2(Raycast2D ray, ConvexHull2 convex);
RaycastResult2D RaycastVSPlane2(Raycast2D ray, Plane2 plane);

// 3D raycast
// Raycast vs 3D shapes
RaycastResult3D RaycastVsAABB3D(Vec3 rayStart, Vec3 rayForwardNormal, float rayDist, AABB3 targetbox);
RaycastResult3D RaycastVsOBB3(Vec3 rayStart, Vec3 rayForwardNormal, float rayDist, OBB3 targetbox);
RaycastResult3D RaycastVsPlane3D(Vec3 rayStart, Vec3 rayForwardNormal, float rayDist, Plane3 plane);
RaycastResult3D RaycastVsSphere3D(Vec3 rayStart, Vec3 rayForwardNormal, float rayDist, Sphere sphere);
RaycastResult3D RaycastVsCylinderZ3D(Vec3 rayStart, Vec3 rayForwardNormal, float rayDist, ZCylinder cylinder);