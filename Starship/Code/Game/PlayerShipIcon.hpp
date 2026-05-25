#pragma once
#include "Game/UIElement.hpp"// this include the g_theRenderer, g_theApp, g_rng

// initialize the variable before the compile of the cpp file
constexpr int		PLAYERSHIPICON_NUM_TRI = 5; // the triangles that this entity uses
constexpr int		PLAYERSHIPICON_NUM_VERT = PLAYERSHIPICON_NUM_TRI * 3; // the vertexs that this entity contains

class PlayerShipIcon : public UIElement
{
public:
	PlayerShipIcon(Game* owner, Vec2 const& startPos);
	virtual ~PlayerShipIcon();

	virtual void Update(float deltaSeconds) override;	// 'override' states that the virtual function is being override
	virtual void Render() const override;

	void RenderFlame() const;
	void RenderShip() const;

	virtual void InitializeLocalVerts() override;

private:
	void UpdateThrustFlameVerts();
	void UPpdateIconPosition(float deltaSeconds);
	Vertex_PCU	m_localVerts[PLAYERSHIPICON_NUM_VERT];
	Vertex_PCU	m_thrustFlameVerts[3];
};