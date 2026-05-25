#include "Engine/Math/OBB2.hpp"
#include "Engine/Math/MathUtils.hpp"

OBB2::OBB2(OBB2 const& copyfrom)
{
	m_center = copyfrom.m_center;
	m_iBasisNormal = copyfrom.m_iBasisNormal;
	m_halfDimensions = copyfrom.m_halfDimensions;
}	 

OBB2::OBB2(Vec2 const& center, Vec2 const& iBasisNormal, Vec2 const& halfDimensions_Width_Height)
{
	m_center = center;
	m_iBasisNormal = iBasisNormal;
	m_halfDimensions = halfDimensions_Width_Height;
}

Vec2 const OBB2::GetCenter() const
{
	return m_center;
}

Vec2 const OBB2::GetDimensions() const
{
	return m_halfDimensions * 2;
}

// return four vector2
void OBB2::GetCornerPoints(Vec2* out_fourCornerWorldPositions) const
{
	Vec2 jBasisNormal = m_iBasisNormal.GetRotated90Degrees();
	// dereferencing?
	out_fourCornerWorldPositions[0] = m_center - m_iBasisNormal * m_halfDimensions.x - jBasisNormal * m_halfDimensions.y;// BL
	out_fourCornerWorldPositions[1] = m_center + m_iBasisNormal * m_halfDimensions.x - jBasisNormal * m_halfDimensions.y;// BR
	out_fourCornerWorldPositions[2] = m_center - m_iBasisNormal * m_halfDimensions.x + jBasisNormal * m_halfDimensions.y;// TL
	out_fourCornerWorldPositions[3] = m_center + m_iBasisNormal * m_halfDimensions.x + jBasisNormal * m_halfDimensions.y;// TR
}

Vec2 OBB2::GetLocalPosForWorldPos(Vec2 worldPos) const
{
	Vec2 jBasisNormal = m_iBasisNormal.GetRotated90Degrees();

	Vec2 dispCP = worldPos - m_center;
	Vec2 P_LocalSpace;
	P_LocalSpace.x = DotProduct2D(dispCP, m_iBasisNormal);
	P_LocalSpace.y = DotProduct2D(dispCP, jBasisNormal);
	return P_LocalSpace;
}

Vec2 OBB2::GetWorldPosForLocalPos(Vec2 localPos) const
{
	Vec2 jBasisNormal = m_iBasisNormal.GetRotated90Degrees();
	Vec3 worldPosIn3D = Vec3(localPos.x, localPos.y, 0.f);
	TransformPositionAroundPosOnXY(worldPosIn3D, m_iBasisNormal, jBasisNormal, m_center);
	Vec2 worldPosIn2D = Vec2(worldPosIn3D.x, worldPosIn3D.y);
	return worldPosIn2D;
}

void OBB2::RotateAboutCenter(float rotationDeltaDegrees)
{
	m_iBasisNormal.RotateDegrees(rotationDeltaDegrees);
}

//Vec2 const OBB2::GetPointAtUV(Vec2 const& uv) const
//{
//	float newX = RangeMap(uv.x, 0.f, 1.f, m_mins.x, m_maxs.x);
//	float newY = RangeMap(uv.y, 0.f, 1.f, m_mins.y, m_maxs.y);
//
//	Vec2 worldPoint(newX, newY);
//
//	return worldPoint;
//}
//
//Vec2 const OBB2::GetUVForPoint(Vec2 const& point) const
//{
//	float newX = GetFractionWithinRange(point.x, m_mins.x, m_maxs.x);
//	float newY = GetFractionWithinRange(point.y, m_mins.y, m_maxs.y);
//	Vec2 UVPoint(newX, newY);
//	return UVPoint;
//}
//
//void OBB2::Translate(Vec2 const& translationToApply)
//{
//	m_mins += translationToApply;
//	m_maxs += translationToApply;
//}
//
//void OBB2::SetCenter(Vec2 const& newCenter)
//{
//	Vec2 currentCenter = Vec2((m_mins.x + m_maxs.x) * .5f, (m_mins.y + m_maxs.y) * .5f);
//	Vec2 disp = newCenter - currentCenter;
//	Translate(disp);
//}
//
//void OBB2::SetDimensions(Vec2 const& newDimensions)
//{
//	Vec2 center = Vec2((m_mins.x + m_maxs.x) * .5f, (m_mins.y + m_maxs.y) * .5f);
//	m_mins = Vec2(center.x - (newDimensions.x * .5f), center.y - (newDimensions.y * .5f));
//	m_maxs = Vec2(center.x + (newDimensions.x * .5f), center.y + (newDimensions.y * .5f));
//}
//
//void OBB2::StretchToIncludePoint(Vec2 const& point)
//{
//	if (m_mins.x > point.x)
//	{
//		m_mins.x = point.x;
//	}
//	if (m_maxs.x < point.x)
//	{
//		m_maxs.x = point.x;
//	}
//	if (m_mins.y > point.y)
//	{
//		m_mins.y = point.y;
//	}
//	if (m_maxs.y < point.y)
//	{
//		m_maxs.y = point.y;
//	}
//}

