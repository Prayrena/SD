#pragma once
#include "Engine/Math/Vec2.hpp"
#include <vector>

struct Plane2
{
public:
	Plane2() {}
	~Plane2() {}
	explicit Plane2(Vec2 const& normal, float dist);

	float	GetAltitudeOfPoint(Vec2 const& p) const;
	bool	IsThePointInFrontOfPlane(Vec2 const& p) const;
	Vec2	GetNearestPointOnPlane(Vec2 const& point) const;

	Vec2 m_normal = Vec2(0.f, 1.f);
	float m_distAlongNormalFromOrigin = 0.f;
};

bool IsPointBehindAllPlane2(Vec2 const& point, std::vector<Plane2> const& planes);