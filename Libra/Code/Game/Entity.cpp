#include "Game/Entity.hpp"
#include "Game/Bullet.hpp"
#include "Game/Game.hpp"
#include "Game/Map.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Game/PlayerTank.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Game/App.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"

extern Game* g_theGame;
extern AudioSystem* g_theAudio;
extern App* g_theApp;
extern Renderer* g_theRenderer;
extern RandomNumberGenerator* g_rng;

Entity::Entity(Map* owner, EntityType entityType, EntityFaction faction, Vec2 const& startPos, float orientationDegrees)
	:m_map(owner), m_entityType(entityType), m_entityFaction(faction), m_position(startPos), m_orientationDegrees(orientationDegrees)
{
	m_heatMap = new TileHeatMap(m_map->m_dimensions);
}

Entity::~Entity()
{
}

//void Entity::TakeDamage()
//{
//	if (m_health > 0)
//	{
//		m_health -= 1;
//	}
//
//	if (m_health == 0)
//	{
//		m_isDead = true;
//		m_isGarbage = true;
//	}
//
//	// play crash sound
//}

void Entity::DebugRender() const
{

}

bool Entity::IsOffScreen() const
{
	return false;
}

void Entity::TakeDamage(int damage)
{
	if (isAlive())
	{
		m_health -= damage;
	}
	if (m_health < 1)
	{
		m_isDead = true;
		Die();
	}
}

void Entity::Die()
{
	if (m_isProjectile)
	{
		float orientationDegrees = g_rng->RollRandomFloatInRange(0.f, 360.f);
		g_theGame->m_currentMap->SpawnNewEntity(ENTITY_TYPE_BULLET_EXPLOSION, m_position, orientationDegrees);
		g_theAudio->StartSound(g_theApp->g_soundEffectsID[BULLET_EXPLOSION], false, 2.0f, 0.f, 1.f, false);
	}
	else
	{
		float orientationDegrees = g_rng->RollRandomFloatInRange(0.f, 360.f);
		g_theGame->m_currentMap->SpawnNewEntity(ENTITY_TYPE_TANK_EXPLOSION, m_position, orientationDegrees);
		g_theAudio->StartSound(g_theApp->g_soundEffectsID[ENEMY_DEATH], false, 2.0f, 0.f, 1.f, false);

		// if the player is dead, there is no sound for gaining the points
		if (m_entityType == ENTITY_TYPE_GOOD_PLAYER)
		{
			return;
		}
		else
		{
			g_theAudio->StartSound(g_theApp->g_soundEffectsID[POINTS_GAINED], false, 2.0f, 0.f, 1.f, false);
		}
	}
}

bool Entity::isAlive() const
{
	if (!m_isDead && !m_isGarbage)
	{
		if (m_health > 0)
		{
			return true;
		}
		else return false;
	}
	else return false;
}

void Entity::ReactToBulletHit(Bullet& bullet)
{
	if (isAlive())
	{
		// when the bullet touches other entities other than aries, the bullet die
		// bullet could bounce off on aries
		TakeDamage(1);
		PlayEnemyHitSound();
		bullet.m_isGarbage = true;
	}
}
//----------------------------------------------------------------------------------------------------------------------------------------------------
void Entity::PlayEnemyShootSound()
{
	g_theAudio->StartSound(g_theApp->g_soundEffectsID[ENEMY_SHOOT], false, 1.0f, 0.f, 1.f, false);
}

void Entity::PlayEnemyHitSound()
{
	g_theAudio->StartSound(g_theApp->g_soundEffectsID[ENEMY_HIT], false, 1.0f, 0.f, 1.f, false);
}

void Entity::PlayEnemyDeathSound()
{
	g_theAudio->StartSound(g_theApp->g_soundEffectsID[ENEMY_DEATH], false, 1.0f, 0.f, 1.f, false);
}
//----------------------------------------------------------------------------------------------------------------------------------------------------
Vec2 Entity::GetForwardNormal() const
{
	return m_normal;
}

