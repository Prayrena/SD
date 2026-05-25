#pragma once
#include "Engine/Math/Vec3.hpp"

struct Plane3 
{
public:
	Plane3() {}
	~Plane3() {}
	explicit Plane3(Vec3 const& normal, float dist);

	Vec3 m_normal = Vec3(0.f, 0.f, 1.f);

	float m_distAlongNormalFromOrigin = 0.f;

	float GetAltitudeOfPoint(Vec3 const& refPoint) const;
	Vec3 GetProjectedVector(Vec3 const& inputVec) const;

	bool IfThePointIsInFrontOfPlane(Vec3 const& pt) const;
	bool IfThePointIsOnThePlane(Vec3 const& pt) const;
};