#pragma once
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"
#include "Engine/Renderer/SpriteAnimDefinition.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Game/GameCommon.hpp"

struct ActorDefinition;
class Actor;
class Shader;

enum class WeaponType
{
	RAYCAST,
	LOCKING,
	PROJECTILE,
	MELEE,
	COUNT
};

enum class WeaponAnimState
{
	IDLE,
	ATTACK,
	COUNT
};

enum class WeaponLockingStatus
{
	INVALID,
	LOCKING,
	LOCKED,
	COUNT
};

struct HUDAnimationDefinition
{
	std::string m_name = "uninitialized";
	float		m_secondsPerFrame = 1.f;
	Shader* m_shader = nullptr;
	SpriteSheet* m_spriteSheet = nullptr;
	IntVec2 m_cellCount = IntVec2();
	int m_startFrame = 0;
	int m_endFrame = 0;

	SpriteAnimPlaybackType m_playbackMode = SpriteAnimPlaybackType::ONCE;

	SpriteAnimDefinition* m_spriteAnimDef;
};

struct WeaponDefinition
{
	WeaponDefinition(XmlElement* weaponDefElement);

	std::string m_weaponName = "Undefined";
	WeaponType m_weaponType = WeaponType::COUNT;
	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// raycast weapon
	float m_refireTime = 0.f;
	int	  m_rayCount = 0;

	// Maximum angle variation for each ray cast, in degrees
	// Each shot fired should be randomly distributed in a cone of this angle relative forward direction of the firing actor.
	float m_rayCone = 0.f;
	float m_rayRange = 0.f; // The distance of each ray cast, in world units

	// Minimum and maximum damage expressed as a float range
	// Each shot fired should do a random amount of damage in this range.
	FloatRange m_rayDamage; 

	// The amount of impulse to impart to any actor hit by a ray
	// Impulse should be in the direction of the ray
	float m_rayImpulse = 0.f;

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// projectile weapon
	// Number of projectiles to launch each time the weapon is fired
	int m_projectileCount = 0;

	float m_flyDist = 999999.f; // the default fire range is infinite

	// Maximum angle variation in degrees for each projectile launched cast
	// Each projectile launched should have its velocity randomly distributed in a cone of this angle relative to the forward direction of the firing actor
	float m_projectileCone = 0.f; // random yaw and pitch for the ray fwd normal

	float m_projectileSpeed = 0.f;
	ActorDefinition* m_projectileActorDef = nullptr;

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// melee attack
	int m_meleeCount = 0; // Number of melee attacks that should occur each time the weapon is fired.
	float m_meleeRange = 0.f;
	float m_meleeArc = 0.f; // Arc in which melee attacks occur, in degrees
	FloatRange m_meleeDamage; // Each melee attack should do a random amount of damage in this range.

	// The amount of impulse to impart to any actor hit by a melee attack
	// Impulse should be in the forward direction of the firing actor
	float m_meleeImpulse = 0.f;

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// HUD
	Shader*  m_shader = nullptr;
	Texture* m_baseTexture = nullptr;
	Texture* m_reticleTexture = nullptr;
	IntVec2  m_reticleSize = IntVec2();
	
	IntVec2  m_spriteSize = IntVec2();
	Vec2     m_spritePivot = Vec2();

	std::vector<HUDAnimationDefinition*> m_HUDAnims;

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// spear locking design
	float	m_lockOnDuration = 0.f;
	float	m_missDuration = 0.f;

	// sounds
	SoundID		m_fireSoundID = MISSING_SOUND_ID;
	SoundID		m_lockingID = MISSING_SOUND_ID;
	SoundID		m_lockedID = MISSING_SOUND_ID;

	static std::vector<WeaponDefinition> s_weaponDefs;
	static void InitializeWeaponDefs();
	static WeaponDefinition* GetWeaponDefByString(std::string str);
};

class Weapon
{
public:
	Weapon(WeaponDefinition* weaponDef);

	WeaponDefinition* m_weaponDef = nullptr;

	// need to check if the weapon is cool down to attack
	// responsible to check if any actor in different faction has been hit 
	ActorPtrList Fire(Actor* owner); 

	// locking target
	Timer*  m_lockingTimer = nullptr;
	Timer*  m_missingTimer = nullptr;
	Actor*  m_lockingTarget = nullptr;
	Actor* m_savedTarget = nullptr;
	bool   m_aimingAligned = false;
	WeaponLockingStatus m_lockingStatus = WeaponLockingStatus::INVALID;
	void DetectLockingTarget(Player* player, Actor* playerActor);
	void UpdateWeaponLockingSound(Actor* actor);

	// shoot calculation functions
	void CalculateAndApplyImpulse(Actor* attacker, Actor* defender, float impulseValue);
	Vec3 GetRandomDirectionInCone(Vec3 direction, float range); // Calculate Shooting Deflection

	// animations
	Clock* m_animationClock = nullptr;
	WeaponAnimState m_currentState = WeaponAnimState::IDLE;
	WeaponAnimState m_lasFrameState = WeaponAnimState::IDLE;

	std::string		GetStringForWeaponState(WeaponAnimState state) const;
	WeaponAnimState	GetWeaponAnimStateByString(std::string name) const;
	HUDAnimationDefinition* GetHUDAnimDefForState(WeaponAnimState state) const;
	void			PlayAnimation(WeaponAnimState state);
	void			UpdateAnimClock();
	float			GetWeaponAnimDurationForWeaponState(WeaponAnimState state);
	SpriteDefinition const& GetSpriteDefForCurrentState() const;

	// sound
	SoundPlaybackID m_fireSoundPlaybackID = MISSING_SOUND_ID;
	SoundPlaybackID m_lockingSoundPlaybackID = MISSING_SOUND_ID;
	SoundPlaybackID m_lockedSoundPlaybackID = MISSING_SOUND_ID;

	Timer* m_weaponTimer = nullptr;
	Clock* m_animClock = nullptr;
};