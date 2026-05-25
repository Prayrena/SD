#include "Game/Leo.hpp"
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
Leo::Leo(Map* owner, EntityType type, EntityFaction faction, Vec2 const& startPos, float angleDegrees)
	:Entity(owner, type, faction, startPos, angleDegrees)
{
	m_map = owner;

	m_physicsRadius =  0.35f;

	m_isFixedOnGround = false;
	m_isPushedByEntities = true;
	m_doesPushEntities = true;
	m_isPushedByWalls = true;
	m_isHitByBullets = true;

	// Property settings
	m_entityFaction = FACTION_EVIL;
	m_entityType = ENTITY_TYPE_EVIL_LEO;			
	
	m_health = PLAYER_HEALTH;
	m_maxHealth = PLAYER_HEALTH;
	m_fireRateInterval = LEO_COOLDOWN;

	m_angularVelocity = g_gameConfigBlackboard.GetValue("leoAngularSpeed", 999.f);
	m_linearVelocity = g_gameConfigBlackboard.GetValue("leoLinearSpeed", 999.f);
	m_position = startPos;
	m_bodyCurrent_OrientDegrees = angleDegrees;

	AddVertsForRender();

	// generate a heat map to see if the exit tile heat value could be modified
	m_heatMap = new TileHeatMap(m_map->m_dimensions);
}

Leo::~Leo()
{
}

void Leo::Update(float deltaSeconds)
{
	m_age += deltaSeconds;
	m_timer += deltaSeconds;
	m_soundTimer += deltaSeconds;
	m_normal = m_iBasis_Body;

	UpdateAIStateController();
	UpdateAction(deltaSeconds);
}

/// <AI>
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Leo::UpdateAIStateController()
{
	m_LastFrame_AIState = m_AIState;

	// if the raycast hit the player, start chasing and Leo need to reach player's pos
	if (m_map->HasLineOfSight(*this))
	{
		m_AIState = LeoAIState::CHASING;
		m_playerLastSeenPosIsReached = false;
		m_playerSpoted = true;
	}
	else
	{
		m_playerSpoted = false;
		// if player's pos has not been reached, checking out the player last been seen place
		if (!m_playerLastSeenPosIsReached)
		{
			m_AIState = LeoAIState::CHASING;
		}
		else m_AIState = LeoAIState::WANDERING;
	}

	// trigger the alarm sound whenever see the player
	if (m_LastFrame_AIState == LeoAIState::WANDERING && m_AIState == LeoAIState::CHASING)
	{
		SoundOffAlarmWhenSpotThePlayerDuringWandering();
	}
}

void Leo::UpdateAction(float deltaSeconds)
{
	switch (m_AIState)
	{
	case LeoAIState::WANDERING: {WanderOnMap(deltaSeconds); } break;
	case LeoAIState::CHASING: 
	{
		ChasingPlayer(deltaSeconds); 
		if (m_playerSpoted)
		{
			EnemyShootBulletWhenLockOn(this, ENTITY_TYPE_EVIL_BULLET, 5.f, m_iBasis_Body);
		}
	} break;
	}
}

void Leo::ChasingPlayer(float deltaSeconds)
{
	//m_playerLastSeenPosIsReached = false;
	//Entity* playerRef = m_map->m_entityPtrListByType[ENTITY_TYPE_GOOD_PLAYER][0];
	//// if the raycast is not blocked by the tiles, update the destination for the Leo
	//if (!m_raycastResult.m_didImpact && playerRef)
	//{
	//	m_playerLastKnownPos = playerRef->m_position;
	//}
	//Vec2 disp = m_playerLastKnownPos - m_position;
	//m_bodyGoal_OrientDegrees = disp.GetOrientationDegrees();

	//float body_MaxDeltaDegrees = LEO_ANGULAR_VELOCITY * deltaSeconds;
	//m_bodyCurrent_OrientDegrees = GetTurnedTowardDegrees(m_bodyCurrent_OrientDegrees, m_bodyGoal_OrientDegrees, body_MaxDeltaDegrees);

	//// get the new iBasis for body and turret
	//m_iBasis_Body = m_iBasis_Body.MakeFromPolarDegrees(m_bodyCurrent_OrientDegrees, 1.f);

	//// Driving
	//// Get new translation and position
	//// in case the tank is moving while rotating, we limit its linear speed until the target is within 90 degrees
	//m_velocity = m_velocity.MakeFromPolarDegrees(m_bodyCurrent_OrientDegrees, m_linearVelocity);
	//m_position += m_velocity * deltaSeconds;

	////----------------------------------------------------------------------------------------------------------------------------------------------------
	//// if Leo has reach the last seen player location, switch to wandering mode
	//if (disp.GetLength() < m_physicsRadius)
	//{
	//	m_playerLastSeenPosIsReached = true;
	//	m_AIState = LeoAIState::WANDERING;

	//	// when switching back to wandering, reset the goal
	//	m_wayPointHasReached = false;
	//	m_shortTermGoalHasReached = false;
	//}
	GenerateHeatMapAndChasePlayer(45.f, 0.5f, deltaSeconds);
}

void Leo::WanderOnMap(float deltaSeconds)
{
	GenerateHeatMapAndWander( 45.f, 0.5f, deltaSeconds);
}

/// <Render>
/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Leo::Render() const //all called functions must be const
{
	RenderTank();
}

void Leo::RenderTank() const
{
	if (!isAlive())
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

	g_theRenderer->BindTexture(g_theApp->g_textureID[T_LEO]);
	g_theRenderer->DrawVertexArray((int)tankBodyVerts.size(), tankBodyVerts.data());
}

// draw debug mode
void Leo::DebugRender() const
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

	// highlight the short term pos as well as the way point pos
	if (this == m_map->m_entityPtrListByType[ENTITY_TYPE_EVIL_LEO][0])
	{
		Vec2 direction = Vec2((float)m_shortTermTileCoords.x, (float)m_shortTermTileCoords.y) + Vec2(0.5f, 0.5f);
		Vec2 wayPointPos = Vec2( (float)m_nextWayPointTileCoords.x, (float)m_nextWayPointTileCoords.y ) + Vec2(0.5f, 0.5f);
		std::vector<Vertex_PCU> verts;
		AddVertesForDisc2D(verts, direction, 0.2f, Rgba8::YELLOW, 12);
		AddVertesForDisc2D(verts, wayPointPos, 0.2f, Rgba8::RED, 12);
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->DrawVertexArray((int)verts.size(), verts.data());
	}

	// show entity chasing player direction
	if (!m_playerLastSeenPosIsReached)
	{
		DrawEnemyChasingDirection_ForDebug();
	}
}

bool Leo::IsOffScreen() const
{
	return false;
}

// the shape of the ship
// make the localverts related to iBasis
void Leo::AddVertsForRender()
{
	m_localVerts.reserve(6);
	
	AABB2 body = AABB2(-.5f, -.5f, .5f, .5f);
	AddVertsForAABB2D(m_localVerts, body, Rgba8(255, 255, 255, 255));
}
