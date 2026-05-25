#pragma once
#include "Game/Entity.hpp"// this include the g_theRenderer, g_theApp, g_rng

constexpr int CAPRICORN_TRI = 2;
constexpr int CAPRICORN_NUM_VERT = CAPRICORN_TRI * 2;

enum class CAPRICORNAIState
{
	WANDERING,
	CHASING,
	NUM_CAPRICORNMODE
};

class Capricorn : public Entity
{
public:
	Capricorn(Map* owner, EntityType type, EntityFaction faction, Vec2 const& startPos, float angleDegrees);
	virtual ~Capricorn();

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

	CAPRICORNAIState m_AIState = CAPRICORNAIState::WANDERING;
	CAPRICORNAIState m_LastFrame_AIState = CAPRICORNAIState::WANDERING;

private:

	bool m_turretNewGoal = false;
	bool m_bodyNewGoal = false;
};

