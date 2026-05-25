#pragma once
#include "Engine/Math/Vec2.hpp"

struct RaycastResult2D
{
public:
	// Basic ray cast result information
	bool	m_didImpact = false;
	float	m_impactDist = 0.f;
	Vec2	m_impactPos = Vec2::ZERO;
	Vec2	m_impactNormal = Vec2::ZERO;

	// Original ray cast information
	Vec2	m_rayFwdNormal = Vec2::ZERO;
	Vec2	m_rayStartPos = Vec2::ZERO;
	float	m_rayMaxLength = 1.f;
};