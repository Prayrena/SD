#pragma once
#include "Game/Entity.hpp"// this include the g_theRenderer, g_theApp, g_rng


// initialize the variable before the compile of the cpp file
constexpr int		WASP_NUM_TRI = 8; // the triangles that this entity uses
constexpr int		WASP_NUM_VERT = WASP_NUM_TRI * 3; // the vertexs that this entity contains


class Wasp : public Entity
{
public:
	Wasp(Game* owner, Vec2 const& startPos);
	virtual ~Wasp();

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

private:
	void InitializeLocalVerts();
	void UpdateFromKeyBoard(float deltaSeconds);
	void BounceOffWalls();
	void Respawn();
	void RenderShip() const;

	Vertex_PCU	m_localVerts[WASP_NUM_VERT];
};

