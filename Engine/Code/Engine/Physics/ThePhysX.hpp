#pragma once
#include "ThirdParty/Physx/include/PxPhysicsAPI.h"
#include "Engine/Animation/Actor.hpp"
#include <unordered_map>
#include <string>
#include <utility>

// !!! notice that the for additional dependencies, we have the physx related lib in Game property settings
// to use physX, you also need related dlls by the side of your exe

constexpr float PHYSX_FIXEDUPDATE_TIMESTEP = 1.f / 60.f;

// Forward declarations of PhysX types
namespace physx {
	class PxDefaultAllocator;
	class PxDefaultErrorCallback;
	class PxFoundation;
	class PxPhysics;
	class PxDefaultCpuDispatcher;
	class PxScene;
	class PxPvd;
	class PxMaterial;
	class PxRigidActor;
	class PxRigidStatic;
	class PxRigidDynamic;
	class PxPlane;
}

// is it possible to combine this class with ThePhysX class?
// how can I set that when specific actor A collide with specific actor B, apply the force magnitude with certain number?
// how can I say if A's velocity is higher, apply more force?
class PhysXCollisionCallback : public physx::PxSimulationEventCallback 
{
public:
	// Called when two actors collide, override physx function
	void onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, physx::PxU32 nbPairs) override;
	void ApplyForceOnCollision(physx::PxRigidDynamic* actorA, physx::PxRigidDynamic* actorB, const physx::PxContactPair& contactPair);
};

struct PhysXSystemConfig
{
	// physics simulation settings
	bool	m_isUsingFixedPhysicsTimestep = true;
	float	m_physicsTimeOwed = 0.f;
	float	m_physicsFixedTimestep = 1.f / 90.f;

	// plane friction setting
	float	m_groundStaticFiction = 0.5f;
	float	m_groundDynamicFiction = 0.5f;
	float	m_groundRestition = 0.f;

	bool	m_debugRender = true;

	// debug settings
	// bool m_showBodyMassAxes = true;
	float m_globalScale = 0.5f;
	std::pair<bool, float> m_showBodyAxes = std::make_pair(true, 1.f);
	std::pair<bool, float> m_showCollisionShapes = std::make_pair(true, 1.f);
	std::pair<bool, float> m_showActorAxes = std::make_pair(true, 1.f);
	// std::pair<bool, float> m_showAngVel = std::make_pair(true, 1.f);
	std::pair<bool, float> m_showJointLocalFrame = std::make_pair(true, 3.f);
	std::pair<bool, float> m_showJointLimits = std::make_pair(true, 1.f);
	std::pair<bool, float> m_showContactPoint = std::make_pair(true, 1.f);
};

class ThePhysX
{
public:
	ThePhysX() = default;
	ThePhysX(PhysXSystemConfig config);
	~ThePhysX() = default;

	void ShutDown();

	void Startup();
	void Update();

	void Render() const;

	void SetVisualizationParameterByConfig();

	physx::PxMaterial* CreatePhysXMaterial(std::string const& name, float staticFriction, float dynamicFriction, float restitution);
	Actor* CreateStaticActor(std::string const& name, float staticFriction, float dynamicFriction, float restitution);

	PhysXSystemConfig				m_PhysXConfig;

	physx::PxDefaultAllocator		m_allocator;
	physx::PxDefaultErrorCallback	m_errorCallback;
	physx::PxFoundation* m_foundation = NULL;
	physx::PxPhysics* m_physics = NULL;
	physx::PxDefaultCpuDispatcher* m_dispatcher = NULL;
	physx::PxScene* m_scene = NULL;
	physx::PxPvd* m_pvd = NULL;

	physx::PxMaterial*		GetPhysXMaterialByStinrg(std::string const& materialName) const;
	std::map<std::string, physx::PxMaterial*>	m_materials;

	physx::PxRigidActor*	GetPhysXActorByActorUID(ActorUID const& actorUID) const;
	// std::map<ActorUID, physx::PxRigidStatic*>	m_staticActors;
	// std::map<ActorUID, physx::PxRigidDynamic*>	m_dynamicActors;	// A Kinematic actor in PhysX is a special configuration of a PxRigidDynamic actor

	std::map<std::string, Actor*>				m_actors;

	void CreatePhysXPlaneAndGroundMat();
	physx::PxPlane* m_plane = nullptr;

	physx::PxRigidDynamic* ConvertActorFromDynamicToKinematic(physx::PxRigidDynamic* dynamicAcotr);
	physx::PxRigidDynamic* ConvertActorFromKinematicToDynamic(physx::PxRigidDynamic* kinematicAcotr);

	// all input transform info is in game space
	void MoveKinematicActor(physx::PxRigidDynamic* actor, Vec3 const& targetPos, Quat const& targetOrientation);

	//PxFilterFlags CreateFilterShader(
	//	physx::PxFilterObjectAttributes attributes0, physx::PxFilterData filterData0,
	//	physx::PxFilterObjectAttributes attributes1, physx::PxFilterData filterData1,
	//	physx::PxPairFlags& pairFlags, const void* constantBlock, physx::PxU32 constantBlockSize);

private:
	bool	m_isUsingFixedPhysicsTimestep = true;
	float	m_physicsTimeOwed = 0.f;
	float	m_physicsFixedTimestep = (1.f / 90.f) / 1.f;	// 5 physics tick per frame at 60 FPS
};