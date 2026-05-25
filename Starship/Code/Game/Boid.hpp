#pragma once
#include "Game/Entity.hpp"// this include the g_theRenderer, g_theApp, g_rng


// initialize the variable before the compile of the cpp file
constexpr int		Boid_NUM_TRI = 1; // the triangles that this entity uses
constexpr int		Boid_NUM_VERT = Boid_NUM_TRI * 3; // the vertexs that this entity contains


class Boid : public Entity
{
public:
	Boid(Game* owner, Vec2 const& startPos);
	virtual ~Boid();

	virtual void Update( float deltaSeconds ) override;	// 'override' states that the virtual function is being override
	virtual void TakeDamage();
	virtual void Render() const override;
	virtual void DebugRender() const override;

	virtual bool IsOffScreen() const override;

	Vec2 deltaVelocity;

	// debris settings
	Rgba8   m_debrisColor = Rgba8(WASP_COLOR_R, WASP_COLOR_G, WASP_COLOR_B, DEBRIS_COLOR_A);
	float   debris_outer_radius = DEBRIS_MAX_SCALE * WASP_PHYSICS_RADIUS;
	float   debris_inner_radius = DEBRIS_MIN_SCALE * WASP_PHYSICS_RADIUS;
	Vec2	m_normal;
	Vec2	m_seperateVelocity;
	Vec2	m_alignVelocity;
	Vec2	m_cohereVelocity;
	Vec2	m_playerAttractionVelocity;
	Boid*   m_nearestFriend = nullptr;

	float   m_thrustingTimer = 0.f;

private:
	void InitializeLocalVerts();
	void UpdateFromKeyBoard(float deltaSeconds);
	void TransformBoidAcrossScreen();
	void Respawn();
	void RenderShip() const;

	Vertex_PCU	m_localVerts[Boid_NUM_VERT];
	void UpdateOrientation();
};

