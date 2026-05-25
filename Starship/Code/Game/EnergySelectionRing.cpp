#include "Game/EnergySelectionRing.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Game/GameCommon.hpp"
#include "Game/App.hpp"
#include "Game/Game.hpp"
#include "Game/PlayerShip.hpp"

extern Renderer* g_theRenderer;
extern App* g_theApp;
extern InputSystem* g_theInput;


// the children's constructor function need to initialize its parent class constructor first
EnergySelectionRing::EnergySelectionRing(Game* owner, Vec2 const& startPos)
	:UIElement(owner, startPos)// constructing the entity before the constructing the bullet
{
	m_UIColor = Rgba8(BEETLE_COLOR_R, BEETLE_COLOR_G, BEETLE_COLOR_B, 255);
	m_energyRingColor = Rgba8(ENERGY_SELECTION_RING_COLOR_R, ENERGY_SELECTION_RING_COLOR_G, ENERGY_SELECTION_RING_COLOR_B, 255);
	m_highlightColor = Rgba8(ENERGY_REMAIN_COLOR_R, ENERGY_REMAIN_COLOR_G, ENERGY_REMAIN_COLOR_B, ENERGY_REMAIN_COLOR_A);
}

EnergySelectionRing::~EnergySelectionRing()
{
}

void EnergySelectionRing::Update(float deltaSeconds)
{
	InitializeLocalVerts();
	UpdateRadiusThickness(deltaSeconds);
}


void EnergySelectionRing::UpdateRadiusThickness(float deltaSeconds)
{
	m_age += deltaSeconds;

	m_ringRadius = RangeMapClamped(m_age, 0.f, 0.06f, 100.f, 30.f);
	m_thickness = RangeMapClamped(m_age, 0.f, 0.06f, 30.f, 3.f);
}


void EnergySelectionRing::Render() const //all called functions must be const
{
	// is current ship is destroyed, do not render
	if (!m_isEnabled)
	{
		return;
	}

	Vertex_PCU tempWorldVerts[ENERGYRING_TRI_NUM_VERT];

	int vertIndex = 0;
	int& vi = vertIndex;

	for (vi; vi < ENERGYRING_TRI_NUM_VERT; ++vi)
	{
		tempWorldVerts[vi] = m_localVerts[vi];
	}

	TransformVertexArrayXY3D(ENERGYRING_TRI_NUM_VERT, tempWorldVerts, 1.f, m_orientationDegrees, m_position);
	g_theRenderer->DrawVertexArray(ENERGYRING_TRI_NUM_VERT, tempWorldVerts);

	// create the disk for icons
	Vec2 centerPos = Vec2(WORLD_SIZE_X * 0.5f, WORLD_SIZE_Y * 0.5f);
	float length = m_ringRadius * 0.75f;
	float thetaA = -30.f;
	float thetaB =  90.f;
	float thetaC = 210.f;

	Vec2 iconAPos = centerPos.MakeFromPolarDegrees(thetaA, length);
	Vec2 iconBPos = centerPos.MakeFromPolarDegrees(thetaB, length);
	Vec2 iconCPos = centerPos.MakeFromPolarDegrees(thetaC, length);

	iconAPos += centerPos;
	iconBPos += centerPos;
	iconCPos += centerPos;

	DrawDisk(iconAPos, 3.f, m_iconColorA, m_iconColorA);
	DrawDisk(iconBPos, 3.f, m_iconColorB, m_iconColorB);
	DrawDisk(iconCPos, 3.f, m_iconColorC, m_iconColorC);

	DrawIconShield(iconAPos);
	DrawIconVelocity(iconBPos);
	DrawIconWeapon(iconCPos);
}