void Entity::DrawXYAxis_ForDebug(Vec2 iBasis, float length, float debugLineThickness) const
{
	// body X axis
	Vec2 debugAxisLineEnd_X = Vec2(length, 0.f);
	float orientationDegrees = iBasis.GetOrientationDegrees();
	debugAxisLineEnd_X.SetOrientationDegrees(orientationDegrees);
	debugAxisLineEnd_X += m_position;
	g_theRenderer->BindTexture(nullptr);
	DebugDrawLine(m_position, debugAxisLineEnd_X, debugLineThickness, Rgba8::RED);
	// body Y axis
	Vec2 debugAxisLineEnd_Y = Vec2(0.f, length);
	debugAxisLineEnd_Y.SetOrientationDegrees(orientationDegrees + 90.f);
	debugAxisLineEnd_Y += m_position;
	g_theRenderer->BindTexture(nullptr);
	DebugDrawLine(m_position, debugAxisLineEnd_Y, debugLineThickness, Rgba8::GREEN);
}

void Entity::DrawEntityVelocity_ForDebug(Vec2 velocity, float debugLineThickness) const
{
	Vec2 velocityLineEnd = m_position + velocity;
	g_theRenderer->BindTexture(nullptr);
	DebugDrawLine(m_position, velocityLineEnd, debugLineThickness, Rgba8::YELLOW);
}

void Entity::DrawPhysicsCollisionRing_ForDebug(float debugLineThickness) const
{
	g_theRenderer->BindTexture(nullptr);
	DebugDrawRing(m_position, m_physicsRadius, debugLineThickness, Rgba8::MAGENTA);
}

void Entity::DrawCosmeticCollisionRing_ForDebug(float debugLineThickness) const
{
	g_theRenderer->BindTexture(nullptr);
	DebugDrawRing(m_position, m_cosmeticRadius, debugLineThickness, Rgba8::CYAN);
}

void Entity::DrawEnemyChasingDirection_ForDebug() const
{
	Vec2 headingPos;
	headingPos.x = (float)m_playerLastKnownTileCoords.x;
	headingPos.y = (float)m_playerLastKnownTileCoords.y;
	g_theRenderer->BindTexture(nullptr);
	DebugDrawLine(m_position, headingPos, .04f, Rgba8::BLACK_TRANSPARENT);
	DrawDisk(headingPos, 0.05f, Rgba8::BLACK, Rgba8::BLACK);
}

void Entity::GenerateHeatMapForWandering()
{
	IntVec2 startCoords = m_map->GetTileCoordsInMap_For_WorldPos(m_position);
	bool treatWaterAsSolid = true;
	if (m_canSwim)
	{
		treatWaterAsSolid = false;
	}
	m_map->PopulateDistanceField(*m_heatMap, startCoords, 999999.f, treatWaterAsSolid, true);
}

void Entity::GenerateHeatMapForWanderingWayPoint( IntVec2 startCoords )
{
	bool treatWaterAsSolid = true;
	if (m_canSwim)
	{
		treatWaterAsSolid = false;
	}
	m_map->PopulateDistanceField(*m_heatMap, startCoords, 999999.f, treatWaterAsSolid, true);
}

void Entity::GenerateHeatMapFromPlayerSeenLocation()
{
	bool treatWaterAsSolid = true;
	if (m_canSwim)
	{
		treatWaterAsSolid = false;
	}
	IntVec2 playerTileCoords = m_map->GetTileCoordsInMap_For_WorldPos(m_map->m_entityPtrListByType[ENTITY_TYPE_GOOD_PLAYER][0]->m_position);
	m_map->PopulateDistanceField(*m_heatMap, playerTileCoords, 999999.f, treatWaterAsSolid, true);
}

