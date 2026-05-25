#include "Engine/Math/Capsule2.hpp"

Capsule2::Capsule2(Capsule2 const& copyfrom)
{
	m_start = copyfrom.m_start;
	m_end	= copyfrom.m_end;
	m_radius= copyfrom.m_radius;
}

Capsule2::Capsule2(Vec2 startPos, Vec2 endPos, float radius)
{
	m_start  = startPos;
	m_end	 = endPos;
	m_radius = radius;
}

void Capsule2::Translate(Vec2 translation)
{
	m_start += translation;
	m_end += translation;
}

void Capsule2::SetCenter(Vec2 newCenter)
{
	Vec2 center = (m_start + m_end) * 0.5f;
	Vec2 translation = newCenter - center;
	m_start += translation;
	m_end += translation;
}

void Capsule2::RotateAboutCenter(float rotationDeltaDegrees)
{
	Vec2 center = (m_start + m_end) * 0.5f;

	Vec2 startDisp = m_start - center;
	startDisp.GetRotatedDegrees(rotationDeltaDegrees);
	m_start = center + startDisp;

	Vec2 endDisp = m_end - center;
	endDisp.GetRotatedDegrees(rotationDeltaDegrees);
	m_end = center + endDisp;
}
