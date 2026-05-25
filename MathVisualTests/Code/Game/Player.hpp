#pragma once
#include "Engine/Math/Mat44.hpp"
#include "Engine/Renderer/Camera.hpp"

class Camera;
class Map;
class Actor;

class Player
{
public:
	Player(Vec3 position = Vec3());
	~Player();

	void Startup();
	void Update();
	void Render() const;

	Mat44 GetModelMatrix() const;

	Vec3		m_position;
	Vec3		m_velocity;
	EulerAngles m_orientation;
	Quat		m_quatOrientation;
	EulerAngles m_angularVelocity; // Euler angles per second

	Vec3		m_deltaMovement;

	// but if this is an instance, how should I able to assign other camera to player's camera
	Camera m_playerCamera; // if this is camera*, then when I try to set up set transform will throw an error

	float m_moveSpeed = 2.f; // 2 units per second
	float m_turnRate = 90.f; // 90 degrees per second
	float m_floatingSpeed = 2.f; // 2 units per second going upwards or downwards
	float m_sprintModifier = 10.f;
	float m_onePerSprintModifier = 1.f; // 1 / sprint modifier, used to get back to normal speed

	float m_mousePitchSensitiveMultiplier = 0.025f;
	float m_mouseYawSensitiveMultiplier = 0.025f;

	// Quat m_mousePitchStep = Quat(Vec3(0.f, 1.f, 0.f), PI/180.f);
	// Quat m_mouseYawStep = Quat(0.f, 0.f, 0.05f, 0.f).GetNormalized();

	float m_controllerPitchSensitiveMultiplier = 3.f;
	float m_controllerYawSensitiveMultiplier = 3.f;
	float m_controllerMovementMultiplier = 0.15f;
	float m_controllerRollMultiplier = 12.f;

	bool  m_controllingCamera = true;

	// rotation around center mode
	bool m_rotationMode = false;


private:
	void CalculateTransformAndRoationBasedOnInput();
	void UpdateCameraTransformation();
	void SpawnDebugRenderGeometry();
};