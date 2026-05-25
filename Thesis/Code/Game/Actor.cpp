#include "Game/Actor.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"

extern RandomNumberGenerator* g_rng;

Actor::Actor(Game* owner, Vec3 const& startPos /*= Vec3()*/)
	: m_game(owner)
	, m_position(startPos)
{

}

Actor::~Actor()
{
}

void Actor::Startup()
{
	m_position = Vec3(-2.f, 0.f, 0.f);
}

void Actor::DebugRender() const
{

}

bool Actor::IsOffScreen() const
{
	return false;
}

void Actor::InitializeLocalVerts()
{

}

Vec2 Actor::GetForwardNormal() const
{
	return Vec2(0.f, 0.f);
}

// transform coordinate from local to world matrix
// [translate][rotate]
Mat44 Actor::GetModelMatrix() const
{
	Mat44 transformMat;
	transformMat.SetTranslation3D(m_position);
	Mat44 orientationMat = m_orientation.GetAsMatrix_XFwd_YLeft_ZUp();
	// orientationMat.Append(transformMat);
	// return orientationMat;
	transformMat.Append(orientationMat);
	return transformMat;
}

