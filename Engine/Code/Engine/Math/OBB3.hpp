#pragma once
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/Mat44.hpp"

struct OBB3
{
public:
	Vec3 m_center;// the center pos of the OBB2 in world space
	Vec3 m_halfDimensions;//.x means the width, .y means the height
	Vec3 m_iBasis;
	Vec3 m_jBasis;
	Vec3 m_kBasis;

public:
	OBB3() {}
	~OBB3() {}
	OBB3(OBB3 const& copyfrom);
	explicit OBB3(Vec3 const& centerPos, Vec3 const& iBasisNormal, Vec3 const& jBasisNormal, Vec3 const& halfDimensions_Width_depth_Height);

	Vec3 const	GetCenter() const;
	Vec3 const	GetDimensions() const;

	void GetCornerPoints(Vec3* out_fourCornerWorldPositions) const;

	Mat44 GetModelMatrix() const;
	Vec3 GetLocalPosForWorldPos(Vec3 worldPos) const;
	Vec3 GetWorldPosForLocalPos(Vec3 localPos) const;
	// void RotateAboutCenter(float rotationDeltaDegrees);
};
