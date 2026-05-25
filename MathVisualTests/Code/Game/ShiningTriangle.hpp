#pragma once
#include "Game/UI.hpp"// this include the g_theRenderer, g_theApp, g_rng

// initialize the variable before the compile of the cpp file
constexpr int		ATTRACTMODE_TRI_NUM_TRI = 1; // the triangles that this entity uses
constexpr int		ATTRACTMODE_TRI_NUM_VERT = ATTRACTMODE_TRI_NUM_TRI * 3; // the vertexs that this entity contains

class ShiningTriangle : public UI
{
public:
	ShiningTriangle(Vec2 const& startPos);
	virtual ~ShiningTriangle();

	virtual void Update(float deltaSeconds) override;	// 'override' states that the virtual function is being override
	virtual void Render() const override;
	void UpdateAlphaColor(float deltaSeconds);

	virtual void InitializeLocalVerts() override;
	float m_shiningTimer = 0.f;
	bool  m_brightnessIncreasing = true;
	float m_scaleTimer = 0.f;
	float colorChangingRate = 5.0f;
private:

	Vertex_PCU	m_localVerts[ATTRACTMODE_TRI_NUM_VERT];
};