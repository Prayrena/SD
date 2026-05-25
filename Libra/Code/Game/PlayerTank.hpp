#pragma once
#include "Game/Entity.hpp"
#include "Engine/input/XboxController.hpp"
#include <vector>


// initialize the variable before the compile of the cpp file
constexpr int		PLAYERTANK_NUM_TRI = 4;
constexpr int		PLAYERTANK_NUM_VERT = PLAYERTANK_NUM_TRI * 3;


class PlayerTank : public Entity
{
public:
	PlayerTank(Map* owner, EntityType type, EntityFaction faction, Vec2 const& startPos, float angleDegrees);
	virtual ~PlayerTank();

	virtual void Update( float deltaSeconds ) override;	// 'override' states that the virtual function is being override

	//virtual void TakeDamage() override;
	virtual void Render() const override;
	virtual void DebugRender() const override;

	virtual bool IsOffScreen() const override;
	virtual void AddVertsForRender() override;

	void RenderTank() const;

	virtual void ReactToBulletHit(Bullet& bullet) override;

	void ResetToRespawn();
	///<player control>
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void PlayerControl();
	void CheckIfPlayerContinueToPlayAfterDeath();
	void GetInputOrientationDegreesForBody();
	void GetInputOrientationDegreesForTurret();	
	void AlignTankMovementWithInput(float deltaSeconds);

	// Movement settings
	float  m_speed;
	float  m_fractionOfSpeed = 0.f;

	bool	m_explosionAnimationPlayed = false;

private:
	bool m_turretNewGoal = false;
	bool m_bodyNewGoal = false;
	std::vector<Vertex_PCU> m_tankBody_LocalVerts;
	std::vector<Vertex_PCU> m_tankTurret_LocalVerts;
	Vertex_PCU	m_tankVerts[PLAYERTANK_NUM_VERT];
	Vertex_PCU  m_thrustFlameVerts[3];
};

