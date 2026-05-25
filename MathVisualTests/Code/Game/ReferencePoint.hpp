#pragma once
#include "Game/Entity.hpp"// this include the g_theRenderer, g_theApp, g_rng
#include "Engine/input/XboxController.hpp"


// initialize the variable before the compile of the cpp file
constexpr int		PLAYERSHIP_NUM_TRI = 5; // the triangles that this entity uses
constexpr int		PLAYERSHIP_NUM_VERT = PLAYERSHIP_NUM_TRI * 3; // the vertexs that this entity contains


class ReferencePoint : public Entity
{
public:
	ReferencePoint(Vec2 const& startPos);
	virtual ~ReferencePoint();

	virtual void Update( float deltaSeconds ) override;	// 'override' states that the virtual function is being override

	virtual void Render() const override;
	virtual void DebugRender() const override;

	virtual bool IsOffScreen() const override;
	virtual void InitializeLocalVerts() override;

	Vec2 deltaVelocity;

private:
	void RenderShip() const;

	Vertex_PCU	m_localVerts[PLAYERSHIP_NUM_VERT];
	Vertex_PCU  m_thrustFlameVerts[3];
};

