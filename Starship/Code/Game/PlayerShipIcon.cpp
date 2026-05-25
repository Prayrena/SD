#include "Game/PlayerShipIcon.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Game/GameCommon.hpp"
#include "Game/App.hpp"
#include "Game/Game.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"

extern Renderer* g_theRenderer;
extern App* g_theApp;
extern RandomNumberGenerator* g_rng;

// the children's constructor function need to initialize its parent class constructor first
PlayerShipIcon::PlayerShipIcon(Game* owner, Vec2 const& startPos)
	:UIElement(owner, startPos)// constructing the entity before the constructing the bullet
{
	m_uniformScale = 0.9f;
	InitializeLocalVerts();
}

PlayerShipIcon::~PlayerShipIcon()
{
}

void PlayerShipIcon::Update(float deltaSeconds)
{
	if (g_theApp->m_attractModeIsOn)
	{
		UpdateThrustFlameVerts();
		UPpdateIconPosition(deltaSeconds);
	}
}

void PlayerShipIcon::Render() const //all called functions must be const
{
	RenderFlame();
	RenderShip();
}

void PlayerShipIcon::RenderFlame() const
{
	Vertex_PCU tempWorldVerts[3];

	int vertIndex = 0;
	int& vi = vertIndex;

	for (vi; vi < 3; ++vi)
	{
		tempWorldVerts[vi] = m_thrustFlameVerts[vi];
	}

	TransformVertexArrayXY3D(3, tempWorldVerts, 1.f, m_orientationDegrees, m_position);
	g_theRenderer->DrawVertexArray(3, tempWorldVerts);
}

void PlayerShipIcon::RenderShip() const
{
	Vertex_PCU tempWorldVerts[PLAYERSHIPICON_NUM_VERT];

	int vertIndex = 0;
	int& vi = vertIndex;

	for (vi; vi < PLAYERSHIPICON_NUM_VERT; ++vi)
	{
		tempWorldVerts[vi] = m_localVerts[vi];
	}

	TransformVertexArrayXY3D(PLAYERSHIPICON_NUM_VERT, tempWorldVerts, 1.f, m_orientationDegrees, m_position);
	g_theRenderer->DrawVertexArray(PLAYERSHIPICON_NUM_VERT, tempWorldVerts);
}

// the shape of the ship
void PlayerShipIcon::InitializeLocalVerts()
{
	// nose
	m_localVerts[0].m_position = Vec3(1.f, 0.f, 0.f) ;
	m_localVerts[1].m_position = Vec3(0.f, 1.f, 0.f) ;
	m_localVerts[2].m_position = Vec3(0.f, -1.f, 0.f);
	//m_localVerts[0].m_color = Rgba8(255, 255, 255, 255);
	//m_localVerts[1].m_color = Rgba8(255, 255, 0, 255);
	//m_localVerts[2].m_color = Rgba8(255, 255, 0, 255);

	// left wing
	m_localVerts[3].m_position = Vec3(2.f, 1.f, 0.f)   ;
	m_localVerts[4].m_position = Vec3(0.f, 2.f, 0.f)   ;
	m_localVerts[5].m_position = Vec3(-2.f, 1.f, 0.f)  ;

	// right wing
	m_localVerts[6].m_position = Vec3(2.f, -1.f, 0.f)  ;
	m_localVerts[7].m_position = Vec3(-2.f, -1.f, 0.f)  ;
	m_localVerts[8].m_position = Vec3(0.f, -2.f, 0.f) ;
	// body (tri 1 of 2)
	m_localVerts[9].m_position = Vec3(0.f, 1.f, 0.f)   ;
	m_localVerts[10].m_position = Vec3(-2.f, -1.f, 0.f);
	m_localVerts[11].m_position = Vec3(0.f, -1.f, 0.f) ;

	// body (tri 2 of 2)
	m_localVerts[12].m_position = Vec3(0.f, 1.f, 0.f)  ;
	m_localVerts[13].m_position = Vec3(-2.f, 1.f, 0.f) ;
	m_localVerts[14].m_position = Vec3(-2.f, -1.f, 0.f);

	// change color according to vertex index
	for (int vertIndex = 0; vertIndex < PLAYERSHIPICON_NUM_VERT; ++vertIndex)
	{
		m_localVerts[vertIndex].m_position *= m_uniformScale;
		m_localVerts[vertIndex].m_color = Rgba8(PLAYERSHIP_COLOR_R, PLAYERSHIP_COLOR_G, PLAYERSHIP_COLOR_B, PLAYERSHIP_COLOR_A);
	}

}

void PlayerShipIcon::UpdateThrustFlameVerts()
{
	m_thrustFlameVerts[0].m_position = Vec3(-1.6f, -1.f, 0.f);
	m_thrustFlameVerts[1].m_position = Vec3(-1.6f, 1.f, 0.f);
	m_thrustFlameVerts[2].m_position = Vec3((-PLAYERSHIP_FLAME_LENGTH_LONG), 0.f, 0.f);

	for (int i = 0; i < 2; ++i)
	{
		m_thrustFlameVerts[i].m_position *= m_uniformScale;
		m_thrustFlameVerts[i].m_color = Rgba8(PLAYERSHIP_FLAME_COLOR_R, PLAYERSHIP_FLAME_COLOR_G, PLAYERSHIP_FLAME_COLOR_B, 255);
	}
	m_thrustFlameVerts[2].m_position *= m_uniformScale;
	m_thrustFlameVerts[2].m_color = Rgba8(PLAYERSHIP_FLAME_COLOR_R, PLAYERSHIP_FLAME_COLOR_G, PLAYERSHIP_FLAME_COLOR_B, 0);


	float flameFlickingScale = g_rng->RollRandomFloatInRange(0.8f, 1.2f);

	m_thrustFlameVerts[2].m_position = Vec3((-PLAYERSHIP_FLAME_LENGTH_LONG * flameFlickingScale * m_uniformScale), 0.f, 0.f);// the x could larger than the 0


	// change vertex color
	for (int i = 0; i < 2; ++i)
	{
		m_thrustFlameVerts[i].m_color = Rgba8(PLAYERSHIP_FLAME_COLOR_R, PLAYERSHIP_FLAME_COLOR_R, PLAYERSHIP_FLAME_COLOR_R, 255);
	}
	m_thrustFlameVerts[2].m_color = Rgba8(PLAYERSHIP_FLAME_COLOR_R, PLAYERSHIP_FLAME_COLOR_G, PLAYERSHIP_FLAME_COLOR_B, 0);
}

void PlayerShipIcon::UPpdateIconPosition(float deltaSeconds)
{
	m_position = m_originalPosition;

	m_age += deltaSeconds;
	//float driftingCycleX = g_rng->RollRandomFloatInRange(0.f, 1.f);
	//float driftingCycleY = g_rng->RollRandomFloatInRange(0.f, 1.f);

	float driftingPosX = static_cast<unsigned char>(RangeMap(sinf(m_age * m_floatingCycle), -1.f, 1.f, 0.f, 20.f));
	float driftingPosY = static_cast<unsigned char>(RangeMap(cosf(m_age * m_floatingCycle), -1.f, 1.f, 0.f, 20.f));

	m_position.x = 0.95f * m_position.x + 0.05f * driftingPosX;
	m_position.y = 0.95f * m_position.y + 0.05f * driftingPosY;
}

