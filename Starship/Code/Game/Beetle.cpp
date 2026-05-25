#include "Game/Beetle.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Game/GameCommon.hpp"
#include "Game/PlayerShip.hpp"
#include "Game/App.hpp"
#include "Game/Game.hpp"


extern Renderer* g_theRenderer;
extern App* g_theApp;
extern PlayerShip* g_playerShip;

// the children's constructor function need to initialize its parent class constructor first
Beetle::Beetle(Game* owner, Vec2 const& startPos)
	:Entity(owner, startPos)// constructing the entity before the constructing the bullet
{
	m_physicsRadius = PLAYERSHIP_PHYSICS_RADIUS;
	m_cosmeticRadius = PLAYERSHIP_COSMETIC_RADIUS;
	InitializeLocalVerts();
	m_health = BEETLE_HEALTH;
}

Beetle::~Beetle()
{
}

void Beetle::Update(float deltaSeconds)
{
	// Beetle has a fixed speed
	float fixedSpeed = BEETLE_SPEED * deltaSeconds;// units/second*frame

	// Beetle is always facing towards the player ship
	//Vec2 dispToPlayership = g_playerShip->m_position - m_position;// incomplete class means concluded class
	Vec2 dispToPlayership;
	if (!m_game->m_playerShip->m_isDead)
	{
		dispToPlayership = m_game->m_playerShip->m_position - m_position;
		m_orientationDegrees = dispToPlayership.GetOrientationDegrees();// when player is dead, the orientation degree do not change
	}

	m_velocity = deltaVelocity.MakeFromPolarDegrees(m_orientationDegrees, fixedSpeed);// acceleration is the pointing to the orientation degrees
	 
	// recalculate player ship's position based on velocity
	m_position += m_velocity;

	if (m_isDead)
	{
	}
}

void Beetle::TakeDamage()
{
	if (m_health > 0)
	{
		m_health -= 1;
	}

	if (m_health == 0)
	{
		m_isDead = true;
		m_isGarbage = true;
		m_game->TurnCameraShakeOnForExplosion();
		m_game->SpawnDebris(this, 0.f, ENTITY_DEBRIS_MIN_NUM, ENTITY_DEBRIS_MAX_NUM, Rgba8(BEETLE_COLOR_R, BEETLE_COLOR_G, BEETLE_COLOR_B, DEBRIS_COLOR_A));
	}

	// play crash sound
}

void Beetle::Render() const //all called functions must be const
{
	RenderShip();
}


void Beetle::RenderShip() const
{
	// is current ship is destroyed, do not render
	if (m_isDead || m_isGarbage)
	{
		return;
	}

	Vertex_PCU tempWorldVerts[BEETLE_NUM_VERT];

	int vertIndex = 0;
	int& vi = vertIndex;

	for (vi; vi < BEETLE_NUM_VERT; ++vi)
	{
		tempWorldVerts[vi] = m_localVerts[vi];
	}

	TransformVertexArrayXY3D(BEETLE_NUM_VERT, tempWorldVerts, 1.f, m_orientationDegrees, m_position);
	g_theRenderer->DrawVertexArray(BEETLE_NUM_VERT, tempWorldVerts);
}

// draw debug mode
void Beetle::DebugRender() const
{
	// Y axis
	Vec2 debugLineEnd_Y = Vec2(1.f, 0.f);
	debugLineEnd_Y.SetOrientationDegrees(m_orientationDegrees);
	debugLineEnd_Y.SetLength(PLAYERSHIP_COSMETIC_RADIUS);
	debugLineEnd_Y += m_position;
	DebugDrawLine(m_position, debugLineEnd_Y, DEBUGLINE_THICKNESS, Rgba8(DEBUGLINE_AXISY_COLOR_R, DEBUGLINE_AXISY_COLOR_G, DEBUGLINE_AXISY_COLOR_B, DEBUGLINE_AXISY_COLOR_A));
	// X axis
	Vec2 debugLineEnd_X = Vec2(0.f, 1.f);
	debugLineEnd_X.SetOrientationDegrees(m_orientationDegrees + 90.f); 
	debugLineEnd_X.SetLength(PLAYERSHIP_COSMETIC_RADIUS);
	debugLineEnd_X += m_position;
	DebugDrawLine(m_position, debugLineEnd_X, DEBUGLINE_THICKNESS, Rgba8(DEBUGLINE_AXISX_COLOR_R, DEBUGLINE_AXISX_COLOR_G, DEBUGLINE_AXISX_COLOR_B, DEBUGLINE_AXISX_COLOR_A));
	// collision debug ring
	DebugDrawRing(m_position, PLAYERSHIP_COSMETIC_RADIUS, DEBUGRING_THICKNESS, Rgba8(DEBUGRING_COSMETIC_COLOR_R, DEBUGRING_COSMETIC_COLOR_G, DEBUGRING_COSMETIC_COLOR_B, DEBUGRING_COSMETIC_COLOR_A));
	DebugDrawRing(m_position, PLAYERSHIP_PHYSICS_RADIUS, DEBUGRING_THICKNESS, Rgba8(DEBUGRING_PHYSICS_COLOR_R, DEBUGRING_PHYSICS_COLOR_G, DEBUGRING_PHYSICS_COLOR_B, DEBUGRING_PHYSICS_COLOR_A));
	// velocity debug line
	Vec2 debugLineEnd_Vel = m_position + m_velocity; // m_velocity is how far we move in a second
	DebugDrawLine(m_position, debugLineEnd_Vel, DEBUGLINE_THICKNESS, Rgba8(DEBUGLINE_VELOCITY_COLOR_R, DEBUGLINE_VELOCITY_COLOR_G, DEBUGLINE_VELOCITY_COLOR_B, DEBUGLINE_VELOCITY_COLOR_A));
}

