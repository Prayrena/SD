#include "Game/Entity.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"

extern RandomNumberGenerator* g_rng;

Entity::Entity(Game* owner, Vec3 const& startPos /*= Vec3()*/)
	: m_game(owner)
	, m_position(startPos)
{

}

Entity::~Entity()
{
}

void Entity::Startup()
{
	m_position = Vec3(-2.f, 0.f, 0.f);
}

void Entity::DebugRender() const
{

}

bool Entity::IsOffScreen() const
{
	return false;
}

void Entity::InitializeLocalVerts()
{

}

Vec2 Entity::GetForwardNormal() const
{
	return Vec2(0.f, 0.f);
}

// transform coordinate from local to world matrix
// [translate][rotate]
Mat44 Entity::GetModelMatrix() const
{
	Mat44 transformMat;
	transformMat.SetTranslation3D(m_position);
	Mat44 orientationMat = m_orientation.GetAsMatrix_XFwd_YLeft_ZUp();
	// orientationMat.Append(transformMat);
	// return orientationMat;
	transformMat.Append(orientationMat);
	return transformMat;
}

