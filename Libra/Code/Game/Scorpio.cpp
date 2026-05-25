#include "Game/Scorpio.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Game/GameCommon.hpp"
#include "Game/App.hpp"
#include "Game/Game.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/core/VertexUtils.hpp"
#include "Game/Map.hpp"
#include "Game/Bullet.hpp"

extern Renderer* g_theRenderer;
extern App* g_theApp;
extern InputSystem* g_theInput;
extern AudioSystem* g_theAudio;
extern RandomNumberGenerator* g_rng;

// the children's constructor function need to initialize its parent class constructor first
Scorpio::Scorpio(Map* owner, EntityType type, EntityFaction faction, Vec2 const& startPos, float angleDegrees)
	:Entity(owner, type, faction, startPos, angleDegrees)
{
	m_physicsRadius =  0.3f;
	m_turretCurrent_OrientDegrees = angleDegrees;
	
	m_isFixedOnGround = true;
	m_isPushedByEntities = false;
	m_doesPushEntities = true;
	m_isPushedByWalls = true;
	m_isHitByBullets = true;

	m_entityType = ENTITY_TYPE_EVIL_SCORPIO;
	m_entityFaction = FACTION_EVIL;

	//InitializeFlameVerts();
	m_health = 3;
	m_maxHealth = 3;
	m_fireRateInterval = SCORPIO_COOLDOWN;

	// start at the tile(1, 1)
	m_position = startPos;

	AddVertsForRender();
}

Scorpio::~Scorpio()
{
}

void Scorpio::Update(float deltaSeconds)
{
	m_age += deltaSeconds;
	m_timer += deltaSeconds;
	m_soundTimer += deltaSeconds;

	UpdateAIStateController();
	UpdateAction(deltaSeconds);
}

void Scorpio::UpdateAIStateController()
{
	m_LastFrame_AIState = m_AIState;

	// if the raycast hit the player, start chasing and Leo need to reach player's pos
	if (m_map->HasLineOfSight(*this))
	{
		m_AIState = ScorpioState::AIMING;
	}
	else m_AIState = ScorpioState::SCANNING;

	// trigger the alarm sound whenever see the player
	if (m_LastFrame_AIState == ScorpioState::SCANNING && m_AIState == ScorpioState::AIMING)
	{
		SoundOffAlarmWhenSpotThePlayerDuringWandering();
	}
}

void Scorpio::UpdateAction(float deltaSeconds)
{
	switch (m_AIState)
	{
	case ScorpioState::SCANNING: { ScanSurroundings(deltaSeconds); } break;
	case ScorpioState::AIMING: 
	{ 
		Rotate_AimAtThePlayer(deltaSeconds); 
		EnemyShootBulletWhenLockOn(this, ENTITY_TYPE_EVIL_BULLET, 5.f, m_iBasis_Turret);
	} break;
	}
}

void Scorpio::ScanSurroundings(float deltaSeconds)
{
	m_turretCurrent_OrientDegrees += SCORPIO_ANGULAR_VELOCITY * deltaSeconds;
	m_iBasis_Turret = m_iBasis_Turret.MakeFromPolarDegrees(m_turretCurrent_OrientDegrees, 1.f);
}

void Scorpio::Rotate_AimAtThePlayer(float deltaSeconds)
{
	Entity* playerTank = m_map->m_entityPtrListByType[ENTITY_TYPE_GOOD_PLAYER][0];
	if (playerTank)
	{
		Vec2 direction = playerTank->m_position - m_position;
		m_turretGoal_OrientDegrees = direction.GetOrientationDegrees();
		float turretMaxDeltaDegrees = SCORPIO_ANGULAR_VELOCITY * deltaSeconds;

		float newTurrentOrientationDegree = GetTurnedTowardDegrees(m_turretCurrent_OrientDegrees, m_turretGoal_OrientDegrees, turretMaxDeltaDegrees);
		m_turretCurrent_OrientDegrees = newTurrentOrientationDegree;
		m_iBasis_Turret = m_iBasis_Turret.MakeFromPolarDegrees(m_turretCurrent_OrientDegrees, 1.f);
	}
}

