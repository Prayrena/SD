#include "Engine/Math/OBB3.hpp"
#include "Engine/Math/MathUtils.hpp"

OBB3::OBB3(OBB3 const& copyfrom)
	: m_center(copyfrom.m_center)
	, m_iBasis(copyfrom.m_iBasis)
	, m_jBasis(copyfrom.m_jBasis)
	, m_kBasis(copyfrom.m_kBasis)
	, m_halfDimensions(copyfrom.m_halfDimensions)
{

}	 

OBB3::OBB3(Vec3 const& centerPos, Vec3 const& iBasisNormal, Vec3 const& jBasisNormal, Vec3 const& halfDimensions_Width_depth_Height)
	: m_center(centerPos)
	, m_iBasis(iBasisNormal)
	, m_jBasis(jBasisNormal)
	, m_halfDimensions(halfDimensions_Width_depth_Height)
{
	m_kBasis = CrossProduct3D(m_iBasis, m_jBasis);
}

Vec3 const OBB3::GetCenter() const
{
	return m_center;
}

Vec3 const OBB3::GetDimensions() const
{
	return m_halfDimensions * 2;
}

// return eight vec3
void OBB3::GetCornerPoints(Vec3* out_fourCornerWorldPositions) const
{
	out_fourCornerWorldPositions[0] = m_center - m_iBasis * m_halfDimensions.x - m_jBasis * m_halfDimensions.y - m_kBasis * m_halfDimensions.z;// BBL
	out_fourCornerWorldPositions[1] = m_center + m_iBasis * m_halfDimensions.x - m_jBasis * m_halfDimensions.y - m_kBasis * m_halfDimensions.z;// BBR
	out_fourCornerWorldPositions[2] = m_center - m_iBasis * m_halfDimensions.x + m_jBasis * m_halfDimensions.y - m_kBasis * m_halfDimensions.z;// BTL
	out_fourCornerWorldPositions[3] = m_center + m_iBasis * m_halfDimensions.x + m_jBasis * m_halfDimensions.y - m_kBasis * m_halfDimensions.z;// BTR

	out_fourCornerWorldPositions[4] = m_center - m_iBasis * m_halfDimensions.x - m_jBasis * m_halfDimensions.y + m_kBasis * m_halfDimensions.z;// TBL
	out_fourCornerWorldPositions[5] = m_center + m_iBasis * m_halfDimensions.x - m_jBasis * m_halfDimensions.y + m_kBasis * m_halfDimensions.z;// TBR
	out_fourCornerWorldPositions[6] = m_center - m_iBasis * m_halfDimensions.x + m_jBasis * m_halfDimensions.y + m_kBasis * m_halfDimensions.z;// TTL
	out_fourCornerWorldPositions[7] = m_center + m_iBasis * m_halfDimensions.x + m_jBasis * m_halfDimensions.y + m_kBasis * m_halfDimensions.z;// TTR
}

Mat44 OBB3::GetModelMatrix() const
{
	Mat44 transformMat(m_iBasis, m_jBasis, m_kBasis, m_center);

	return transformMat;
}

Vec3 OBB3::GetLocalPosForWorldPos(Vec3 worldPos) const
{
	Vec3 dispCP = worldPos - m_center;
	Mat44 worldToLocal = GetModelMatrix().GetOrthonormalInverse();
	Vec3 localSpacePos = worldToLocal.TransformVectorQuantity3D(dispCP);

	return localSpacePos;
}

Vec3 OBB3::GetWorldPosForLocalPos(Vec3 localPos) const
{
	Vec3 worldPos = GetModelMatrix().TransformPosition3D(localPos);
	return worldPos;
}

//void OBB2::RotateAboutCenter(float rotationDeltaDegrees)
//{
//	m_iBasisNormal.RotateDegrees(rotationDeltaDegrees);
//}

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

