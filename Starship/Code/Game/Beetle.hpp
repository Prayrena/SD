#pragma once
#include "Game/Entity.hpp"// this include the g_theRenderer, g_theApp, g_rng


// initialize the variable before the compile of the cpp file
constexpr int		BEETLE_NUM_TRI = 11; // the triangles that this entity uses
constexpr int		BEETLE_NUM_VERT = BEETLE_NUM_TRI * 3; // the vertexs that this entity contains


class Beetle : public Entity
{
public:
	Beetle(Game* owner, Vec2 const& startPos);
	virtual ~Beetle();

	virtual void Update( float deltaSeconds ) override;	// 'override' states that the virtual function is being override
	virtual void TakeDamage();
	virtual void Render() const override;
	virtual void DebugRender() const override;

	virtual bool IsOffScreen() const override;

	Vec2 deltaVelocity;

	// debris settings
	Rgba8   m_debrisColor = Rgba8(BEETLE_COLOR_R, BEETLE_COLOR_G, BEETLE_COLOR_B, DEBRIS_COLOR_A);
	float   debris_outer_radius = DEBRIS_MAX_SCALE * BEETLE_PHYSICS_RADIUS;
	float   debris_inner_radius = DEBRIS_MIN_SCALE * BEETLE_PHYSICS_RADIUS;

private:
	void InitializeLocalVerts();
	void RenderShip() const;

	Vertex_PCU	m_localVerts[BEETLE_NUM_VERT];
};

