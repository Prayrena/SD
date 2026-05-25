#include "Game/Bullet.hpp"
#include "Game/Map.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/core/VertexUtils.hpp"
#include "Game/App.hpp"
#include "Game/Game.hpp"
#include "Engine/Renderer/SpriteAnimDefinition.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Game/Map.hpp"
#include "Game/Bullet.hpp"
#include "Engine/Audio/AudioSystem.hpp"

extern RandomNumberGenerator* g_rng;
extern App* g_theApp;
extern Game* g_theGame;
extern Renderer* g_theRenderer;
extern AudioSystem* g_theAudio;

Bullet::Bullet(Map* owner, EntityType type, EntityFaction faction, Vec2 const& startPos, float angleDegrees)
	:Entity(owner, type, faction, startPos, angleDegrees)
{
	m_entityFaction = faction;
	m_isProjectile = true;
	m_physicsRadius = 0.1f;
	
	m_iBasis_Body = m_iBasis_Body.MakeFromPolarDegrees(angleDegrees, 1.f);
	m_orientationDegrees = angleDegrees;
	m_isFixedOnGround = false;
	m_isPushedByEntities = false;
	m_doesPushEntities = false;
	m_isPushedByWalls = true;
	m_isHitByBullets = false;

	switch (type)
	{
	case ENTITY_TYPE_GOOD_BULLET: { m_health = 3; m_speed = g_gameConfigBlackboard.GetValue("playerBulletSpeed", 999.f); } break; // player's bullet could bounce three times
	case ENTITY_TYPE_EVIL_BULLET: { m_health = 1; m_speed = g_gameConfigBlackboard.GetValue("enemyBulletSpeed", 999.f); } break; // enemy's bullet could not bounce
	case ENTITY_TYPE_EVIL_MISSILE: 
	{ m_health = 1; 
	m_speed = g_gameConfigBlackboard.GetValue("enemyMissileLinearSpeed", 999.f); 
  	m_angularVelocity = g_gameConfigBlackboard.GetValue("enemyMissileAngularSpeed", 999.f);
	} break; // enemy's missile could not bounce, but could track

	case ENTITY_TYPE_GOOD_FLAME:
	{ m_health = 1;
	m_speed = g_gameConfigBlackboard.GetValue("playerFlameLinearSpeed", 999.f);
	} break;

	case ENTITY_TYPE_EVIL_FLAME:
	{ m_health = 1;
	m_speed = g_gameConfigBlackboard.GetValue("enemyFlameLinearSpeed", 999.f);
	} break;
	}

	m_velocity = m_velocity.MakeFromPolarDegrees(angleDegrees, m_speed);
	IntVec2 spawnTileCoords = m_map->GetTileCoordsInMap_For_WorldPos(startPos);
	if (m_map->IsTileSolid(spawnTileCoords))
	{
		m_velocity.GetRotatedDegrees(180.f);
	}

	int i = g_rng->RollRandomIntInRange(0, 1);
	if (i == 0)
	{
		m_angularVelocity = g_rng->RollRandomFloatInRange(100.f, 500.f); // each flame has random rotation speed
	}
	else
	{
		m_angularVelocity = g_rng->RollRandomFloatInRange(-100.f, -500.f); // each flame has random rotation speed
	}
	AddVertsForRender();
}

Bullet::~Bullet()
{

}

void Bullet::Update(float deltaSeconds)
{
	if (isAlive())
	{
		switch (m_entityType)
		{
		case ENTITY_TYPE_GOOD_BULLET: { BulletUpdate(deltaSeconds); }
			break;
		case ENTITY_TYPE_EVIL_BULLET: { BulletUpdate(deltaSeconds); }
			break;
		case ENTITY_TYPE_EVIL_MISSILE: { MissileUpdate(deltaSeconds); }
			break;
		case ENTITY_TYPE_GOOD_FLAME: { FlameUpdate(deltaSeconds); }
			break;
		case ENTITY_TYPE_EVIL_FLAME: { FlameUpdate(deltaSeconds); }
			break;
		}
	}
}

void Bullet::BulletUpdate(float deltaSeconds)
{
	Vec2 prevPosition = m_position; // copy over last frame pos as previous position
	m_position += m_velocity * deltaSeconds;

	// check if the bullet is running inside a solid tile
	IntVec2 prevTileCoords = m_map->GetTileCoordsInMap_For_WorldPos(prevPosition);
	IntVec2 nextTileCoords = m_map->GetTileCoordsInMap_For_WorldPos(m_position);
	if (m_map->IsTileSolid(nextTileCoords))
	{
		// Bounce off when the next frame is inside a tile
		m_position = prevPosition; // back up to last known safe position
		Vec2 reflectionNormal = Vec2(prevTileCoords - nextTileCoords).GetNormalized();
		BouceOff(reflectionNormal);
	}
}

