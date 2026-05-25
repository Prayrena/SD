#include "Engine/Math/SecondOrderDynamics.hpp"
#include "Engine/Core/EngineCommon.hpp"

SecondOrderDynamics::SecondOrderDynamics(float f, float z, float r, Vec3 x_input)
	: m_position_previous(x_input)
	, m_f(f)
	, m_z(z)
	, m_r(r)
{
	// compute the three constant for the equation
	m_k1 = m_z / (PI * m_f);
	m_k2 = 1.f / ((2.f * PI * m_f) * (2.f * PI * m_f));
	m_k3 = (m_r * m_z) / (2.f * PI * m_f);

	// calculate the max of delta seconds that the system cannot exceed to keep stable
	m_deltaSecondsCrit = 0.8f * (sqrt(4.f * m_k2 + m_k1 * m_k1) - m_k1);	// the 0.8f is trying to keep the safe boundary to the max

	// initialize the variables for the system
	m_position_previous = x_input;
	m_goal_position = x_input;
	m_goal_velocity = Vec3();
}

void SecondOrderDynamics::Update(float deltaSeconds, Vec3 x_current, Vec3 x_accerleration /*= Vec3::INVALID*/)
{
	// calculate input acceleration if player not input valid value
	if (x_accerleration == Vec3::INVALID)
	{
		x_accerleration = (x_current - m_position_previous) / deltaSeconds;
	}

	// update x record
	m_position_previous = x_current;

	// // check and make sure the deltaSeconds is under safe time step boundary
	// int numInteration = (int)ceil(deltaSeconds / m_deltaSecondsCrit);
	// float timeStep = deltaSeconds / (float)numInteration;
	// 
	// // update the velocity and acceleration
	// for (int i = 0; i < numInteration; i++)
	// {
	// 	m_goal_position = m_goal_position + timeStep * m_goal_velocity;
	// 	m_goal_velocity = m_goal_velocity + (timeStep * (x_current + m_k3 * x_accerleration - m_goal_position - m_k1 * m_goal_velocity)) / m_k2;
	// }

	if (m_isUsingFixedPhysicsTimestep)
	{
		m_physicsTimeOwed += deltaSeconds;
		while (m_physicsTimeOwed >= m_physicsFixedTimestep)
		{
			m_physicsTimeOwed -= m_physicsFixedTimestep;
			m_goal_position = m_goal_position + m_physicsFixedTimestep * m_goal_velocity;
		}	m_goal_velocity = m_goal_velocity + (m_physicsFixedTimestep * (x_current + m_k3 * x_accerleration - m_goal_position - m_k1 * m_goal_velocity)) / m_k2;
	}
	else
	{
		m_goal_position = m_goal_position + m_physicsFixedTimestep * m_goal_velocity;
		m_goal_velocity = m_goal_velocity + (m_physicsFixedTimestep * (x_current + m_k3 * x_accerleration - m_goal_position - m_k1 * m_goal_velocity)) / m_k2;
	}
}

void SecondOrderDynamics::UpdateParameters(float f, float z, float r, Vec3 x_input)
{
	m_position_previous = x_input;
	m_f = f;
	m_z = z;
	m_r = r;

	// compute the three constant for the equation
	m_k1 = m_z / (PI * m_f);
	m_k2 = 1.f / ((2.f * PI * m_f) * (2.f * PI * m_f));
	m_k3 = (m_r * m_z) / (2.f * PI * m_f);

	// calculate the max of delta seconds that the system cannot exceed to keep stable
	m_deltaSecondsCrit = 0.8f * (sqrt(4.f * m_k2 + m_k1 * m_k1) - m_k1);	// the 0.8f is trying to keep the safe boundary to the max

	// initialize the variables for the system
	m_position_previous = x_input;
	m_goal_position = x_input;
	m_goal_velocity = Vec3();
}

