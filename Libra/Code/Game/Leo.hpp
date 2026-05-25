#pragma once
#include "Game/Entity.hpp"// this include the g_theRenderer, g_theApp, g_rng

constexpr int LEO_TRI = 2;
constexpr int LEO_NUM_VERT = LEO_TRI * 2;

enum class LeoAIState
{
	WANDERING,
	CHASING,
	NUM_LEOMODE
};

class Leo : public Entity
{
public:
	Leo(Map* owner, EntityType type, EntityFaction faction, Vec2 const& startPos, float angleDegrees);
	virtual ~Leo();

	virtual void Update( float deltaSeconds ) override;	// 'override' states that the virtual function is being override

	//virtual void TakeDamage() override;
	virtual void Render() const override;
	virtual void DebugRender() const override;

	virtual bool IsOffScreen() const override;
	virtual void AddVertsForRender() override;

	void RenderTank() const;
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// AI
	void UpdateAIStateController();
	void UpdateAction(float deltaSeconds);
	void WanderOnMap(float deltaSeconds);
	void ChasingPlayer(float deltaSeconds);

	// Movement settings
	float  m_age = 0.f;
	float  m_speed= 1.f;
	float  m_fractionOfSpeed = 0.f;

	LeoAIState m_AIState = LeoAIState::WANDERING;
	LeoAIState m_LastFrame_AIState = LeoAIState::WANDERING;
private:

	bool m_turretNewGoal = false;
	bool m_bodyNewGoal = false;
};