// Energy bar contains two part, the background and current usage
void EnergySelectionRing::InitializeLocalVerts()
{
	// right analog decides which pie is selected
	Rgba8 colorA = m_energyRingColor;
	Rgba8 colorB = m_energyRingColor;
	Rgba8 colorC = m_energyRingColor;

	m_iconColorA = m_energyRingColor;
	m_iconColorB = m_energyRingColor;
	m_iconColorC = m_energyRingColor;

	XboxController const& controller = g_theInput->GetController(0);//?????????????????????????????????????????????? if get controller from playership, does not work

	//float rightAnalgoMagnitude = m_game->m_playerShip->m_controller.GetLeftstick().GetMagnitude();
	//float rightAnalgoOrientation = m_game->m_playerShip->m_controller.GetRightstick().GetOrientationDegrees();
	float rightAnalgoMagnitude = controller.GetRightstick().GetMagnitude();
	float rightAnalgoOrientation = controller.GetRightstick().GetOrientationDegrees();
		
	if (rightAnalgoOrientation < 0.f)
	{
		rightAnalgoOrientation += 360.f;
	}

	// changing the bool needs both the magnitude as well as orientation
	if (rightAnalgoMagnitude > 0.5f || g_theInput->IsKeyDown('K'))
	{
		if (rightAnalgoOrientation > 270.f || rightAnalgoOrientation <= 30.f || g_theInput->IsKeyDown('L'))
		{
			m_game->m_playerShip->m_shieldGenerated = true;
			m_game->m_playerShip->m_fireRateBooster = false;
			m_game->m_playerShip->m_speedBooster = false;
		}

		if (rightAnalgoOrientation > 30.f && rightAnalgoOrientation <= 150.f)
		{
			m_game->m_playerShip->m_shieldGenerated = false;
			m_game->m_playerShip->m_fireRateBooster = false;
			m_game->m_playerShip->m_speedBooster = true;
		}

		if (g_theInput->IsKeyDown('I'))
		{
			m_game->m_playerShip->m_shieldGenerated = false;
			m_game->m_playerShip->m_fireRateBooster = false;
			m_game->m_playerShip->m_speedBooster = true;
		}

		if (rightAnalgoOrientation > 150.f && rightAnalgoOrientation <= 270.f)
		{
			m_game->m_playerShip->m_shieldGenerated = false;
			m_game->m_playerShip->m_fireRateBooster = true;
			m_game->m_playerShip->m_speedBooster = false;
		}

		if (g_theInput->IsKeyDown('J'))
		{
			m_game->m_playerShip->m_shieldGenerated = false;
			m_game->m_playerShip->m_fireRateBooster = true;
			m_game->m_playerShip->m_speedBooster = false;
		}
	}

	// the color changing only depends on the bool
	if (m_game->m_playerShip->m_shieldGenerated)
	{
		colorA = m_highlightColor;
		m_iconColorA = m_highlightColor;
	}

	if (m_game->m_playerShip->m_speedBooster)
	{
		colorB = m_highlightColor;
		m_iconColorB = m_highlightColor;
	}

	if (m_game->m_playerShip->m_fireRateBooster)
	{
		colorC = m_highlightColor;
		m_iconColorC = m_highlightColor;
	}

	CreateEnergyRingVerts( static_cast<int>( ENERGY_RING_HIDING_SIDES * 0.5 ), static_cast<int>( ENERGY_RING_DISPLAY_SIDES + ENERGY_RING_HIDING_SIDES * 0.5), colorA);
	CreateEnergyRingVerts( static_cast<int>( ENERGY_RING_HIDING_SIDES * 1.5 + ENERGY_RING_DISPLAY_SIDES ), static_cast<int>(ENERGY_RING_DISPLAY_SIDES * 2 + ENERGY_RING_HIDING_SIDES * 1.5), colorB);
	CreateEnergyRingVerts( static_cast<int>( ENERGY_RING_HIDING_SIDES * 2.5 + ENERGY_RING_DISPLAY_SIDES * 2 ), static_cast<int>(ENERGY_RING_DISPLAY_SIDES * 3 + ENERGY_RING_HIDING_SIDES * 2.5), colorC);
}

