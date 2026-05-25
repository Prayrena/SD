#pragma once
#include "Game/Entity.hpp"
#include "Game/BlockIterator.hpp"
#include "Engine/Renderer/Camera.hpp"

class Camera;

class Player: public Entity
{
public:
	Player();
	virtual ~Player();

	virtual void Startup() override;
	virtual void Update() override;
	virtual void Render() const override;

	Mat44 GetMovementTransformMatrix();

	// but if this is an instance, how should I able to assign other camera to player's camera
	Camera m_playerCamera; // if this is camera*, then when I try to set up set transform will throw an error
	
	float m_moveSpeed = 4.f; // 2 units per second
	float m_turnRate = 90.f; // 90 degrees per second
	float m_floatingSpeed = 4.f; // 2 units per second going upwards or downwards

	float m_mousePitchSensitiveMultiplier = 0.09f;
	float m_mouseYawSensitiveMultiplier = 0.09f;

	float m_controllerPitchSensitiveMultiplier = 3.f;
	float m_controllerYawSensitiveMultiplier = 3.f;
	float m_controllerMovementMultiplier = 0.15f;
	float m_controllerRollMultiplier = 12.f;

	float m_raycastDist = int(float(CHUNK_SIZE_X) * 1.f);
	BlockDef m_buildingBlockDef;

private:
	void PlayerInputControls();
	void UpdateCameraTransformation();
	void SpawnDebugRenderGeometry();

	void GenerateWorldAxesInfrontOfCamera();

	float m_rayDist = 20.f;
};