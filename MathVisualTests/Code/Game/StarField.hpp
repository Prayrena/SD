#pragma once
#include "Game/UI.hpp"// this include the g_theRenderer, g_theApp, g_rng

// initialize the variable before the compile of the cpp file
constexpr int		STARFIELD_TRI_NUM_TRI = 4; // the triangles that this entity uses
constexpr int		STARFIELD_TRI_NUM_VERT = STARFIELD_TRI_NUM_TRI * 3; // the vertexs that this entity contains
//constexpr int		STARFIELD_DRAW_POINTS_NUM = 9;

class StarField : public UI
{
public:
	StarField(GetNearestPointsScene* owner, Vec2 const& startPos);
	virtual ~StarField();

	virtual void Update(float deltaSeconds) override;	// 'override' states that the virtual function is being override
	virtual void Render() const override;
	void UpdateAlphaColor(float deltaSeconds);

	virtual void InitializeLocalVerts() override;
	float m_shiningTimer = 0.f;
	bool  m_brightnessIncreasing = true;

	float m_counterPlayerVelocityScale = 0.f;

private:
	Vertex_PCU	m_localVerts[STARFIELD_TRI_NUM_VERT];
	//Vertex_PCU  m_drawVerts[STARFIELD_DRAW_POINTS_NUM];
};