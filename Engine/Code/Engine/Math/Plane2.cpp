#include "Engine/Math/Plane2.hpp"
#include "Engine/Math/MathUtils.hpp"

Plane2::Plane2(Vec2 const& normal, float dist)
		: m_normal(normal)
		, m_distAlongNormalFromOrigin(dist)
{
	
}

// this will return negative when the point is in the back
float Plane2::GetAltitudeOfPoint(Vec2 const& p) const
{
	float projectedLen = DotProduct2D(p, m_normal);
	return (projectedLen - m_distAlongNormalFromOrigin);
}

bool Plane2::IsThePointInFrontOfPlane(Vec2 const& p) const
{
	// Vec2 nearestPt = GetNearestPointOnPlane(p);
	// float distOnNormal = DotProduct2D(p, m_normal);
	float height = GetAltitudeOfPoint(p);
	if (height > 0.f)
	{
		return true;
	}
	else // (distOnNormal <= m_distAlongNormalFromOrigin)
	{
		return false;
	}
}

Vec2 Plane2::GetNearestPointOnPlane(Vec2 const& point) const
{
	float refPosAltitude = GetAltitudeOfPoint(point);
	Vec2 disp = refPosAltitude * (-m_normal);
	return (point + disp);
}

bool IsPointBehindAllPlane2(Vec2 const& point, std::vector<Plane2> const& planes)
{
	for (Plane2 plane : planes)
	{
		if (!plane.IsThePointInFrontOfPlane(point))
		{
			continue;
		}
		else return false;
	}
	return true;
}
