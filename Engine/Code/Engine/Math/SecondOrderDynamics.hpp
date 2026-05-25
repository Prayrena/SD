#pragma once
#include "Engine/Math/Vec3.hpp"

class SecondOrderDynamics
{
public:
	SecondOrderDynamics(float f, float z, float r, Vec3 x_input);
	~SecondOrderDynamics() = default;

	void Update(float deltaSeconds, Vec3 x_current, Vec3 x_accerleration = Vec3::INVALID);
	void UpdateParameters(float f, float z, float r, Vec3 x_input);

public:
	Vec3 m_position_previous = Vec3();
	Vec3 m_goal_position = Vec3();
	Vec3 m_goal_velocity = Vec3();

	float m_k1 = 0.f;
	float m_k2 = 0.f;
	float m_k3 = 0.f;

	float m_deltaSecondsCrit = 0.f;
	float m_f = 0.f;
	float m_z = 0.f;
	float m_r = 0.f;

	// fixed update
	bool	m_isUsingFixedPhysicsTimestep = true;
	float	m_physicsTimeOwed = 0.f;
	float	m_physicsFixedTimestep = (1.f / 60.f) / 10.f;	// 10 physics tick per frame at 60 FPS
};