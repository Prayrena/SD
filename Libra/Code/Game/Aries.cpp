#include "Game/Aries.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Game/GameCommon.hpp"
#include "Game/App.hpp"
#include "Game/Game.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/core/VertexUtils.hpp"
#include "Game/Bullet.hpp"
#include "Game/Map.hpp"

extern Renderer* g_theRenderer;
extern App* g_theApp;
extern InputSystem* g_theInput;
extern AudioSystem* g_theAudio;
extern RandomNumberGenerator* g_rng;

// the children's constructor function need to initialize its parent class constructor first
Aries::Aries(Map* owner, EntityType type, EntityFaction faction, Vec2 const& startPos, float angleDegrees)
	:Entity(owner, type, faction, startPos, angleDegrees)
{
	m_physicsRadius =  0.35f;
	m_isFixedOnGround = false;
	m_isPushedByEntities = true;
	m_doesPushEntities = true;
	m_isPushedByWalls = true;
	m_isHitByBullets = true;

	m_bodyCurrent_OrientDegrees = angleDegrees;

	m_entityType = ENTITY_TYPE_EVIL_ARIES;
	m_entityFaction = FACTION_EVIL;

	m_health = ARIES_HEALTH;
	m_maxHealth = ARIES_HEALTH;
	m_angularVelocity = g_gameConfigBlackboard.GetValue("ariesAngularSpeed", 999.f);
	m_linearVelocity = g_gameConfigBlackboard.GetValue("ariesLinearSpeed", 999.f);

	// start at the tile(1, 1)
	m_position = startPos;

	AddVertsForRender();
}

Aries::~Aries()
{
}

void Aries::Update(float deltaSeconds)
{
	m_age += deltaSeconds;
	m_normal = m_iBasis_Body;
	m_soundTimer += deltaSeconds;

	UpdateAIStateController();
	UpdateAction(deltaSeconds);
}

void Aries::UpdateAIStateController()
{
	m_LastFrame_AIState = m_AIState;

	// if the raycast hit the player, start chasing and Leo need to reach player's pos
	if (m_map->HasLineOfSight(*this))
	{
		m_AIState = AriesAIState::CHASING;
		m_playerLastSeenPosIsReached = false;
		m_playerSpoted = true;
	}
	else
	{
		m_playerSpoted = false;
		// if player's pos has not been reached, checking out the player last been seen place
		if (!m_playerLastSeenPosIsReached)
		{
			m_AIState = AriesAIState::CHASING;
		}
		else
		{
			m_AIState = AriesAIState::WANDERING;
		}
	}

	// trigger the alarm sound whenever see the player
	if (m_LastFrame_AIState == AriesAIState::WANDERING && m_AIState == AriesAIState::CHASING)
	{
		SoundOffAlarmWhenSpotThePlayerDuringWandering();
	}
}

void Aries::UpdateAction(float deltaSeconds)
{
	switch (m_AIState)
	{
	case AriesAIState::WANDERING: { WanderOnMap(deltaSeconds); } break;
	case AriesAIState::CHASING: { ChasingPlayer(deltaSeconds); } break;
	}
}

void Aries::WanderOnMap(float deltaSeconds)
{
	GenerateHeatMapAndWander(30.f, 0.2f, deltaSeconds);
}

void Aries::ChasingPlayer(float deltaSeconds)
{
	GenerateHeatMapAndChasePlayer(30.f, 0.2f, deltaSeconds);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// <render>
void Aries::Render() const
{
	RenderTank();
	//RenderShield();
}

void Aries::RenderTank() const
{
	if (m_isDead || m_isGarbage)
	{
		return;
	}

	std::vector<Vertex_PCU> tankBodyVerts;
	tankBodyVerts.resize(6);
	for (int bodyIndex = 0; bodyIndex < (int)tankBodyVerts.size(); ++bodyIndex)
	{
		tankBodyVerts[bodyIndex] = m_localVerts[bodyIndex];
	}
	TransformVertexArrayXY3D(tankBodyVerts, m_iBasis_Body, m_position);

	g_theRenderer->BindTexture(g_theApp->g_textureID[T_ARIES]);
	g_theRenderer->DrawVertexArray((int)tankBodyVerts.size(), tankBodyVerts.data());
}

void Aries::RenderShield()
{
	// todo: draw 90 degrees shield
	// AddVertsForAABB2D()
}

void Aries::ReactToBulletHit(Bullet& bullet)
{
	Vec2 fwdNormal = GetForwardNormal();
	Vec2 dispToBullet = bullet.m_position - m_position;
	float degree = GetAngleDegreesBetweenVectors2D(fwdNormal, dispToBullet);
  	if (degree < 45.f)
	{
		// deflect the bullet
		PushDiscOutOfFixedDisc2D(bullet.m_position, bullet.m_physicsRadius, m_position, m_physicsRadius);
		dispToBullet = dispToBullet.GetNormalized();
		bullet.BouceOff(dispToBullet);
	}
	else
	{
		bullet.m_isGarbage = true;
		TakeDamage(1);
		PlayEnemyHitSound();
	}
}

// draw debug mode
void Aries::DebugRender() const
{
	DrawXYAxis_ForDebug(m_iBasis_Body, m_cosmeticRadius, DEBUGLINE_THICKNESS);
	DrawEntityVelocity_ForDebug(m_velocity, DEBUGLINE_THICKNESS);
	DrawPhysicsCollisionRing_ForDebug(DEBUGRING_THICKNESS);
	DrawCosmeticCollisionRing_ForDebug(DEBUGRING_THICKNESS);

	// body orientation debug line
	Vec2 debugBodyOrientGoal_Start = Vec2(PLAYERTANK_COSMETIC_RADIUS, 0.f);
	Vec2 debugBodyOrientGoal_End = debugBodyOrientGoal_Start + Vec2(0.5f, 0.f);
	debugBodyOrientGoal_Start.SetOrientationDegrees(m_bodyGoal_OrientDegrees);
	debugBodyOrientGoal_End.SetOrientationDegrees(m_bodyGoal_OrientDegrees);
	debugBodyOrientGoal_Start += m_position;
	debugBodyOrientGoal_End += m_position;
	g_theRenderer->BindTexture(nullptr);
	DebugDrawLine(debugBodyOrientGoal_Start, debugBodyOrientGoal_End, DEBUGLINE_THICKNESS * 3.f, Rgba8(255, 0, 0,200));

	// show enity chasing player direction
	if (!m_playerLastSeenPosIsReached)
	{
		DrawEnemyChasingDirection_ForDebug();
	}
}

bool Aries::IsOffScreen() const
{
	return false;
}

// the shape of the ship
// make the localverts related to iBasis
void Aries::AddVertsForRender()
{
	m_localVerts.reserve(6);
	
	AABB2 body = AABB2(-.5f, -.5f, .5f, .5f);
	AddVertsForAABB2D(m_localVerts, body, Rgba8(255, 255, 255, 255));
}
