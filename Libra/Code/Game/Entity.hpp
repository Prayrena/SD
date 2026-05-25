#pragma once
#include "Engine/core/Vertex_PCU.hpp"
#include "Game/GameCommon.hpp"
#include "Engine/core/RaycastResult2D.hpp"
#include <vector>
#include "Engine/core/HeatMaps.hpp"

class Map;
class Bullet;

enum EntityFaction
{
	UNKNOW_FACTION = -1,
	FACTION_GOOD,
	FACTION_NEUTRAL,
	FACTION_EVIL,
	NUM_FACTION
};

enum EntityType
{
	UNKNOW_TYPE = -1,
	ENTITY_TYPE_EVIL_SCORPIO,
	ENTITY_TYPE_EVIL_LEO,
	ENTITY_TYPE_EVIL_ARIES,
	ENTITY_TYPE_EVIL_CAPRICORN,
	ENTITY_TYPE_GOOD_BULLET,
	ENTITY_TYPE_EVIL_BULLET,
	ENTITY_TYPE_GOOD_FLAME,
	ENTITY_TYPE_EVIL_FLAME,
	ENTITY_TYPE_EVIL_MISSILE,
	ENTITY_TYPE_GOOD_PLAYER,
	ENTITY_TYPE_MUZZLE_FLASH,
	ENTITY_TYPE_TANK_EXPLOSION,
	ENTITY_TYPE_BULLET_EXPLOSION,
	NUM_ENTITY_TYPES
};

class Entity
{
public:
	// owner define the ownership so the children could know which parent they should talk to
	// as well as they could talk to their parents 
	// however, g_theGame could directly control all the Entities's children
	Entity(Map* owner, EntityType entityType, EntityFaction faction,  Vec2 const& startPos, float orientationDegrees);
	virtual ~Entity(); // the virtual will also make sure that the derived class(children class) is deleted

	virtual void Update( float deltaSeconds) = 0;// 0 means it's pure virtual func that children's func must be rewritten


	virtual void Render() const = 0;
	virtual void DebugRender() const = 0;

	virtual bool IsOffScreen() const;
	virtual void AddVertsForRender() = 0;
	virtual void TakeDamage(int damage);

	virtual void Die();

	bool isAlive() const;

	// weapon related functions
	void EnemyShootBulletWhenLockOn(Entity* enemyEntity, EntityType bulletType, float angleRange, Vec2 aimingNormal);// todo:??? combine uniform attack function from enemies?
	virtual void ReactToBulletHit(Bullet& bullet);
	void PlayEnemyShootSound();
	void PlayEnemyHitSound();
	void PlayEnemyDeathSound();

	Vec2 GetForwardNormal() const;

	// draw debug function
	void DrawXYAxis_ForDebug(Vec2 iBasis, float length, float debugLineThickness) const;
	void DrawEntityVelocity_ForDebug(Vec2 velocity, float debugLineThickness) const;
	void DrawPhysicsCollisionRing_ForDebug(float debugLineThickness) const;
	void DrawCosmeticCollisionRing_ForDebug(float debugLineThickness) const;
	void DrawEnemyChasingDirection_ForDebug() const;

	// path finding functions
	void GenerateHeatMapForWandering();				  // used when the enemy has a new position to wander to, , notice that enemies will treat scorpio ally's occupying tile as solid tiles
	void GenerateHeatMapForWanderingWayPoint(IntVec2 startCoords); // construct a new heat map which starts from the way point with a heat value 0
	void GenerateHeatMapFromPlayerSeenLocation(); 

	// AI 
	void GenerateHeatMapAndWander(float speedUpDegreeRange, float lowerSpeedMultiplier, float deltaSeconds );
	void GenerateHeatMapAndChasePlayer(float speedUpDegreeRange, float lowerSpeedMultiplier, float deltaSeconds);

	// Audio
	void SoundOffAlarmWhenSpotThePlayerDuringWandering();

	Map*			m_map					= nullptr;			// which map this entity is managed
	EntityFaction	m_entityFaction			= UNKNOW_FACTION;	// which side the entity is on
	EntityType		m_entityType			= UNKNOW_TYPE;		// which type of the entity is this entity
	bool			m_isProjectile			= false;			// identify if the entity is projectile
	bool			m_isAnim				= false;			// identify if the entity is animation, then no collision
	bool			m_canSwim				= false;

	Vec2	m_position;
	Vec2	m_velocity;
	Vec2	m_normal;

	float  m_turretGoal_OrientDegrees = 0.f;
	float  m_turretCurrent_OrientDegrees = 0.f;
	float  m_bodyGoal_OrientDegrees = 0.f;
	float  m_bodyCurrent_OrientDegrees = 0.f;

	Vec2   m_iBasis_Body = Vec2(1.f, 0.f);
	Vec2   m_iBasis_Turret = Vec2(1.f, 0.f);

	bool			m_hasLineOfSight = false;// use to draw analysis to check if the enemy could see the player
	bool			m_playerLastSeenPosIsReached = true;
	IntVec2			m_playerLastKnownTileCoords;// for enemy to record the last time they saw the position of the player
	IntVec2			m_nextWayPointTileCoords; // the center of a tile that the player is moving to, used when chasing or wandering
	IntVec2			m_shortTermTileCoords;
	bool			m_wayPointHasReached = true; // the center of a tile that the player is moving to, used when chasing or wandering
	bool			m_shortTermGoalHasReached = true;

	RaycastResult2D m_raycastResult;// use to store the raycast information that the entity doing every frame
	TileHeatMap*	m_heatMap; // used to improve 

	// physics flags
	bool	m_isPushedByEntities = false;
	bool	m_doesPushEntities	 = false;
	bool	m_isPushedByWalls    = false; // Bullets are not able to be pushed out of walls
	bool	m_isHitByBullets	 = false; // is the bullet could do the damage to the entity
	bool	m_isFixedOnGround	 = false;

	bool	m_playerSpoted = false; // if this entity has spot the player

	float	m_orientationDegrees	= 0.f;
	float	m_linearVelocity		= 0.f;
	float   m_angularVelocity		= 0.f;// spin rate, in degrees/second, + is in counter-clockwise

	float	m_physicsRadius			= 0.5f;// radius for determine the collision
	float	m_cosmeticRadius		= 0.5f;// radius for what it looks like

	float	m_uniformScale			= 1.5f;

	float   m_timer					= 0.f;
	float   m_soundTimer			= 3.f; // trigger the alarm sound whenever see the player
	float   m_fireRateInterval		= 0.f;

	int		m_health				= 1;
	int		m_maxHealth				= 1;
	bool	m_isDead				= false;
	bool	m_isGarbage				= false;// need to be delete or not

	std::vector<Vertex_PCU> m_localVerts;
};

