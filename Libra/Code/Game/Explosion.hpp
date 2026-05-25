#pragma once
#include "Game/Entity.hpp"
#include <vector>

class Explosion: public Entity
{
public:
	Explosion(Map* owner, EntityType type, EntityFaction faction, Vec2 const& startPos, float angleDegrees, float duration);
	virtual ~Explosion();

	virtual void Update( float deltaSeconds) override;

	virtual void Render() const override;
	virtual void DebugRender() const override;

	virtual void AddVertsForRender() override;

	float					m_scale = 1.f;
	float					m_duration = 1.f;
	AABB2					m_uvBounds;
};

