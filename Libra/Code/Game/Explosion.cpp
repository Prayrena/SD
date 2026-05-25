#include "Game/Explosion.hpp"
#include "Game/Map.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/core/VertexUtils.hpp"
#include "Engine/Renderer/SpriteAnimDefinition.hpp"
#include "Game/App.hpp"
#include "Game/Game.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Game/Map.hpp"
#include "Game/Bullet.hpp"
#include "Engine/Audio/AudioSystem.hpp"

extern RandomNumberGenerator*	g_rng;
extern App*						g_theApp;
extern Renderer*				g_theRenderer;
extern AudioSystem*				g_theAudio;
extern Game*					g_theGame;

Explosion::Explosion(Map* owner, EntityType type, EntityFaction faction, Vec2 const& startPos, float angleDegrees, float duration)
	:Entity(owner, type, faction, startPos, angleDegrees)
{
	m_entityFaction = faction;
	m_isAnim = true;
	m_physicsRadius = 0.f;
	
	m_iBasis_Body = m_iBasis_Body.MakeFromPolarDegrees(angleDegrees, 1.f);
	m_orientationDegrees = angleDegrees;
	m_isFixedOnGround = true;
	m_isPushedByEntities = false;
	m_doesPushEntities = false;
	m_isPushedByWalls = false;
	m_isHitByBullets = false;
	m_cosmeticRadius = m_scale;
	m_timer = 0.f;
	m_duration = duration;

	switch (type)
	{
	case ENTITY_TYPE_MUZZLE_FLASH: {m_scale = 0.75f; }
		break;
	case ENTITY_TYPE_TANK_EXPLOSION: {m_scale = 1.f; }
		break;
	case ENTITY_TYPE_BULLET_EXPLOSION: {m_scale = 0.5f; }
		break;
	}
	AddVertsForRender(); // otherwise the render function will have m_localVerts.size() = 0 and throw an error
}

Explosion::~Explosion()
{

}

void Explosion::Update(float deltaSeconds)
{
	m_timer += deltaSeconds;
	if ( m_timer >= m_duration )
	{
		m_isGarbage = true;
	}
	switch (m_entityType)
	{
	case ENTITY_TYPE_MUZZLE_FLASH: 
	{	
		SpriteDefinition sprite = g_theGame->m_muzzleFlashAnim->GetSpriteDefAtTime(m_timer); 
		m_uvBounds = sprite.GetUVs();
	}break;
	case ENTITY_TYPE_TANK_EXPLOSION: 
	{
		SpriteDefinition sprite = g_theGame->m_tankExplosionAnim->GetSpriteDefAtTime(m_timer);
		m_uvBounds = sprite.GetUVs();
	}break;
	case ENTITY_TYPE_BULLET_EXPLOSION: 
	{
		SpriteDefinition sprite = g_theGame->m_bulletExplosionAnim->GetSpriteDefAtTime(m_timer);
		m_uvBounds = sprite.GetUVs();
	}break;
	}
	AddVertsForRender();
}

/// <Render>
/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Explosion::AddVertsForRender()
{
	m_localVerts.clear();
	m_localVerts.reserve(6);
	// the size of the sprite is defined by the scale
	AABB2 body = AABB2(-m_scale * 0.5f, -m_scale * 0.5f, m_scale * 0.5f, m_scale * 0.5f);
	AddVertsUVForAABB2D(m_localVerts, body, Rgba8::WHITE, m_uvBounds);
}

void Explosion::Render() const
{
	std::vector<Vertex_PCU> explosionVerts;
	explosionVerts.resize(6);
	for (int i = 0; i < (int)explosionVerts.size(); ++i)
	{
		explosionVerts[i] = m_localVerts[i];
	}
	TransformVertexArrayXY3D(explosionVerts, m_iBasis_Body, m_position);

	g_theRenderer->BindTexture(g_theApp->g_textureID[SPRITESHEET_EXPLOSION]);
	g_theRenderer->SetBlendMode(BlendMode::ADDITIVE);
	g_theRenderer->DrawVertexArray((int)explosionVerts.size(), explosionVerts.data());
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
}

void Explosion::DebugRender() const
{
	DrawXYAxis_ForDebug(m_iBasis_Body, m_cosmeticRadius, DEBUGLINE_THICKNESS);
	DrawCosmeticCollisionRing_ForDebug(DEBUGRING_THICKNESS);
}