void EnergySelectionRing::CreateEnergyRingVerts(int indexStart, int indexEnd, Rgba8 m_ringColor)
{
	// the goal is to draw trapezoid for DEBUG_NUM_SIDES times in a circle

	// get the R
	float halfThickness = .5f * m_thickness;
	float innerRadius = m_ringRadius - halfThickness;
	float outerRadius = m_ringRadius + halfThickness;

	//get each pie's degree
	Vec3 centerPos = Vec3(WORLD_SIZE_X * 0.5f, WORLD_SIZE_Y * 0.5f, 0.f);

	// first 1/3 ring
	for (int sideIndex = indexStart; sideIndex < indexEnd; ++sideIndex)
	{
		// get the theta_degrees
		float startDegrees = ENERGY_RING_DEGREES_PERSIDE * static_cast<float>(sideIndex);
		float endDegrees = ENERGY_RING_DEGREES_PERSIDE * static_cast<float>(sideIndex + 1);

		// use polar system( cos radian and sin radian) to get Cartesian system
		float cosStrtPt = CosDegrees(startDegrees - 90.f);
		float sinStrdPt = SinDegrees(startDegrees - 90.f);
		float cosEndPt = CosDegrees(endDegrees - 90.f);
		float sinEndPt = SinDegrees(endDegrees - 90.f);

		// the local space position of four vertex
		Vec3 innerStrtPos(innerRadius * cosStrtPt, innerRadius * sinStrdPt, 0.f);
		Vec3 outerStrtPos(outerRadius * cosStrtPt, outerRadius * sinStrdPt, 0.f);
		Vec3 innerEndPos(innerRadius * cosEndPt, innerRadius * sinEndPt, 0.f);
		Vec3 outerEndPos(outerRadius * cosEndPt, outerRadius * sinEndPt, 0.f);

		innerStrtPos;
		outerStrtPos;
		innerEndPos;
		outerEndPos;

		// transform pos from local to world
		innerStrtPos += centerPos;
		outerStrtPos += centerPos;
		innerEndPos += centerPos;
		outerEndPos += centerPos;

		// trapezoid is made of two tris: tri ABC & tri DEF
		// C and E share the same spot, B and D share the same spot
		// match up the tri vertex with render vert array Index
		int VertIndexA = (sideIndex * 6) + 0;
		int VertIndexB = (sideIndex * 6) + 1;
		int VertIndexC = (sideIndex * 6) + 2;
		int VertIndexD = (sideIndex * 6) + 3;
		int VertIndexE = (sideIndex * 6) + 4;
		int VertIndexF = (sideIndex * 6) + 5;

		// assign the vertex with pos info
		// A is inner start, B is outer Start, C is inner end
		// D is outer end, E is inner end, F is outer start
		m_localVerts[VertIndexA].m_position = innerStrtPos;
		m_localVerts[VertIndexB].m_position = outerStrtPos;
		m_localVerts[VertIndexC].m_position =  innerEndPos;
		m_localVerts[VertIndexA].m_color = m_ringColor;
		m_localVerts[VertIndexB].m_color = m_ringColor;
		m_localVerts[VertIndexC].m_color = m_ringColor;

		m_localVerts[VertIndexD].m_position = outerEndPos;
		m_localVerts[VertIndexE].m_position = innerEndPos;
		m_localVerts[VertIndexF].m_position = outerStrtPos;
		m_localVerts[VertIndexD].m_color = m_ringColor;
		m_localVerts[VertIndexE].m_color = m_ringColor;
		m_localVerts[VertIndexF].m_color = m_ringColor;
	}

}

