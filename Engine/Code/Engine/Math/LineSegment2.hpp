#pragma once
#include "Engine/Math/Vec2.hpp"

struct LineSegment2
{
public:
	Vec2  m_start;
	Vec2  m_end;

public:
	LineSegment2() {}
	~LineSegment2() {}
	LineSegment2(LineSegment2 const& copyfrom);
	explicit LineSegment2(Vec2 startPos, Vec2 endPos);

	void Translate(Vec2 translation);
	void SetCenter(Vec2 newCenter);
	void RotateAboutCenter(float rotationDeltaDegrees);
};