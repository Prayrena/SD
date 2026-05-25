#include "Engine/Math/LineSegment2.hpp"

LineSegment2::LineSegment2(LineSegment2 const& copyfrom)
{
	m_start = copyfrom.m_start;
	m_end	= copyfrom.m_end;
}

LineSegment2::LineSegment2(Vec2 startPos, Vec2 endPos)
{
	m_start  = startPos;
	m_end	 = endPos;
}

void LineSegment2::Translate(Vec2 translation)
{
	m_start += translation;
	m_end += translation;
}

void LineSegment2::SetCenter(Vec2 newCenter)
{
	Vec2 center = (m_start + m_end) * 0.5f;
	Vec2 translation = newCenter - center;
	m_start += translation;
	m_end += translation;
}

void LineSegment2::RotateAboutCenter(float rotationDeltaDegrees)
{
	Vec2 center = (m_start + m_end) * 0.5f;

	Vec2 startDisp = m_start - center;
	startDisp.GetRotatedDegrees(rotationDeltaDegrees);
	m_start = center + startDisp;

	Vec2 endDisp = m_end - center;
	endDisp.GetRotatedDegrees(rotationDeltaDegrees);
	m_end = center + endDisp;
}