void EnergySelectionRing::DrawIconShield(Vec2 centerPos) const
{
	Vec3 worldPos = Vec3(centerPos.x, centerPos.y, 0.f);
	Vertex_PCU verts[9] = {};
	Vec3 pointA = Vec3(-1.2f, 1.f, 0.f);
	Vec3 pointB = Vec3(-1.f, -1.f, 0.f);
	Vec3 pointC = Vec3( 0.f, -2.f, 0.f);
	Vec3 pointD = Vec3(1.f, -1.f, 0.f);
	Vec3 pointE = Vec3(1.2f, 1.f, 0.f);

	verts[0].m_position = pointA + worldPos;
	verts[1].m_position = pointB + worldPos;
	verts[2].m_position = pointD + worldPos;

	verts[3].m_position = pointA + worldPos;
	verts[4].m_position = pointD + worldPos;
	verts[5].m_position = pointE + worldPos;

	verts[6].m_position = pointB + worldPos;
	verts[7].m_position = pointC + worldPos;
	verts[8].m_position = pointD + worldPos;

	g_theRenderer->DrawVertexArray(9, &verts[0]);
}

void EnergySelectionRing::DrawIconVelocity(Vec2 centerPos) const
{
	Vec3 worldPos = Vec3(centerPos.x, centerPos.y, 0.f);
	Vertex_PCU verts[9] = {};
	Vec3 pointA = Vec3(0.f, 1.8f, 0.f);
	Vec3 pointB = Vec3(0.f, -1.8f, 0.f);
	Vec3 pointC = Vec3(2.f, -1.8f, 0.f);
	Vec3 pointD = Vec3(2.f, 1.8f, 0.f);

	verts[0].m_position = pointA + worldPos;
	verts[1].m_position = pointB + worldPos;
	verts[2].m_position = pointC + worldPos;

	verts[3].m_position = pointA + worldPos;
	verts[4].m_position = pointC + worldPos;
	verts[5].m_position = pointD + worldPos;

	verts[6].m_position = Vec3(-2.f, 0.f, 0.f) + worldPos;
	verts[7].m_position = Vec3(0.f, .3f, 0.f) + worldPos;
	verts[8].m_position = Vec3(0.f, -.3f, 0.f) + worldPos;

	g_theRenderer->DrawVertexArray(9, &verts[0]);
}

void EnergySelectionRing::DrawIconWeapon(Vec2 centerPos) const
{
	Vec3 worldPos = Vec3(centerPos.x, centerPos.y, 0.f);
	Vertex_PCU verts[18] = {};
	Vec3 pointA = Vec3(-2.f, .6f, 0.f);
	Vec3 pointB = Vec3(-2.f, -.6f, 0.f);
	Vec3 pointC = Vec3(-.5f, -.6f, 0.f);
	Vec3 pointD = Vec3(-.5f, .6f, 0.f);
	Vec3 pointE = Vec3(-.5f, .3f, 0.f);
	Vec3 pointF = Vec3(-.5f, -.3f, 0.f);
	Vec3 pointG = Vec3(1.f, -.3f, 0.f);
	Vec3 pointH = Vec3(1.f, .3f, 0.f);
	Vec3 pointI = Vec3(1.f, .4f, 0.f);
	Vec3 pointJ = Vec3(1.f, -.4f, 0.f);
	Vec3 pointK = Vec3(1.8f, -0.4f, 0.f);
	Vec3 pointL = Vec3(1.8f, 0.4f, 0.f);

	verts[0].m_position = pointA + worldPos;
	verts[1].m_position = pointB + worldPos;
	verts[2].m_position = pointC + worldPos;

	verts[3].m_position = pointA + worldPos;
	verts[4].m_position = pointC + worldPos;
	verts[5].m_position = pointD + worldPos;

	verts[6].m_position = pointE + worldPos;
	verts[7].m_position = pointF + worldPos;
	verts[8].m_position = pointG + worldPos;

	verts[9].m_position = pointE + worldPos;
	verts[10].m_position = pointG + worldPos;
	verts[11].m_position = pointH + worldPos;

	verts[12].m_position = pointI + worldPos;
	verts[13].m_position = pointJ + worldPos;
	verts[14].m_position = pointK + worldPos;

	verts[15].m_position = pointI + worldPos;
	verts[16].m_position = pointK + worldPos;
	verts[17].m_position = pointL + worldPos;

	g_theRenderer->DrawVertexArray(18, &verts[0]);
}
