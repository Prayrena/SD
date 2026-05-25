#pragma once
#include "Engine/core/Vertex_PCU.hpp"
#include "Engine/core/Vertex_PCUTBN.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/core/VertexUtils.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/core/RaycastUtils.hpp"
#include "Engine/Renderer/SpriteAnimDefinition.hpp"
#include "Engine/core/Clock.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Math/Splines.hpp"
#include "Game/ActorUID.hpp"
#include "Game/Game.hpp"
#include "Game/GameCommon.hpp"
#include "Game/Controller.hpp"
#include "Game/AIController.hpp"
#include "Game/Weapon.hpp"
#include <string>
#include <map>


class Game;// allow all entity uses Game class's function
struct ZCylinder;
class Texture;
struct SpawnInfo;
class Shader;

enum class ActorAnimState
{
	WALK,
	ATTACK,
	HURT,
	DEATH,
	NUM_STATE

};

enum class ActorFaction
{
	NEUTRAL,
	DEMON,
	MARINE,
	COUNT
};

struct SpriteAnimationGroupDefinition
{
	SpriteAnimationGroupDefinition() = default;
	~SpriteAnimationGroupDefinition() = default;

	std::string m_name = "uninitialized";
	float		m_secondsPerFrame = 1.f;
	SpriteAnimPlaybackType m_playbackMode = SpriteAnimPlaybackType::ONCE;
	bool		m_scaleBySpeed = false;

	std::vector<Vec3> m_directions;
	std::vector<SpriteAnimDefinition*> m_spriteAnimDefs;

	// std::map<Vec3, SpriteAnimDefinition*> m_animations; // todo: 4.15 why this std will throw error?
};

struct ActorDefinition
{
	ActorDefinition() = default;
	~ActorDefinition() = default;
	ActorDefinition(XmlElement* actorDefElement);

	// base
	std::string    m_actorName;
	ActorFaction m_faction = ActorFaction::NEUTRAL;
	float  m_health = 0.f;
	bool m_visible = false;
	float m_invisibleDuration = 0.f;
	float m_teleportDist = 0.f;
	float m_corpseLifetime = 0.f;
	bool m_renderForward = false; // true to render beak-like cone on its face

	Rgba8 m_solidColor = Rgba8(32, 32, 32);
	Rgba8 m_wireframeColor = Rgba8(192, 192, 192);

	// characteristic
	bool m_canBePossessed = false;
	bool m_isShielded = false;
	bool m_bulletproof = false;
	bool m_isProjectile = false;

	// collision
	float m_physicsRadius = 0.f;
	float m_physicsFootHeight = 0.f;
	float m_physicsHeight = 0.f;
	bool  m_collidesWithWorld = false;
	bool  m_collidesWithActors = false;
	FloatRange  m_damageOnCollide = FloatRange(0.f, 0.f);
	float  m_impulseOnCollide = 0.f; // The amount of impulse the actor should impart to another actor upon collision
	bool  m_dieOnCollide = false;
	bool  m_dieOnSpawn = false;

	// physics
	bool  m_simulated = false;
	bool  m_flying = false; // Specifies whether the actor can come off the ground or not
	bool  m_isHomingMissile = false;
	float m_walkSpeed = 0.f; 
	float m_runSpeed = 0.f;
	float m_drag = 0.f; //Scalar value used to determine how much to slow down actor movement each frame. Will be negated and multiplied by velocity during physics simulation.
	float m_turnSpeed = 0.f; // Maximum rate at which the actor can turn, in degrees per second.

	// visuals
	Vec2 m_size = Vec2(1.f, 1.f); // Size of the actor sprite, in world units
	Vec2 m_pivot = Vec2(0.5f, 0.5f); // pivot for centering and positioning the actor sprite
	BillboardType m_billboardType = BillboardType::NONE; // Billboard type for this actor
	bool m_renderLit = false; // Whether or not to include vertex normals in the geometry for this actor for purposes of lighting
	bool m_renderRounded = false; // Whether or not this actor should be rendered with two adjacent quads and rounded normals
	Shader* m_shader = nullptr; // Shader to use when rendering this actor
	IntVec2 m_cellCount = IntVec2(1, 1); // Sprite sheet grid dimensions
	SpriteSheet* m_spriteSheet = nullptr; // Sprite sheet containing the actor animations

	// camera
	float m_eyeHeight = 0.f; //Height off the ground at which to place the camera when this actor is possessed
	float m_cameraFOVDegrees = 60.f;

	// AI
	bool m_aiEnabled = false;
	float m_sightRadius = 0.f;
	float m_sightAngle = 0.f;

	// weapon
	std::vector<WeaponDefinition*> m_weapons;

	// animation
	std::vector<SpriteAnimationGroupDefinition*> m_spriteAnimGroupDefs;

	// sounds
	SoundID		m_hurtSoundID = MISSING_SOUND_ID;
	SoundID		m_deathSoundID = MISSING_SOUND_ID;
	SoundID		m_activeSoundID = MISSING_SOUND_ID;

	static void InitializeActorDefs(char const* filePath);
	static ActorDefinition* GetActorDefByString(std::string str);
	static ActorFaction GetActorFactionByString(std::string str);
	static std::vector<ActorDefinition*> s_actorDefs;
};

