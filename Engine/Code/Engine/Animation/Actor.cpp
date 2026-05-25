#include "Engine/Animation/Actor.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Animation/Skeleton.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "ThirdParty/Physx/include/PxPhysicsAPI.h"

extern RandomNumberGenerator* g_rng;
extern Renderer* g_theRenderer;

ActorUID const ActorUID::INVALID(0x0000FFFFu, 0x0000FFFFu);

using namespace std;
using namespace physx;

ActorUID::ActorUID(unsigned int salt, unsigned int index)
{
	// << will have bitwise left shift, move 16 bits to the left
	m_data = (((0x0000FFFFu & salt) << 16) | (0x0000FFFFu & index));
}

bool ActorUID::operator==(ActorUID const& other) const
{
	return m_data == other.m_data;
}

bool ActorUID::operator!=(ActorUID const& other) const
{
	return m_data != other.m_data;
}

bool ActorUID::IsValid() const
{
	return (*this != ActorUID::INVALID);
}

unsigned int ActorUID::GetIndex() const
{
	return (0x0000FFFFu & m_data);
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
Actor::Actor(Vec3 const& startPos /*= Vec3()*/, EulerAngles const& angle /*= EulerAngles()*/)
	: m_position(startPos)
	, m_orientation(angle)
{

}

Actor::~Actor()
{
	// if an actor is binding to a skeleton, deleting this actor will unbound the connection
	if (m_skeleton)
	{
		m_skeleton->m_actor = nullptr;
	}
}

void Actor::Startup()
{
	m_position = Vec3(-2.f, 0.f, 0.f);
}
 
void Actor::Update()
{

}

void Actor::Render() const
{
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->BindShader(nullptr);

	g_theRenderer->SetDepthMode(DepthMode::DISABLED);
	g_theRenderer->SetModelConstants(GetModelMatrix(), m_color);
	// transparent
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->DrawVertexArray((int)m_unlitVertexes.size(), m_unlitVertexes.data());
	// opaque
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->SetRasterizerMode(RasterizerMode::WIREFRAME_CULL_BACK);
	g_theRenderer->DrawVertexArray((int)m_unlitVertexes.size(), m_unlitVertexes.data());

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

void Actor::AttachSkeleton(Skeleton* skeleton)
{
	m_skeleton = skeleton;
	skeleton->m_actor = this;
}

void Actor::DetachSkeleton()
{
	if (m_skeleton)
	{
		m_skeleton->m_actor = nullptr;
	}
	m_skeleton = nullptr;
}

