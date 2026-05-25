#pragma once
#include "Game/Entity.hpp"// this include the g_theRenderer, g_theApp, g_rng
#include "Engine/input/XboxController.hpp"


// initialize the variable before the compile of the cpp file
constexpr int		PLAYERSHIP_NUM_TRI = 5; // the triangles that this entity uses
constexpr int		PLAYERSHIP_NUM_VERT = PLAYERSHIP_NUM_TRI * 3; // the vertexs that this entity contains


class PlayerShip : public Entity
{
public:
	PlayerShip(Game* owner, Vec2 const& startPos);
	virtual ~PlayerShip();

	virtual void Update() override;	// 'override' states that the virtual function is being override

	virtual void TakeDamage() override;
	virtual void Render() const override;
	virtual void DebugRender() const override;

	virtual bool IsOffScreen() const override;
	virtual void InitializeLocalVerts() override;

	Vec2 deltaVelocity;

	// Movement settings
	float m_thrustFraction= 0.f;

	// debris settings
	bool	m_debrisPlayed		= false;
	int		m_debris_min_num	= PLAYERSHIP_DEBRIS_MIN_NUM;
	int		m_debris_max_num	= PLAYERSHIP_DEBRIS_MAX_NUM;
	bool	m_speedBooster		= false;
	bool	m_fireRateBooster	= false;
	bool	m_shieldGenerated	= false;
	float   m_fireRateInterval  = 0.f;
	float   m_fireTimer			= 0.f;
	float   m_shieldThickness = 3.f;
	float   m_shieldRadius    = 3.f;


private:
	void BounceOffWalls();
	void RenderShip() const;

	Vertex_PCU	m_localVerts[PLAYERSHIP_NUM_VERT];
	Vertex_PCU  m_thrustFlameVerts[3];

	void UpdateThrustFlameVerts();
	void InitializeFlameVerts();
	void RenderFlame() const;

	void DrawShieldRing(Vec2 const& center, Rgba8 const& colorA, Rgba8 const& colorB) const;
	void RenderShield() const;
};