class Actor
{
public:
	Actor(SpawnInfo spawnInfo, Map* map, ActorUID uid);
	Actor(Map* owner, Vec3 startPos, EulerAngles orientation, Rgba8 color, ZCylinder collision, bool isStatic = true);
	virtual ~Actor();

	void Startup();
	void Update();
	void UpdatePhysics();
	void UpdateDeathDestroyedStatus();
	void ShutDown();

	// collision
	ZCylinder GetCylinderCollisionInWorldSpace() const;

	void Render() const;
	void DebugRender();	

	void InitializeCollision();
	void InitializeQuadVertsAndRender() const;

	Mat44 GetModelMatrix() const;
	Mat44 GetRenderModelMatrix() const; // only take yaw so the cylinder will not look down

	Vec3 GetForwardNormal() const;
	Vec3 GetEyePosition() const;
	Vec3 GetWeaponPosition() const;

	void TakeDamage(float damage, Actor* source);
	void Die(Actor* source);

	void AddForce(const Vec3& force);
	void AddImpulse(const Vec3& impulse);
	void OnCollide(Actor* other); // what happens if other actor collide with me: nothing, die, damage, impulse

	void OnPossessed(Controller* controller);
	void OnUnpossessed(Controller* controller);
	void MoveInDirection(Vec3 direction, float speed);
	void TurnInDirection(float goalDegrees);

	// teleportation
	bool m_hasTeleported = false;
	void TeleportToRandomDirection();

	ActorFaction GetActorFaction();

	// weapons and attack function
	Weapon* GetEquippedWeapon();
	void	EquipNextWeapon();
	void	EquipPreviousWeapon();
	void	EquipWeapon(int weaponIndex);
	void	Attack();

	// positions
	float m_launchHeightMultiplier = 0.75f;
	float m_rightHandOffset = 0.f;
	Vec3 GetRaycastShootingPosition() const;
	Vec3 GetPlasmaShootingPosition() const;

	Vec3		m_position;
	Vec3		m_velocity; // 3D velocity, as a Vec3, in world units per second
	Vec3		m_acceleration; // 3D acceleration, as a Vec3, in world units per second squared
	EulerAngles m_orientation;
	EulerAngles m_angularVelocity; // Euler angles per second

	Rgba8	m_color = Rgba8::WHITE;

	ZCylinder   m_collision; // in local space
	Vec3		m_collisionOffset; // this is the offset of the collision to the actor origin

	bool	m_isStatic = true;

	Texture* m_texture = nullptr;

	Game* m_game = nullptr;
	Map* m_map = nullptr;
	ActorDefinition*   m_actorDef;

	// animation
	Clock* m_animationClock = nullptr;
	ActorAnimState m_currentState = ActorAnimState::WALK;
	ActorAnimState m_lasFrameState = ActorAnimState::WALK;
	std::vector<SpriteAnimationGroupDefinition*> m_spriteAnimGroupDefs;

	std::string						GetStringForActorState(ActorAnimState state) const;
	ActorAnimState					GetActorStateByString(std::string name) const;
	SpriteAnimationGroupDefinition* GetAnimGroupDefByActorState(ActorAnimState state) const;
	void	PlayAnimation(ActorAnimState state);
	void	UpdateAnimClock();
	float	GetAnimDurationForActorState(ActorAnimState state) const;
	SpriteDefinition const& GetSpriteDefForCurrentState() const;

	// controls
	ActorUID	m_actorUID = ActorUID::INVALID;
	ActorUID	m_ownerUID = ActorUID::INVALID;
	Weapon*		m_ownerWeapon = nullptr; // if this actor is a projectile, it will remember which weapon fires itself
	Controller* m_controller;
	AIController* m_AIController;

	std::vector<Weapon*> m_weapons;
	int m_equippedWeaponIndex = 0;

	RaycastResult3D m_raycastResult; // stored info for enemy to check

	// sounds
	SoundPlaybackID m_soundPlaybackID = MISSING_SOUND_ID;
	SoundPlaybackID m_weaponSoundPlaybackID = MISSING_SOUND_ID;

	bool m_isListenedByPlayer = false;

	void UpdateAudio();
	void PlaySound(SoundID soundID);
	void PlayWeaponSound(SoundID soundID);
	void StopWeaponSound();

	// homing missile
	void ConstructProjectileTrajectory(Vec3 start, Actor* actor, float flyingDuration, float flyingSpeed);
	void UpdateMissilePositionOnTrajectory();
	CubicBezierCurve3D* m_trajectory = nullptr;
	float m_flyingDuration = 0.f;
	float m_flyingSpeed = 0.f;
	Clock* m_flyingClock = nullptr;
	Mat44  m_homingMissileMatrix;
	Actor* m_lockedTarget = nullptr;

	ActorUID m_shieldOwnerID = ActorUID::INVALID;

	// status
	float	m_health = 0.f;
	bool	m_isDead = false;
	bool	m_isDestroyed = false;// need to be delete or not

	Timer*  m_lifetimeTimer = nullptr;
	Timer*  m_invisibleTimer = nullptr;
	//----------------------------------------------------------------------------------------------------------------------------------------------------
	float	m_orientationDegrees = 0.f;
	float	m_linearVelocity = 0.f;
	float   m_angularVelocityDegree = 0.f;// spin rate, in degrees/second, + is in counter-clockwise

	float	m_uniformScale = 1.f;
};