void Entity::GenerateHeatMapAndWander(float speedUpDegreeRange, float lowerSpeedMultiplier, float deltaSeconds )
{
	if (m_wayPointHasReached)
	{
		// if has not reach previous way point, randomly pick a way point on the map that this entity is able to get to //this is the long term goal
		GenerateHeatMapForWandering();
		for (;;)
		{
			m_nextWayPointTileCoords.x = g_rng->RollRandomIntInRange(1, (m_map->m_dimensions.x - 2));
			m_nextWayPointTileCoords.y = g_rng->RollRandomIntInRange(1, (m_map->m_dimensions.y - 2));

			int tileIndex = m_map->GetTileIndex_For_TileCoordinates(m_nextWayPointTileCoords);
			if (m_heatMap->m_values[tileIndex] == 999999.f)
			{
				continue;
			}
			else
			{
				GenerateHeatMapForWanderingWayPoint(m_nextWayPointTileCoords);// construct a new heat map based on the wandering way point
				m_wayPointHasReached = false;
				break;
			}
		}
	}
	//----------------------------------------------------------------------------------------------------------------------------------------------------
	if (m_shortTermGoalHasReached)
	{
		// based on the destination heat value find a short term goal
		for (;;)
		{
			IntVec2 currentTileCoords = m_map->GetTileCoordsInMap_For_WorldPos(m_position);
			int currentTileIndex = m_map->GetTileIndex_For_TileCoordinates(currentTileCoords);
			// randomly pick from four directions
			int direction = g_rng->RollRandomIntInRange(0, (NUM_CRAWLDIRECTION - 1));
			if (direction == 0) { m_shortTermTileCoords = currentTileCoords + STEP_EAST; }
			else if (direction == 1) { m_shortTermTileCoords = currentTileCoords + STEP_SOUTH; }
			else if (direction == 2) { m_shortTermTileCoords = currentTileCoords + STEP_WEST; }
			else { m_shortTermTileCoords = currentTileCoords + STEP_NORTH; }

			// only pick the nearby tile which has lower heat value
			int shortTermTileIndex = m_map->GetTileIndex_For_TileCoordinates(m_shortTermTileCoords);
			if (m_heatMap->m_values[shortTermTileIndex] < m_heatMap->m_values[currentTileIndex])
			{
				m_shortTermGoalHasReached = false;
				break; // the short term goal is adjacent to current tile
			}
			else continue;
		}
	}
	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// new orientation degrees
	Vec2 direction = Vec2((float)m_shortTermTileCoords.x, (float)m_shortTermTileCoords.y) + Vec2(0.5f, 0.5f);
	Vec2 dispToNewDirection = direction - m_position;
	m_bodyGoal_OrientDegrees = dispToNewDirection.GetOrientationDegrees();

	// new orientation degrees for every frame
	float body_MaxDeltaDegrees = m_angularVelocity * deltaSeconds;
	if (m_bodyCurrent_OrientDegrees != m_bodyGoal_OrientDegrees)
	{
		m_bodyCurrent_OrientDegrees = GetTurnedTowardDegrees(m_bodyCurrent_OrientDegrees, m_bodyGoal_OrientDegrees, body_MaxDeltaDegrees);
	}

	// get the new iBasis for body and turret
	m_iBasis_Body = m_iBasis_Body.MakeFromPolarDegrees(m_bodyCurrent_OrientDegrees, 1.f);

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// check if the entity has got to the way point tile
	// in case the tank is moving while rotating, we limit its linear speed until the target is within 90 degrees

	Vec2 dispToShortTerm = direction - m_position;
	float degree = GetAngleDegreesBetweenVectors2D(m_normal, dispToShortTerm);
	if (degree < speedUpDegreeRange)
	{
		m_linearVelocity = g_gameConfigBlackboard.GetValue("leoLinearSpeed", 999.f);
	}
	else m_linearVelocity = g_gameConfigBlackboard.GetValue("leoLinearSpeed", 999.f) * lowerSpeedMultiplier;

	// Driving
	// Get new translation and position
	m_velocity = m_velocity.MakeFromPolarDegrees(m_bodyCurrent_OrientDegrees, m_linearVelocity);
	m_position += m_velocity * deltaSeconds;

	if (dispToShortTerm.GetLength() < m_physicsRadius)
	{
		m_shortTermGoalHasReached = true;
	}

	Vec2 wayPointPos = Vec2((float)m_nextWayPointTileCoords.x, (float)m_nextWayPointTileCoords.y) + Vec2(0.5f, 0.5f);
	Vec2 dispToWayPoint = wayPointPos - m_position;
	if (dispToWayPoint.GetLength() < m_physicsRadius)
	{
		m_wayPointHasReached = true;
	}
}

