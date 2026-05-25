#include "Game/Bullet.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Game/GameCommon.hpp"
#include "Game/PlayerShip.hpp"
#include "Game/Game.hpp"
#include "Game/App.hpp"


extern Renderer* g_theRenderer;
extern AudioSystem* g_theAudio;
extern App* g_theApp;

//extern App* g_theApp;
//extern Game* g_theGame;

// the children's constructor function need to initialize its parent class constructor first
Bullet::Bullet(Game* owner, Vec2 const& startPos)
	:Entity(owner, startPos)// constructing the entity before the constructing the bullet
{
	m_physicsRadius = BULLET_PHYSICS_RADIUS;
	m_cosmeticRadius = BULLET_COSMETIC_RADIUS;
	m_isProjectile = true;
	InitializeLocalVerts();
	m_health = BULLET_HEALTH;

	// prepare the sound 
    m_bulletSound = g_theAudio->StartSound(g_theApp->g_soundEffectsID[SHOOTINGBULLETS], false, 0.3f, 0.f, 0.6f, false);
}

Bullet::~Bullet()
{
}

void Bullet::Update(float deltaSeconds)
{
	Vec2 V;
	// m_game-> // this is the owner of this bullet
	//m_orientationDegrees = g_theGame->m_PlayerShip->m_orientationDegrees;
	m_position += m_velocity * deltaSeconds;// bullet only has translation, no rotation, no acceleration

	if (IsOffScreen())
	{
		m_isGarbage = true;
	}

 	float movingBalance = RangeMap(this->m_position.x, 0.f, 200.f, -1.f, 1.f);
 	g_theAudio->SetSoundPlaybackBalance(m_bulletSound, movingBalance);
}

void Bullet::TakeDamage()
{
	if (m_health > 0)
	{
		m_health -= 1;
	}

	if (m_health == 0)
	{
		m_isDead = true;
		m_isGarbage = true;
		m_game->SpawnDebris(this, 180.f, BULLET_DEBRIS_MIN_NUM, BULLET_DEBRIS_MAX_NUM, Rgba8(BULLET_DEBRIS_COLOR_R, BULLET_DEBRIS_COLOR_G, BULLET_DEBRIS_COLOR_B, DEBRIS_COLOR_A));
	}
	// play hitted sound
}

void Bullet::Render() const
{
	// if the current asteroid is define as dead
	// No need to render
	if (m_isDead || m_isGarbage)
		return;

	RenderBullet();

}

void Bullet::RenderBullet() const
{
	// create a new temp array and copy all vertexes from m_localVerts
	Vertex_PCU tempWorldVerts[BULLET_NUM_VERTS];

	int vertIndex = 0;
	int& vi = vertIndex;

	for (vi; vi < BULLET_NUM_VERTS; ++vi)
	{
		tempWorldVerts[vi] = m_localVerts[vi];
	}

	// transform the vertexes from local space to world space
	TransformVertexArrayXY3D(BULLET_NUM_VERTS, tempWorldVerts, 1.f, m_orientationDegrees, m_position);
	g_theRenderer->DrawVertexArray(BULLET_NUM_VERTS, tempWorldVerts);

}

void Bullet::DebugRender() const
{
	// Y axis
	Vec2 debugLineEnd_Y = Vec2(1.f, 0.f);
	debugLineEnd_Y.SetOrientationDegrees(m_orientationDegrees);
	debugLineEnd_Y.SetLength(BULLET_COSMETIC_RADIUS);
	debugLineEnd_Y += m_position;
	DebugDrawLine(m_position, debugLineEnd_Y, DEBUGLINE_THICKNESS, Rgba8(DEBUGLINE_AXISY_COLOR_R, DEBUGLINE_AXISY_COLOR_G, DEBUGLINE_AXISY_COLOR_B, DEBUGLINE_AXISY_COLOR_A));
	// X axis
	Vec2 debugLineEnd_X = Vec2(0.f, 1.f);
	debugLineEnd_X.SetOrientationDegrees(m_orientationDegrees + 90.f);
	debugLineEnd_X.SetLength(BULLET_COSMETIC_RADIUS);
	debugLineEnd_X += m_position;
	DebugDrawLine(m_position, debugLineEnd_X, DEBUGLINE_THICKNESS, Rgba8(DEBUGLINE_AXISX_COLOR_R, DEBUGLINE_AXISX_COLOR_G, DEBUGLINE_AXISX_COLOR_B, DEBUGLINE_AXISX_COLOR_A));
	// collision debug ring
	DebugDrawRing(m_position, BULLET_COSMETIC_RADIUS, DEBUGRING_THICKNESS, Rgba8(DEBUGRING_COSMETIC_COLOR_R, DEBUGRING_COSMETIC_COLOR_G, DEBUGRING_COSMETIC_COLOR_B, DEBUGRING_COSMETIC_COLOR_A));
	DebugDrawRing(m_position, BULLET_PHYSICS_RADIUS, DEBUGRING_THICKNESS, Rgba8(DEBUGRING_PHYSICS_COLOR_R, DEBUGRING_PHYSICS_COLOR_G, DEBUGRING_PHYSICS_COLOR_B, DEBUGRING_PHYSICS_COLOR_A));
	// velocity debug line
	Vec2 debugLineEnd_Vel = m_position + m_velocity; // m_velocity is how far we move in a second
	DebugDrawLine(m_position, debugLineEnd_Vel, DEBUGLINE_THICKNESS, Rgba8(DEBUGLINE_VELOCITY_COLOR_R, DEBUGLINE_VELOCITY_COLOR_G, DEBUGLINE_VELOCITY_COLOR_B, DEBUGLINE_VELOCITY_COLOR_A));
}

bool Bullet::IsOffScreen() const
{
	// if the bullet is completely could not be seen over the window, return true
	if (m_position.x - BULLET_PHYSICS_RADIUS > WORLD_SIZE_X ||
		m_position.x + BULLET_PHYSICS_RADIUS < 0.f)
	{
		return true;
	}

	if (m_position.y - BULLET_PHYSICS_RADIUS > WORLD_SIZE_Y ||
		m_position.y + BULLET_PHYSICS_RADIUS < 0.f)
	{
		return true;
	}

	else
	{
		return false;
	}

}

void Bullet::InitializeLocalVerts()
{
	// the shape of bullet's two tri in local space
	// nose
	m_localVerts[0].m_position = Vec3(.5f, 0.f, 0.f);
	m_localVerts[1].m_position = Vec3(0.f, .5f, 0.f);
	m_localVerts[2].m_position = Vec3(0.f, -.5f, 0.f);
	m_localVerts[0].m_color = Rgba8(255, 255, 0, 255);
	m_localVerts[1].m_color = Rgba8(255, 255, 0, 255);
	m_localVerts[2].m_color = Rgba8(255, 255, 0, 255);
	// trail
	m_localVerts[3].m_position = Vec3(0.f, .5f, 0.f);
	m_localVerts[4].m_position = Vec3(-2.f, 0.f, 0.f);
	m_localVerts[5].m_position = Vec3(0.f, -0.5f, 0.f);
	m_localVerts[3].m_color = Rgba8(255, 0, 0, 255);
	m_localVerts[4].m_color = Rgba8(255, 0, 0, 0);
	m_localVerts[5].m_color = Rgba8(255, 0, 0, 255);

}

