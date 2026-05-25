#pragma once
#include "Game/Entity.hpp"
#include <vector>

class Bullet: public Entity
{
public:
	Bullet(Map* owner, EntityType type, EntityFaction faction, Vec2 const& startPos, float angleDegrees);
	virtual ~Bullet();

	virtual void Update( float deltaSeconds) override;
	void BulletUpdate(float deltaSeconds);
	void MissileUpdate(float deltaSeconds);
	void FlameUpdate(float deltaSeconds);

	virtual void Render() const override;
	virtual void DebugRender() const override;
	void RenderBullet() const;

	virtual void AddVertsForRender() override;

	void BouceOff(Vec2 wallNormal);

	float  m_speed = BULLET_SPEED;
	float  m_flameHalfSize = 0.6f;
	AABB2  m_uvBounds;
};