void Bullet::MissileUpdate(float deltaSeconds)
{
	Entity* playerRef = m_map->m_entityPtrListByType[ENTITY_TYPE_GOOD_PLAYER][0];
	Vec2 prevPosition = m_position;
	// when the player is alive the bullet turns towards to the player
	if (playerRef->isAlive())
	{
		Vec2 direction = (playerRef->m_position - m_position).GetNormalized();
		m_bodyGoal_OrientDegrees = direction.GetOrientationDegrees();

		float body_MaxDeltaDegrees = m_angularVelocity * deltaSeconds;
		if (m_orientationDegrees != m_bodyGoal_OrientDegrees)
		{
			m_orientationDegrees = GetTurnedTowardDegrees(m_orientationDegrees, m_bodyGoal_OrientDegrees, body_MaxDeltaDegrees);
			m_iBasis_Body = m_iBasis_Body.MakeFromPolarDegrees(m_orientationDegrees, 1.f);
			m_velocity = m_velocity.MakeFromPolarDegrees(m_orientationDegrees, m_speed);
		}
	}

	// if the player is dead, the missile go straight
	m_position += m_velocity * deltaSeconds;

	// check if the bullet is running inside a solid tile
	IntVec2 prevTileCoords = m_map->GetTileCoordsInMap_For_WorldPos(prevPosition);
	IntVec2 nextTileCoords = m_map->GetTileCoordsInMap_For_WorldPos(m_position);
	if (m_map->IsTileSolid(nextTileCoords))
	{
		m_isDead = true;
		g_theAudio->StartSound(g_theApp->g_soundEffectsID[BULLET_BOUNCE], false, 0.05f);
	}
}

void Bullet::FlameUpdate(float deltaSeconds)
{
	m_timer += deltaSeconds;
	if (m_timer >= 1.f)
	{
		m_isGarbage = true;
	}

	m_orientationDegrees += m_angularVelocity * deltaSeconds;
	m_iBasis_Body = m_iBasis_Body.MakeFromPolarDegrees(m_orientationDegrees, 1.f);
	m_position += m_velocity * deltaSeconds;

	// the flame sprite will rewrite its UV information every frame
	m_localVerts.clear();
	m_localVerts.reserve(6);
	AABB2 body = AABB2(-m_flameHalfSize, -m_flameHalfSize, m_flameHalfSize, m_flameHalfSize);

	SpriteDefinition sprite = g_theGame->m_bulletExplosionAnim->GetSpriteDefAtTime(m_timer);
	m_uvBounds = sprite.GetUVs();
	AddVertsUVForAABB2D(m_localVerts, body, Rgba8::WHITE, m_uvBounds);
}

void Bullet::BouceOff(Vec2 wallNormal)
{
	TakeDamage(1);
	// todo: player reflection sound
	m_velocity = m_velocity.Reflect(wallNormal);
	float spreadAngle = g_rng->RollRandomFloatInRange(-5.f, 5.f);
	m_velocity = m_velocity.GetRotatedDegrees(spreadAngle);
	if (m_health > 0)
	{
		g_theAudio->StartSound(g_theApp->g_soundEffectsID[BULLET_BOUNCE], false, 0.05f);
	}
}

void Bullet::RenderBullet() const
{
	std::vector<Vertex_PCU> bulletVerts;
	bulletVerts.resize(6);
	for (int i = 0; i < (int)bulletVerts.size(); ++i)
	{
		bulletVerts[i] = m_localVerts[i];
	}
	TransformVertexArrayXY3D(bulletVerts, m_iBasis_Body, m_position);

	switch (m_entityType)
	{
	case ENTITY_TYPE_GOOD_BULLET: { g_theRenderer->BindTexture(g_theApp->g_textureID[T_GOOD_BOLT]); } break;
	case ENTITY_TYPE_EVIL_BULLET: { g_theRenderer->BindTexture(g_theApp->g_textureID[T_EVIL_BOLT]); } break;
	case ENTITY_TYPE_EVIL_MISSILE: { g_theRenderer->BindTexture(g_theApp->g_textureID[T_EVIL_MISSILE]); } break;
	case ENTITY_TYPE_GOOD_FLAME: 
	{ 
		g_theRenderer->SetBlendMode(BlendMode::ADDITIVE);
		g_theRenderer->BindTexture(g_theApp->g_textureID[SPRITESHEET_EXPLOSION]);
	} break;
	case ENTITY_TYPE_EVIL_FLAME: 
	{ 
		g_theRenderer->SetBlendMode(BlendMode::ADDITIVE);
		g_theRenderer->BindTexture(g_theApp->g_textureID[SPRITESHEET_EXPLOSION]);	
	} break;
	}
	g_theRenderer->DrawVertexArray((int)bulletVerts.size(), bulletVerts.data());
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
}

/// <Render>
/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Bullet::AddVertsForRender()
{
	m_localVerts.reserve(6);

	AABB2 body = AABB2(-BULLET_SPRITE_HALFSIZE, -BULLET_SPRITE_HALFSIZE, BULLET_SPRITE_HALFSIZE, BULLET_SPRITE_HALFSIZE);
	AddVertsForAABB2D(m_localVerts, body, Rgba8::WHITE);
}

void Bullet::Render() const
{
	RenderBullet();
}

void Bullet::DebugRender() const
{
	DrawXYAxis_ForDebug(m_iBasis_Body, m_cosmeticRadius, DEBUGLINE_THICKNESS);
	DrawEntityVelocity_ForDebug(m_velocity, DEBUGLINE_THICKNESS);
	DrawPhysicsCollisionRing_ForDebug(DEBUGRING_THICKNESS);
	DrawCosmeticCollisionRing_ForDebug(DEBUGRING_THICKNESS);
}
