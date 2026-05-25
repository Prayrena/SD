#pragma once
#include "Engine/Math/Vec3.hpp"

struct Capsule3
{
public:
	Vec3  m_start;
	Vec3  m_end;
	bool  m_isFixedRadius = true;
	float m_radius_start = 0.f;
	float m_radius_end = 0.f;

public:
	Capsule3() {}
	~Capsule3() {}
	Capsule3(Capsule3 const& copyfrom);
	explicit Capsule3(Vec3 startPos, Vec3 endPos, bool isFixedRadius, float startRadius, float endRadius);

	void Translate(Vec3 const& translation);
	void SetCenter(Vec3 const& newCenter);
	Capsule3 RotateFromStart(float yaw, float pitch);
};