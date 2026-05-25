#pragma once
#include "Engine/Math/Vec2.hpp"

struct Capsule2
{
public:
	Vec2  m_start;
	Vec2  m_end;
	float m_radius = 0.f;

public:
	Capsule2() {}
	~Capsule2() {}
	Capsule2(Capsule2 const& copyfrom);
	explicit Capsule2(Vec2 startPos, Vec2 endPos, float radius);

	void Translate(Vec2 translation);
	void SetCenter(Vec2 newCenter);
	void RotateAboutCenter(float rotationDeltaDegrees);
};