bool Beetle::IsOffScreen() const
{
	return false;
}

// the shape of the ship
void Beetle::InitializeLocalVerts()
{
	// nose
	m_localVerts[0].m_position = Vec3(1.5f, 1.5f, 0.f);
	m_localVerts[1].m_position = Vec3(0.5f, 1.5f, 0.f);
	m_localVerts[2].m_position = Vec3(1.5f, -1.5f, 0.f);
	
	// left wing
	m_localVerts[3].m_position = Vec3(0.f, 1.5f, 0.f);
	m_localVerts[4].m_position = Vec3(-1.5f, 1.5f, 0.f);
	m_localVerts[5].m_position = Vec3(-1.5f, 1.f, 0.f);

	// right wing
	m_localVerts[6].m_position = Vec3(0.f, 1.5f, 0.f);
	m_localVerts[7].m_position = Vec3(-1.5f, 1.f, 0.f);
	m_localVerts[8].m_position = Vec3(.5f, 1.f, 0.f);
	// body (tri 1 of 2)
	m_localVerts[9].m_position  = Vec3(-1.f, 1.f, 0.f);
	m_localVerts[10].m_position = Vec3(-1.f, 0.5f, 0.f);
	m_localVerts[11].m_position = Vec3(.5f, 1.f, 0.f);

	// tail body
	m_localVerts[12].m_position = Vec3(.5f, 0.f, 0.f);
	m_localVerts[13].m_position = Vec3(-2.f, 1.f, 0.f);
	m_localVerts[14].m_position = Vec3(-2.f, -1.f, 0.f);

	m_localVerts[15].m_position = Vec3(-1.f, -.5f, 0.f);
	m_localVerts[16].m_position = Vec3(.5f, -1.f, 0.f);
	m_localVerts[17].m_position = Vec3(.5f, 0.f, 0.f);

	m_localVerts[18].m_position = Vec3(-1.f, -.5f, 0.f);
	m_localVerts[19].m_position = Vec3(-1.f, -1.f, 0.f);
	m_localVerts[20].m_position = Vec3(0.5f, -1.f, 0.f);

	m_localVerts[21].m_position = Vec3(.5f, 1.f, 0.f);
	m_localVerts[22].m_position = Vec3(-1.f, .5f, 0.f);
	m_localVerts[23].m_position = Vec3(.5f, 0.f, 0.f);

	m_localVerts[24].m_position = Vec3(-1.5f, -1.f, 0.f);
	m_localVerts[25].m_position = Vec3(0.f, -1.5f, 0.f);
	m_localVerts[26].m_position = Vec3(.5f, -1.f, 0.f);

	m_localVerts[27].m_position = Vec3(-1.5f, -1.f, 0.f);
	m_localVerts[28].m_position = Vec3(-1.5f, -1.5f, 0.f);
	m_localVerts[29].m_position = Vec3(0.f, -1.5f, 0.f);

	m_localVerts[30].m_position = Vec3(.5f, 1.5f, 0.f);
	m_localVerts[31].m_position = Vec3(.5f, -1.5f, 0.f);
	m_localVerts[32].m_position = Vec3(1.5f, -1.5f, 0.f);


	// change color according to vertex index
	for (int vertIndex = 0; vertIndex < BEETLE_NUM_VERT; ++vertIndex)
	{
		m_localVerts[vertIndex].m_color = Rgba8(BEETLE_COLOR_R, BEETLE_COLOR_G, BEETLE_COLOR_B, 255);
	}
}
