#pragma once
#include "Engine/Math/Mat44.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Game/GameCommon.hpp"
#include "Engine/core/Clock.hpp"

class Map;
struct SpawnInfo;
class Controller;

class Player: public Controller
{
public:
	Player(SpawnInfo spawnInfo, Map* owner = nullptr, ActorUID uid = ActorUID::INVALID);
	Player(Vec3 position = Vec3(), Map* owner = nullptr);
	~Player();

	void Startup();
	virtual void Update() override;
	void Render() const;
	void RenderHitIndicator() const;

	virtual bool IsPlayer() override;
	Mat44 GetModelMatrix() const;

	Vec3		m_position;
	Vec3		m_velocity;
	EulerAngles m_orientation;
	EulerAngles m_angularVelocity; // Euler angles per second

	Vec3 GetCameraOrientation();
	Vec3 GetCameraUp();

	// but if this is an instance, how should I able to assign other camera to player's camera
	// multiply player setting
	Camera m_worldCamera; // if this is camera*, then when I try to set up set transform will throw an error
	Camera m_screenCamera; // for HUD rendering
	AABB2 m_normalizedViewport = AABB2::ZERO_TO_ONE;

	int m_playerIndex = 0;
	int m_listenerIndex = 0;
	int m_controllerIndex = -1; // -1 for mouse and keyboard

	void  SetPlayerNormalizedViewport(const AABB2& viewport);
	AABB2 GetPlayerNormalizedViewport() const;
	// AABB2 GetViewport() const; // BL is (0.f, 0.f), TR is (1.f, 1.f)

	float m_moveSpeed = 1.f; // 1 units per second
	float m_turnRate = 90.f; // 90 degrees per second
	float m_floatingSpeed = 2.f; // 2 units per second going upwards or downwards
	float m_sprintModifier = 15.f;
	float m_onePerSprintModifier = 1.f; // 1 / sprint modifier, used to get back to normal speed

	float m_mousePitchSensitiveMultiplier = 0.075f;
	float m_mouseYawSensitiveMultiplier = 0.075f;

	float m_controllerPitchSensitiveMultiplier = 2.1f;
	float m_controllerYawSensitiveMultiplier = 2.1f;
	float m_controllerViewSpeed = 180.f; // 180 degrees per second
	float m_controllerMovementMultiplier = 0.1f;
	float m_controllerRollMultiplier = 12.f;

	Map* m_mapOwner = nullptr;
	Actor* m_projectile = nullptr;

	bool m_freeFlyCamera = false; // false means first person camera, true means free-fly camera

	// hit indicator
	bool m_wasHit = false;
	Timer* m_hitIndicator = nullptr;
	float m_hitIndicateDuration = 3.f;
	Rgba8 m_hitColor = Rgba8::RED_TRANSPARENT;
	Rgba8 m_healColor = Rgba8::RED_CLEAR;
	void PlayerGotHit();

	// HUD
	int m_kills = 0;
	int m_deaths = 0;
	float m_HUDnumFontSize = 50.f;

	float m_lockingSpeed = 300.f;
	float m_lockingLengthOnScreen = 15.f;// distance of radius to snap on the target
	bool m_inLockingAimingMode = false;
	Vec2 m_lockTargetPos = Vec2();
	Vec2 m_lockCursorPos = Vec2(SCREEN_CAMERA_ORTHO_X * 0.5f, SCREEN_CAMERA_ORTHO_Y * 0.5f);
	float m_lockingCircleRadius = 150.f;
	float m_lockingRadiusThickness = 6.f;

	void UpdateLockingWeaponIndicator();
	void RenderScreenCamera() const;
	void RenderHUDAndWeapon() const;

	// listen mode
	bool m_isListening = false;
	float m_listeningDist = 7.5f;
	float m_maxRingRadius = 180.f;
	float m_minRingRadius = 30.f;
	float m_ringThickness = 12.f;

	Timer* m_listeningTimer = nullptr;

	std::vector<Vec2> m_ringPosList;
	std::vector<float> m_ringDistList;
	void DetectNearbyEnemiesInListeningMode();
	void ResetAllActorsBeingListenedStatus();
	void RenderListeningMode() const;

private:
	void UpdateInput(); // Perform input processing for controlling actors and free-fly camera mode
	void UpdateCamera(); // Update our camera settings, taking in to account actor eye height and field of vision
	void UpdateAnimClockForCurrentWeapon(); 
	void Update3DListening(); // update listener info for 3D sound
	void UpdateHitIndicator();
	void SpawnDebugRenderGeometry();
};