#include "Engine/Math/AABB3.hpp"

AABB3::AABB3(AABB3 const& copyfrom)
{
	m_mins = copyfrom.m_mins;
	m_maxs = copyfrom.m_maxs;
}

AABB3::AABB3(float minX, float minY, float minZ, float maxX, float maxY, float maxZ)
{
	m_mins = Vec3(minX, minY, minZ);
	m_maxs = Vec3(maxX, maxY, maxZ);
}

AABB3::AABB3(Vec3 const& mins, Vec3 const& maxs)
{
	m_mins = mins;
	m_maxs = maxs;
}

void AABB3::SetTranslation(Vec3 const& translation)
{
	m_mins += translation;
	m_maxs += translation;
}

void AABB3::GetAllEightPointsOfTheCorners(Vec3& BBL, Vec3& BBR, Vec3& BTR, Vec3& BTL, Vec3& FBL, Vec3& FBR, Vec3& FTR, Vec3& FTL) const
{
	Vec3 max = m_maxs;
	Vec3 min = m_mins;

	FBL = Vec3(max.x, min.y, min.z);
	FBR = Vec3(max.x, max.y, min.z);
	FTL = Vec3(max.x, min.y, max.z);
	FTR = Vec3(max.x, max.y, max.z);

	BBL = Vec3(min.x, min.y, min.z);
	BBR = Vec3(min.x, max.y, min.z);
	BTL = Vec3(min.x, min.y, max.z);
	BTR = Vec3(min.x, max.y, max.z);
}

void AABB3::GetCornerPoints(Vec3* out_eightCornerWorldPositions) const
{
	Vec3 max = m_maxs;
	Vec3 min = m_mins;

	out_eightCornerWorldPositions[0] = Vec3(max.x, min.y, min.z); // BBR
	out_eightCornerWorldPositions[1] = Vec3(max.x, max.y, min.z); // BTR
	out_eightCornerWorldPositions[2] = Vec3(max.x, min.y, max.z); // TBR
	out_eightCornerWorldPositions[3] = Vec3(max.x, max.y, max.z); // TTR

	out_eightCornerWorldPositions[4] = Vec3(min.x, min.y, min.z); // BBL
	out_eightCornerWorldPositions[5] = Vec3(min.x, max.y, min.z); // BTL
	out_eightCornerWorldPositions[6] = Vec3(min.x, min.y, max.z); // TBL
	out_eightCornerWorldPositions[7] = Vec3(min.x, max.y, max.z); // TTL
}

Vec3 AABB3::GetCenter() const
{
	return (m_mins + m_maxs) / 2.f;
}

void AABB3::operator *= (float const& uniformScale)
{
	Vec3 center = GetCenter();
	Vec3 dispToBBL = m_mins - center;
	Vec3 dispToFTR = m_maxs - center;

	dispToFTR *= uniformScale;
	dispToBBL *= uniformScale;

	m_mins = dispToBBL + center;
	m_maxs = dispToFTR + center;
}

AABB3 AABB3::operator * (float const& uniformScale) const
{
	AABB3 newCube;

	Vec3 center = GetCenter();
	Vec3 dispToBBL = m_mins - center;
	Vec3 dispToFTR = m_maxs - center;

	dispToFTR *= uniformScale;
	dispToBBL *= uniformScale;

	newCube.m_mins = dispToBBL + center;
	newCube.m_maxs = dispToFTR + center;
	return newCube;
}