void Entity::GenerateHeatMapAndChasePlayer(float speedUpDegreeRange, float lowerSpeedMultiplier, float deltaSeconds)
{
	// when the entity see the player, generate and update the heat map every second for chasing
	if (m_playerSpoted)
	{
		bool treatWaterAsSolid = true;
		if (m_canSwim)
		{
			treatWaterAsSolid = false;
		}
		IntVec2 playerTileCoords = m_map->GetTileCoordsInMap_For_WorldPos(m_map->m_entityPtrListByType[ENTITY_TYPE_GOOD_PLAYER][0]->m_position);
		m_map->PopulateDistanceField(*m_heatMap, playerTileCoords, 999999.f, treatWaterAsSolid, true);
		playerTileCoords = m_map->GetTileCoordsInMap_For_WorldPos(m_map->m_entityPtrListByType[ENTITY_TYPE_GOOD_PLAYER][0]->m_position);

		// if the heat map is newly generated, and the player tile coords is different to last known pos tile coords
		if (m_playerLastKnownTileCoords != playerTileCoords)
		{
			m_shortTermGoalHasReached = true; // means the newly generated tile coords needed to be refreshed
			m_playerLastKnownTileCoords = playerTileCoords;
		}
	}
	m_nextWayPointTileCoords = m_playerLastKnownTileCoords; // for debug

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	if (m_shortTermGoalHasReached)
	{
		// based on the destination heat value find a short term goal
		for (;;)
		{
			IntVec2 currentTileCoords = m_map->GetTileCoordsInMap_For_WorldPos(m_position);
			int currentTileIndex = m_map->GetTileIndex_For_TileCoordinates(currentTileCoords);
			// randomly pick from four directions
			int direction = g_rng->RollRandomIntInRange(0, (NUM_CRAWLDIRECTION - 1));
			if (direction == 0)		 { m_shortTermTileCoords = currentTileCoords + STEP_EAST; }
			else if (direction == 1) { m_shortTermTileCoords = currentTileCoords + STEP_SOUTH; }
			else if (direction == 2) { m_shortTermTileCoords = currentTileCoords + STEP_WEST; }
			else { m_shortTermTileCoords = currentTileCoords + STEP_NORTH; }

			// only pick the nearby tile which has lower heat value
			int shortTermTileIndex = m_map->GetTileIndex_For_TileCoordinates(m_shortTermTileCoords);
			if (m_heatMap->m_values[shortTermTileIndex] < m_heatMap->m_values[currentTileIndex])
			{
				m_shortTermGoalHasReached = false;
				break; // the short term goal is adjacent to current tile
			}
			else if (m_heatMap->m_values[shortTermTileIndex] == 1) // when the entity hit the player, all four neighbor heat tile's heat value become 1 and it is stuck in the infinite loop
			{
				break;
			}
			else continue;
		}
	}
	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// new orientation degrees
	IntVec2 entityOccupiedTileCoords = m_map->GetTileCoordsInMap_For_WorldPos(m_position);
	int entityOccupiedTileIndex = m_map->GetTileIndex_For_TileCoordinates(entityOccupiedTileCoords);
	Vec2 direction;
	if (m_heatMap->m_values[entityOccupiedTileIndex] == 1.f || m_heatMap->m_values[entityOccupiedTileIndex] == 0.f)// if the player is in the nearby tile, then go to the player directly
	{
		direction = m_map->m_entityPtrListByType[ENTITY_TYPE_GOOD_PLAYER][0]->m_position;
	}
	else direction = Vec2((float)m_shortTermTileCoords.x, (float)m_shortTermTileCoords.y) + Vec2(0.5f, 0.5f);
	Vec2 dispToNewDirection = direction - m_position;
	m_bodyGoal_OrientDegrees = dispToNewDirection.GetOrientationDegrees();

	// new orientation degrees for every frame
	float body_MaxDeltaDegrees = m_angularVelocity * deltaSeconds;
	if (m_bodyCurrent_OrientDegrees != m_bodyGoal_OrientDegrees)
	{
		m_bodyCurrent_OrientDegrees = GetTurnedTowardDegrees(m_bodyCurrent_OrientDegrees, m_bodyGoal_OrientDegrees, body_MaxDeltaDegrees);
	}

	// get the new iBasis for body and turret
	m_iBasis_Body = m_iBasis_Body.MakeFromPolarDegrees(m_bodyCurrent_OrientDegrees, 1.f);

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// check if the entity has got to the way point tile
	// in case the tank is moving while rotating, we limit its linear speed until the target is within 90 degrees

	Vec2 dispToShortTerm = direction - m_position;
	float degree = GetAngleDegreesBetweenVectors2D(m_normal, dispToShortTerm);
	if (degree < speedUpDegreeRange)
	{
		m_linearVelocity = g_gameConfigBlackboard.GetValue("leoLinearSpeed", 999.f);
	}
	else m_linearVelocity = g_gameConfigBlackboard.GetValue("leoLinearSpeed", 999.f) * lowerSpeedMultiplier;

	// Driving
	// Get new translation and position
	m_velocity = m_velocity.MakeFromPolarDegrees(m_bodyCurrent_OrientDegrees, m_linearVelocity);
	m_position += m_velocity * deltaSeconds;

	if (dispToShortTerm.GetLength() < m_physicsRadius)
	{
		m_shortTermGoalHasReached = true;
	}

	// if has touches the player
	Vec2 dispToPlayer = m_map->m_entityPtrListByType[ENTITY_TYPE_GOOD_PLAYER][0]->m_position - m_position;
	if (dispToPlayer.GetLength() < m_physicsRadius)
	{
		m_playerLastSeenPosIsReached = true;
	}
}

