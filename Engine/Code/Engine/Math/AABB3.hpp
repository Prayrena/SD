#pragma once
#include "Engine/Math/Vec3.hpp"

struct AABB3
{
public:
	Vec3 m_mins;
	Vec3 m_maxs;

	AABB3() {}
	~AABB3() {}
	AABB3(AABB3 const& copyfrom);

	explicit AABB3(float minX, float minY, float minZ, float maxX, float maxY, float maxZ);
	explicit AABB3(Vec3 const& mins, Vec3 const& maxs);

	void SetTranslation(Vec3 const& translation);
	void GetAllEightPointsOfTheCorners(Vec3& BBL, Vec3& BBR, Vec3& BTR, Vec3& BTL, 
										Vec3& FBL, Vec3& FBR, Vec3& FTR, Vec3& FTL) const;
	void GetCornerPoints(Vec3* out_eightCornerWorldPositions) const;


	Vec3 GetCenter() const;

	AABB3 operator * (float const& uniformScale) const;

	void operator *= (float const& uniformScale);
};