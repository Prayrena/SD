#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/MathUtils.hpp"

AABB2 const AABB2::ZERO_TO_ONE = AABB2(Vec2(0.f, 0.f), Vec2(1.f, 1.f));

AABB2::AABB2(AABB2 const& copyfrom)
{
	m_mins = copyfrom.m_mins;
	m_maxs = copyfrom.m_maxs;
}

AABB2::AABB2(float minX, float minY, float maxX, float maxY)
{
	m_mins = Vec2(minX, minY);
	m_maxs = Vec2(maxX, maxY);
}

AABB2::AABB2(Vec2 const& mins, Vec2 const& maxs)
{
	m_mins = mins;
	m_maxs = maxs;
}

bool AABB2::IsPointInside(Vec2 const& point) const
{
	if (point.x > m_mins.x && point.x < m_maxs.x)
	{
		if (point.y > m_mins.y && point.y < m_maxs.y)
		{
			return true;
		}
	}

	return false;

}

Vec2 const AABB2::GetCenter() const
{
	float newX = (m_mins.x + m_maxs.x) * .5f;
	float newY = (m_mins.y + m_maxs.y) * .5f;
	Vec2 center(newX, newY);

	return center;
}

Vec2 const AABB2::GetDimensions() const
{
	return Vec2(m_maxs.x - m_mins.x, m_maxs.y - m_mins.y);
}

Vec2 const AABB2::GetNearestPoint(Vec2 const& referencePosition) const
{
	float newX = GetClamped(referencePosition.x, m_mins.x, m_maxs.x);
	float newY = GetClamped(referencePosition.y, m_mins.y, m_maxs.y);

	Vec2 NearestPoint(newX, newY);

	return Vec2(newX, newY);
}

Vec2 const AABB2::GetPointAtUV(Vec2 const& uv) const
{
	float newX = RangeMap(uv.x, 0.f, 1.f, m_mins.x, m_maxs.x);
	float newY = RangeMap(uv.y, 0.f, 1.f, m_mins.y, m_maxs.y);

	Vec2 worldPoint(newX, newY);

	return worldPoint;
}

Vec2 const AABB2::GetUVForPoint(Vec2 const& point) const
{
	float newX = GetFractionWithinRange(point.x, m_mins.x, m_maxs.x);
	float newY = GetFractionWithinRange(point.y, m_mins.y, m_maxs.y);
	Vec2 UVPoint(newX, newY);
	return UVPoint;
}

AABB2 AABB2::GetBoxAtUVs(Vec2 uvMins, Vec2 uvMaxs) const
{
	Vec2 dimensions = GetDimensions();
	Vec2 BL;
	Vec2 TR;
	BL.x = m_mins.x + dimensions.x * uvMins.x;
	BL.y = m_mins.y + dimensions.y * uvMins.y;
	TR.x = m_mins.x + dimensions.x * uvMaxs.x;
	TR.y = m_mins.y + dimensions.y * uvMaxs.y;
	AABB2 result(BL, TR);
	return result;
}

void AABB2::Translate(Vec2 const& translationToApply)
{
	m_mins += translationToApply;
	m_maxs += translationToApply;
}

void AABB2::SetCenter(Vec2 const& newCenter)
{
	Vec2 currentCenter = Vec2((m_mins.x + m_maxs.x) * .5f, (m_mins.y + m_maxs.y) * .5f);
	Vec2 disp = newCenter - currentCenter;
	Translate(disp);
}

void AABB2::SetDimensions(Vec2 const& newDimensions)
{
	Vec2 center = Vec2((m_mins.x + m_maxs.x) * .5f, (m_mins.y + m_maxs.y) * .5f);
	m_mins = Vec2(center.x - (newDimensions.x * .5f), center.y - (newDimensions.y * .5f));
	m_maxs = Vec2(center.x + (newDimensions.x * .5f), center.y + (newDimensions.y * .5f));
}

void AABB2::SetScale(float const& scaleX, float const& scaleY)
{
	Vec2 center = Vec2((m_mins.x + m_maxs.x) * .5f, (m_mins.y + m_maxs.y) * .5f);
	Vec2 dimensions = GetDimensions();
	dimensions.x *= scaleX;
	dimensions.y *= scaleY;
	m_mins = Vec2(center.x - (dimensions.x * .5f), center.y - (dimensions.y * .5f));
	m_maxs = Vec2(center.x + (dimensions.x * .5f), center.y + (dimensions.y * .5f));
}

void AABB2::SetUniformScale(float const& scale)
{
	SetScale(scale, scale);
}

void AABB2::StretchToIncludePoint(Vec2 const& point)
{
	if (m_mins.x > point.x)
	{
		m_mins.x = point.x;
	}
	if (m_maxs.x < point.x)
	{
		m_maxs.x = point.x;
	}
	if (m_mins.y > point.y)
	{
		m_mins.y = point.y;
	}
	if (m_maxs.y < point.y)
	{
		m_maxs.y = point.y;
	}
}

void AABB2::AddPadding(float xToAddOnBothSides, float yToAddToTopAndBottom)
{
	Vec2 dimensions = GetDimensions();
	float xPadding = dimensions.x * xToAddOnBothSides;
	float yPadding = dimensions.y * yToAddToTopAndBottom;
	m_mins += Vec2(xPadding, yPadding);
	m_maxs -= Vec2(xPadding, yPadding);
}

AABB2 const AABB2::operator*(float uniformScale) const
{
	return AABB2(m_mins * uniformScale, m_maxs * uniformScale);
}

AABB2 const AABB2::operator+(Vec2 offsetDist) const
{
	return AABB2(m_mins + offsetDist, m_maxs + offsetDist);
}

bool AABB2::operator==(AABB2 comparison) const
{
	return (m_mins == comparison.m_mins && m_maxs == comparison.m_maxs);
}

bool AABB2::operator<(AABB2 const& compare) const 
{
	return (m_mins < compare.m_mins && m_maxs < compare.m_maxs);
}
