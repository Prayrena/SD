#pragma once
#include "Engine/input/XRInputSystem.hpp"
#include "Engine/Math/Quat.hpp"
#include "Engine/core/Vertex_PCU.hpp"
#include "Engine/core/Timer.hpp"
#include "Game/GameCommon.hpp"
#include <vector>

struct XrPosef;
struct Mat44;
class ObjModel;

class PlayerHand
{
public:
	PlayerHand();
	~PlayerHand();

	void Update();
	void Render() const;
	void UpdateHandPoseInGameWorld();

	void AddVertsForHand();
	std::vector<Vertex_PCU> m_debugVerts;

	Mat44 GetModelMatrix() const;
	Vec3  GetWorldPosInGame() const;

	bool m_overlapWithCube = false;
	bool m_isActive = false;

	float m_grabValue = 0.f;
	int m_handIndex = Side::COUNT;

	Timer* m_vibrationTimer = nullptr;
	float m_amplitude = 0.f;
	void StartControllerVibration(float duration, float amplitude);

	float		m_debugArrowRadius = 0.01f;
	float		m_debugArrowLength = 0.1f;

	Vec2		m_joyStickPos = Vec2::ZERO;		// OpenXR runtimes (like Oculus) apply a built-in dead zone before giving you the joystick values

	Vec3		m_pos;
	Vec3		m_posLastFrame;
	Quat		m_orientation;
	XrPosef		m_actionSpacePose;

	ObjModel* m_model = nullptr;
};