#pragma once
#include "Engine/Math/Vec2.hpp"

struct OBB2
{
public:
	Vec2 m_center;// the center pos of the OBB2 in world space
	Vec2 m_iBasisNormal;
	Vec2 m_halfDimensions;//.x means the width, .y means the height

public:
	OBB2() {}
	~OBB2() {}
	OBB2(OBB2 const& copyfrom);
	explicit OBB2(Vec2 const& center, Vec2 const& iBasisNormal, Vec2 const& halfDimensions_Width_Height);

	Vec2 const	GetCenter() const;
	Vec2 const	GetDimensions() const;
	//Vec2 const	GetPointAtUV(Vec2 const& uv) const;
	//Vec2 const	GetUVForPoint(Vec2 const& point) const;
	//
	//void		Translate(Vec2 const& translationToApply);
	//void		SetCenter(Vec2 const& newCenter);
	//void		SetDimensions(Vec2 const& newDimensions);
	//void		StretchToIncludePoint(Vec2 const& point);

	void GetCornerPoints(Vec2* out_fourCornerWorldPositions) const;
	Vec2 GetLocalPosForWorldPos(Vec2 worldPos) const;
	Vec2 GetWorldPosForLocalPos(Vec2 localPos) const;
	void RotateAboutCenter(float rotationDeltaDegrees);
};
