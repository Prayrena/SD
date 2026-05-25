#pragma once
#include "Engine/core/Vertex_PCU.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/core/VertexUtils.hpp"
#include "Engine/Math/EulerAngles.hpp"

class Skeleton;
class Texture;
class HealthBar;

namespace physx
{
	class PxMaterial;
	class PxRigidActor;
}		  

struct ActorUID
{
public:
	ActorUID() = default;
	ActorUID(unsigned int salt, unsigned int index);

	bool IsValid() const;
	unsigned int GetIndex() const;

	bool operator == (ActorUID const& other) const;
	bool operator != (ActorUID const& other) const;

	static const ActorUID INVALID;

private:
	// each unsigned int / int is 4 bytes
	// each byte has 8 bits, means unsigned int has 32 bits
	// 4 bits equal to 1 nibble, used for hexadecimal from as 0 - 15, as A - F
	// so unsigned int has 8 hexadecimal, 0xFFFFFFFFu means 1111 1111 1111 1111 1111 1111 1111 1111
	// int has 8 hexadecimal, 0xFFFFFFFF means 1111 1111 1111 1111 1111 1111 1111 1111
	// int has 8 hexadecimal, 0x00000000 means 1111 1111 1111 1111 1111 1111 1111 1111
	// first 16 bits are salt, last 16 bits are indexes
	unsigned int m_data; // ++0xFFFFFFFFu -> 0x00000000, wrap
	// for int: Ox00000000, 0x0 means positive, 0x1 means negative, this is unsure
};

class Actor
{
public:
	explicit Actor(Vec3 const& startPos = Vec3(), EulerAngles const& angle = EulerAngles());
	virtual ~Actor();

	virtual void Startup();
	virtual void Update();// 0 means it's pure virtual func that children's func must be rewritten

	virtual void Render() const;
	virtual void DebugRender() const;

	virtual bool IsOffScreen() const;
	virtual void InitializeLocalVerts();

	Vec2 GetForwardNormal() const;
	Mat44 GetModelMatrix() const;

	std::string	m_name;

	std::vector<Vertex_PCU> m_unlitVertexes;
	Texture* m_unlitTexture = nullptr;

	Vec3		m_position = Vec3::ZERO;
	Vec3		m_velocity = Vec3::ZERO;
	Vec3		m_velocityDir = Vec3::ZERO;
	float		m_speed = 0.f;
	EulerAngles m_orientation;
	EulerAngles m_angularVelocity; // Euler angles per second

	Rgba8		m_color = Rgba8::WHITE;

	bool		m_renderDebug = false;

	void		AttachSkeleton(Skeleton* skeleton);
	void		DetachSkeleton();
	Skeleton*   m_skeleton = nullptr;

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	float	m_currentHealth = 100.f;
	float	m_maxHealth = 100.f;

	HealthBar* m_healthBar = nullptr;

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// physX
	physx::PxMaterial*		m_physXMaterial = nullptr;
	physx::PxRigidActor*	m_rigidActor = nullptr;

	Vec3			m_collisionPosition;
	EulerAngles		m_collisionOrientation;
};

