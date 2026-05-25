#pragma once
#include "Game/Entity.hpp"

constexpr int ARIES_TRI = 2;
constexpr int ARIES_NUM_VERT = ARIES_TRI * 2;

class Bullet;
class Map;

enum class AriesAIState
{
	WANDERING,
	CHASING,
	NUM_ARIESMODE
};

class Aries : public Entity
{
public:
	Aries(Map* owner, EntityType type, EntityFaction faction, Vec2 const& startPos, float angleDegrees);
	virtual ~Aries();

	virtual void Update( float deltaSeconds ) override;	// 'override' states that the virtual function is being override

	//virtual void TakeDamage() override;
	virtual void Render() const override;
	virtual void DebugRender() const override;

	virtual bool IsOffScreen() const override;
	virtual void AddVertsForRender() override;

	void RenderTank() const;
	void RenderShield();

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void UpdateAIStateController();
	void UpdateAction(float deltaSeconds);
	void WanderOnMap(float deltaSeconds);
	void ChasingPlayer(float deltaSeconds);
	virtual void ReactToBulletHit(Bullet& bullet) override;

	// Movement settings
	float  m_age = 0.f;
	float  m_speed= 1.f;
	float  m_fractionOfSpeed = 0.f;

	AriesAIState m_AIState = AriesAIState::WANDERING;
	AriesAIState m_LastFrame_AIState = AriesAIState::WANDERING;

private:
	bool m_turretNewGoal = false;
	bool m_bodyNewGoal = false;
	std::vector<Vertex_PCU> m_localVerts;
};

