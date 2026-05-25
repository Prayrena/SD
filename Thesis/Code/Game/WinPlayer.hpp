#pragma once
#include "Engine/Animation/Actor.hpp"
#include "Engine/core/RaycastUtils.hpp"

class	VertexBuffer;
class	IndexBuffer;
class   Chain;

class WinPlayer : public Actor
{
public:
	WinPlayer(Vec3 const& pos = Vec3());
	~WinPlayer(); // Actor's constructor is called first, then this

	virtual void Startup() override;
	virtual void Update() override;
	virtual void Render() const override;

	Mat44 GetMovementTransformMatrix();
	Camera*	m_windowsCamera = nullptr;
	Camera*	m_screenCamera = nullptr;

	bool  m_WinCameraIsInControl = true;

	float m_moveSpeed = 1.f; // 1 units per second
	float m_turnRate = 90.f; // 90 degrees per second
	float m_floatingSpeed = 2.f; // 2 units per second going upwards or downwards

	float m_mousePitchSensitiveMultiplier = 0.09f;
	float m_mouseYawSensitiveMultiplier = 0.09f;

	float m_controllerPitchSensitiveMultiplier = 3.f;
	float m_controllerYawSensitiveMultiplier = 3.f;
	float m_controllerMovementMultiplier = 0.15f;
	float m_controllerRollMultiplier = 12.f;

	void	UpdateInput();
	void	ControlActorByInput();
	void	UpdateTargetPositionByMouseRaycast();
	void	UpdateDebugRenderMessages();

	void	ControlMovingTargetByArrowKeys();
	void	CreateBufferForDebugTarget();
	void	RenderDebugTargets() const;
	Mat44	GetModelMatrix_EndEffectorTarget() const;
	Mat44	GetModelMatrix_PoleVector() const;
	float	m_debugRadius = 0.05f;
	Vec3	m_movingTarget_endEffector = Vec3();	// target
	Vec3	m_movingTarget_poleVector = Vec3();	// pole vector
	bool	m_target1_inControl = true;

	bool	m_showDebugInterface = true;

	Vec3	m_debugAxis = Vec3();
	Vec3	m_debugJointPos = Vec3();	// A
	Vec3	m_debugJoint2Pos = Vec3();	// C
	float   m_debugAngle = 0.f;
	bool	m_solveTarget = false;
	Rgba8	m_movingTargetColor = Rgba8::PURPLE_BLUE;
	Rgba8	m_movingPolveVectorColor = Rgba8::DEEP_ORANGE;

	Chain*			m_debugChain = nullptr;
	unsigned int	m_addingCounter = 1;

	std::vector<Vertex_PCU> m_debugVerts_endEffectorTarget;
	VertexBuffer*	m_movingTargetVertexBuffer = nullptr;

	std::vector<Vertex_PCU> m_debugVerts_PoleVector;
	VertexBuffer* m_movingTarget2VertexBuffer = nullptr;

	// RaycastResult3D RaycastFromCameraToMouseToMap

	Timer*		m_FPSTimer = nullptr;
	std::string m_FPSString;

	// raycast debug - use raycast target on ground
	Vec3	m_raycastedTarget;
	bool	m_targetLocked = false;

	// control debug - use arrow keys to move the sphere
};