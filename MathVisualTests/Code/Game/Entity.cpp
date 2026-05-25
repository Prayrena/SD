#include "Game/Entity.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"

extern RandomNumberGenerator* g_rng;

Entity::Entity( Vec2 const& startPos )
{
	m_position = startPos;
}

Entity::~Entity()
{
}

void Entity::TakeDamage()
{
	if (m_health > 0)
	{
		m_health -= 1;
	}

	if (m_health == 0)
	{
		m_isDead = true;
		m_isGarbage = true;
	}

	// play crash sound
}

void Entity::DebugRender() const
{

}

bool Entity::IsOffScreen() const
{
	return false;
}

Vec2 Entity::GetForwardNormal() const
{
	return Vec2(0.f, 0.f);
}