void Entity::SoundOffAlarmWhenSpotThePlayerDuringWandering()
{
	if ( m_soundTimer >= 2.f )
	{
		g_theAudio->StartSound(g_theApp->g_soundEffectsID[PLAYER_ISSPOTTED], false, 0.5f);
		m_soundTimer = 0.f;
	}
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
void Entity::EnemyShootBulletWhenLockOn(Entity* enemyEntity, EntityType bulletType, float angleRange, Vec2 aimingNormal)
{
	PlayerTank*& playerRef = g_theGame->m_playerTank;


	if (enemyEntity->m_timer >= enemyEntity->m_fireRateInterval && !playerRef->m_isDead)
	{
		Vec2 disp = playerRef->m_position - enemyEntity->m_position;
		if (GetAngleDegreesBetweenVectors2D(aimingNormal, disp) <= angleRange)
		{
			Vec2 bulletPos = Vec2(enemyEntity->m_physicsRadius, 0.f);
			Vec2 jBasis = aimingNormal.GetRotated90Degrees();
			TransformPosition2D(bulletPos, aimingNormal, jBasis, enemyEntity->m_position);

			g_theGame->m_currentMap->SpawnNewEntity(bulletType, bulletPos, aimingNormal.GetOrientationDegrees());
			enemyEntity->m_timer = 0.f;
		}
	}
}