void Scorpio::Render() const //all called functions must be const
{
	RenderTank();
	RenderLaserAiming();
}

void Scorpio::RenderLaserAiming() const
{
	std::vector<Vertex_PCU> laserVerts;
	laserVerts.reserve(3);

	Vec2 muzzlePos = Vec2(0.5f, 0.f);
	Vec2 jBasis = m_iBasis_Turret.GetRotated90Degrees();
	TransformPosition2D(muzzlePos, m_iBasis_Turret, jBasis, m_position);
	Vec2 laserEndPos;
	// do raycast for the laser and the laser will be block by tiles
	RaycastResult2D laserResult = m_map->RaycastVsTiles(muzzlePos, m_iBasis_Turret, RAYCAST_TOFINDPLAYER_RANGE);
	if (laserResult.m_didImpact)
	{
		laserEndPos = laserResult.m_impactPos;
	}
	else
	{
		laserEndPos = muzzlePos + m_iBasis_Turret * RAYCAST_TOFINDPLAYER_RANGE;
	}

	AddVertsForTriangle2D(laserVerts, muzzlePos, Rgba8::RED, laserEndPos, Rgba8(255, 0, 0, 0), 0.03f);
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->DrawVertexArray((int)laserVerts.size(), laserVerts.data());
}

void Scorpio::RenderTank() const
{
	if (m_isDead || m_isGarbage)
	{
		return;
	}

	std::vector<Vertex_PCU> tankBodyVerts;
	tankBodyVerts.resize(6);
	for (int bodyIndex = 0; bodyIndex < (int)tankBodyVerts.size(); ++bodyIndex)
	{
		tankBodyVerts[bodyIndex] = m_tankBody_LocalVerts[bodyIndex];
	}
	TransformVertexArrayXY3D(tankBodyVerts, m_iBasis_Body, m_position);

	std::vector<Vertex_PCU> tankTurretVerts;
	tankTurretVerts.resize(6);
	for (int turretIndex = 0; turretIndex < (int)tankTurretVerts.size(); ++turretIndex)
	{
		tankTurretVerts[turretIndex] = m_tankTurret_LocalVerts[turretIndex];
	}
	TransformVertexArrayXY3D(tankTurretVerts, m_iBasis_Turret, m_position);

	g_theRenderer->BindTexture(g_theApp->g_textureID[SCORPIO_BASE]);
	g_theRenderer->DrawVertexArray((int)tankBodyVerts.size(), tankBodyVerts.data());
	g_theRenderer->BindTexture(g_theApp->g_textureID[SCORPIO_TURRET]);
	g_theRenderer->DrawVertexArray((int)tankTurretVerts.size(), tankTurretVerts.data());
}

// draw debug mode
void Scorpio::DebugRender() const
{
	DrawXYAxis_ForDebug(m_iBasis_Body, m_cosmeticRadius, DEBUGLINE_THICKNESS);
	DrawPhysicsCollisionRing_ForDebug(DEBUGRING_THICKNESS);
	DrawCosmeticCollisionRing_ForDebug(DEBUGRING_THICKNESS);
}

bool Scorpio::IsOffScreen() const
{
	return false;
}

// the shape of the ship
// make the localverts related to iBasis
void Scorpio::AddVertsForRender()
{
	m_tankBody_LocalVerts.reserve(6);
	m_tankTurret_LocalVerts.reserve(6);
	
	AABB2 body = AABB2(-.5f, -.5f, .5f, .5f);
	AddVertsForAABB2D(m_tankBody_LocalVerts, body, Rgba8(255, 255, 255, 255));

	AABB2 turret = AABB2(-.5f, -.5f, .5f, .5f);
	AddVertsForAABB2D(m_tankTurret_LocalVerts, turret, Rgba8(255, 255, 255, 255));
}
