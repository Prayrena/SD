#pragma once
#include "Game/UIElement.hpp"// this include the g_theRenderer, g_theApp, g_rng

// initialize the variable before the compile of the cpp file
constexpr int		ENERGYBAR_TRI_NUM_TRI = 4; // the triangles that this entity uses
constexpr int		ENERGYBAR_TRI_NUM_VERT = ENERGYBAR_TRI_NUM_TRI * 3; // the vertexs that this entity contains

class EnergyBar : public UIElement
{
public:
	EnergyBar(Game* owner, Vec2 const& startPos);
	virtual ~EnergyBar();

	virtual void Update(float deltaSeconds) override;	// 'override' states that the virtual function is being override
	virtual void Render() const override;

	virtual void InitializeLocalVerts() override;
	float m_energyRemain = 1.f;
	float m_energyConsuming = 0.f;
	float m_shiningTimer = 0.f;
	float m_scaleTimer = 0.f;
	float m_colorChangingRate = 9.f;

private:
	void UpdateEnergyRemain(float deltaSeconds);

	Vertex_PCU	m_localVerts[ENERGYBAR_TRI_NUM_VERT];
	Rgba8 m_energyRemainColor;
};