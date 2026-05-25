#pragma once
#include "Game/Entity.hpp"// this include the g_theRenderer, g_theApp, g_rng
#include "Engine/input/XboxController.hpp"


// initialize the variable before the compile of the cpp file
constexpr int		SCORPIO_NUM_TRI = 4;
constexpr int		SCORPIO_NUM_VERT = SCORPIO_NUM_TRI * 3;

enum class ScorpioState
{
	SCANNING,
	AIMING,
	NUM_LEOMODE
};

class Scorpio : public Entity
{
public:
	Scorpio(Map* owner, EntityType type, EntityFaction faction, Vec2 const& startPos, float angleDegrees);
	virtual ~Scorpio();

	virtual void Update( float deltaSeconds ) override;	// 'override' states that the virtual function is being override

	//virtual void TakeDamage() override;
	virtual void Render() const override;
	void RenderLaserAiming() const;
	virtual void DebugRender() const override;

	virtual bool IsOffScreen() const override;
	virtual void AddVertsForRender() override;

	void RenderTank() const;
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// AI
	void UpdateAIStateController();
	void UpdateAction(float deltaSeconds);
	void ScanSurroundings(float deltaSeconds);
	void Rotate_AimAtThePlayer(float deltaSeconds);

	// Movement settings
	float  m_age = 0.f;
	float  m_speed= 1.f;
	float  m_fractionOfSpeed = 0.f;

	ScorpioState m_AIState = ScorpioState::SCANNING;
	ScorpioState m_LastFrame_AIState = ScorpioState::SCANNING;
	bool m_playerLastSeenPosIsReached = true;
	Vec2 m_playerLastKnownTileCoords; 

private:
	bool m_turretNewGoal = false;
	bool m_bodyNewGoal = false;
	std::vector<Vertex_PCU> m_tankBody_LocalVerts;
	std::vector<Vertex_PCU> m_tankTurret_LocalVerts;
};

