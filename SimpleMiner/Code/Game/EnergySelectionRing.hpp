#pragma once
#include "Game/UI.hpp"// this include the g_theRenderer, g_theApp, g_rng

// initialize the variable before the compile of the cpp file
constexpr int		ENERGY_RING_NUM_SIDES = 90;
constexpr int		ENERGY_RING_DEGREES_PERSIDE = 360 / ENERGY_RING_NUM_SIDES;
constexpr int		ENERGY_RING_DISPLAY_SIDES = 21;
constexpr int		ENERGY_RING_HIDING_SIDES = (ENERGY_RING_NUM_SIDES - ENERGY_RING_DISPLAY_SIDES * 3) / 3;

constexpr int		ENERGYRING_TRI_NUM_TRI = ENERGY_RING_NUM_SIDES * 2 * 3; // the triangles that this entity uses
constexpr int		ENERGYRING_TRI_NUM_VERT = ENERGYRING_TRI_NUM_TRI * 3; // the vertexs that this entity contains

class EnergySelectionRing : public UI
{
public:
	EnergySelectionRing(Game* owner, Vec2 const& startPos);
	virtual ~EnergySelectionRing();

	virtual void Update(float deltaSeconds) override;	// 'override' states that the virtual function is being override
	virtual void Render() const override;
	void UpdateRadiusThickness(float deltaSeconds);

	virtual void InitializeLocalVerts() override;
	void CreateEnergyRingVerts(int indexStart, int indexEnd, Rgba8 m_ringColor);
	void DrawIconShield(Vec2 certerPos) const;
	void DrawIconVelocity(Vec2 certerPos) const;
	void DrawIconWeapon(Vec2 centerPos) const;

	float m_reformDuration = .3f;
	float m_ringRadius = 30.f;
	float m_thickness = 10.f;
	float m_selectionAngle = 0.f;
	float m_shiningTimer = 0.f;
	bool  m_brightnessIncreasing = true;
	float m_scaleTimer = 0.f;
	float m_colorChangingRate = 9.f;

private:
	Vertex_PCU	m_localVerts[ENERGYRING_TRI_NUM_VERT];
	Vertex_PCU	m_diskAVerts[DISK_NUM_VERTS];
	Vertex_PCU	m_diskBVerts[DISK_NUM_VERTS];
	Vertex_PCU	m_diskCVerts[DISK_NUM_VERTS];
	Rgba8 m_energyRingColor;
	Rgba8 m_highlightColor;
	Rgba8 m_iconColorA;
	Rgba8 m_iconColorB;
	Rgba8 m_iconColorC;
};