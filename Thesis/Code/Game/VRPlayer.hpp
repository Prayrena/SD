#pragma once
#include "Engine/Animation/Actor.hpp"
#include "Game/PlayerHand.hpp"

class Skeleton;
class Game;
class Joint;
class Chain;
class VertexBuffer;

namespace physx
{
	class PxMaterial;
	class PxRigidDynamic;
}

//struct GloveModule
//{
//	GloveModule() = default;
//	virtual ~GloveModule() = default;
//
//	virtual void OnGripDown() = 0;
//	virtual void OnGripHeld() = 0;
//	virtual void OnGripReleased() = 0;
//	virtual void Update(float deltaTime) = 0;
//	std::string GetDisplayName() const;
//
//	std::string m_modularName;
//};
//
//class ShieldModule : public GloveModule
//{
//	virtual void OnGripDown() override;
//	virtual void OnGripHeld() override;
//	virtual void OnGripReleased() override;
//	virtual void Update(float deltaTime) override;
//}; 
//
//class ShockWaveModule : public GloveModule
//{
//	virtual void OnGripDown() override;
//	virtual void OnGripHeld() override;
//	virtual void OnGripReleased() override;
//	virtual void Update(float deltaTime) override;
//};

class VRPlayer: public Actor
{
public:
	VRPlayer(Vec3 const& pos = Vec3());
	~VRPlayer(); 

	// GloveModule* m_currentModule = nullptr;
	// std::vector<GloveModule*> m_allModules;

	virtual void Startup() override;
	virtual void Update() override;
	virtual void Render() const override;

	void InitializeBuiltInGloveModules();
	void LoadExternalGloveModules();

	void RenderHands() const;
	void RenderSkeleton() const;

	bool m_debugSkeleton = true;

	// need to do every frame
	void UpdateHands();
	void UpdateMechArms();
	void ControlMechMovement();
	void UpdatePlayerShoulderDisp();
	
	// skeleton settings
	float m_foreArmLen = 1.5f;
	float m_upperArmLen = 1.6f;
	float m_shoulderLen = 0.7f;

	// functions related to mech arms target pos and elbow pos
	Vec3 GetMechHandPosFromPlayerHandAndVirtualShoulder(int sideIndex);
	void SetArmChainPoleVectorByVirtualElbowPos(int sideIndex);

	// chain control
	void UpdateArmChainByHand(Chain* chain, PlayerHand* hand);
	void ControlArmForCombat(int sideIndex);
	Quat SetD6JointDriveAlignWithJointOrientation(Joint* skeletonJoint, Quat const& rotationAdjustMent = Quat());
	void DebugChainByWinPlayer(Chain* chain);
	void ControlArmToMove(int sideIndex);
	void UpdateAnchorPos(int sideIndex);

	float m_maxMovingSpeed = 8.f;
	float m_mechSlideForceMultiplier = 480.f;
	float m_mechBrakeDisp = 0.04f;
	float m_mechBrakeMinSpeed = 0.05f;	// when mech speed is too slow, no need to add damping
	float m_mechBrakeLinearDampingValue = 0.75f;

	// when player grip on it, it records the start pos
	// every frame if player not release it, the mech is going to move in the opposite direction
	std::vector<bool> m_isArmAnchored_current = std::vector<bool>(Side::COUNT, false);
	std::vector<bool> m_isArmAnchored_lastFrame = std::vector<bool>(Side::COUNT, false);
	std::vector<Vec3> m_mechHandAnchorPos = std::vector<Vec3>(Side::COUNT, Vec3::ZERO);
	std::vector<Vec3> m_mechHandDisp = std::vector<Vec3>(Side::COUNT, Vec3::ZERO);					// update and record when player 
	std::vector<Vec3> m_playerHandAnchorPos = std::vector<Vec3>(Side::COUNT, Vec3::ZERO);
	std::vector<Vec3> m_mechArm_current_TargetPos = std::vector<Vec3>(Side::COUNT, Vec3::ZERO);
	std::vector<Vec3> m_mechArm_lastFrame_TargetPos = std::vector<Vec3>(Side::COUNT, Vec3::ZERO);
	std::vector<Vec3> m_mechArmPoleVectorPos = std::vector<Vec3>(Side::COUNT, Vec3::ZERO);
	// calculate the disp of each frame to force to apply on the mech dynamic actor
	// if the disp is too small(player is dragging but not moving) apply a damping effect on the mech dynamic actor

	float m_grabToMoveValue = 0.75f;

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	ObjModel* m_cockpitModel = nullptr;
	// ObjModel* m_screenModel = nullptr; // (if you want it later)

	ObjModel* m_shoulderModel_r = nullptr;
	ObjModel* m_shoulderModel_l = nullptr;

	ObjModel* m_upperArm_Connector_r = nullptr;
	ObjModel* m_upperArm_Gear_r = nullptr;
	ObjModel* m_lowerArm_Gear_r = nullptr;
	ObjModel* m_lowerArm_Connector_r = nullptr;

	ObjModel* m_maceModel = nullptr;

	ObjModel* m_upperArm_Connector_l = nullptr;
	ObjModel* m_upperArm_Gear_l = nullptr;
	ObjModel* m_lowerArm_Gear_l = nullptr;
	ObjModel* m_lowerArm_Connector_l = nullptr;

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	void	ControlMechToRotate();
	void	SwitchBetweenSnapTurnAndSmoothTurn();

	bool	m_snapTurnMode = false;
	float	m_joystickMinToSmoothRotate = 0.05f;
	float	m_smoothTurningRate = 20.f;

	bool	m_hasRotated = false;
	float	m_joystickMinToSnapRotate = 0.3f;
	float	m_rotationDegrees = 30.f;
	float	m_motionRotatingMultiplier = 4.5f;

	float	m_joystickMinToReset = 0.3f;
	Timer*	m_rotationResetTimer = nullptr;
	float	m_rotationResetDuration = .3f;

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	void UpdateDebugInput();

	void UpdateHandPoseTracking(XrPosef handPose, int handIndex);
	void UpdateHeadPoseTracking(Mat44 headPose);

	void		AssignModelsToHands();
	PlayerHand	m_hands[2];
	float		m_handSphereCollisionRadius = 0.3f;

	Mat44		m_headMat;
	Vec3		m_headTrackingPos;
	Quat		m_headTrackingOrientation;

	// the anchor of the player's shoulder should not be related to bottom of the chair
	Vec3		m_disp_actor_shoulder_r = Vec3(-0.1f, -0.16f, 1.4f);
	Vec3		m_disp_actor_shoulder_l = Vec3(-0.1f, 0.16f, 1.4f);
	Vec3		m_shouldRelativeDispToHead_l = Vec3(0.f, 0.16f, 1.4f);
	float		m_playerMaxArmLength = 0.7f;

	Chain* m_arm_l = nullptr;
	Chain* m_arm_r = nullptr;

	// all the displacement are from the headset center to the fixed 
	Vec3	m_humanArmDisp; // headset to human shoulder
	Vec3	m_mechArmDisp;  // headset to mech shoulder

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// physX stuff
	void CreateDynamicRigidBodyForMech();
	void UpdateRigidBodyTransform();
	void UpdateHeadsetCameraPose();

	float m_capsuleHalfHeight = 0.05f;
	float m_capsuleRadius = 0.85f;

	float m_staticFriction = 0.5f;
	float m_dynamicFriction = 0.5f;
	float m_restitution = 0.f;

	float m_totalMass = 1.f;

	// debug collision - this is current not used because we have visualization in physX
	void AddVertsForCollisionDebug();

	// joints
	void CreateJointToConstrainMechBodyMovement();
	physx::PxRigidDynamic* CreateDynamicActorForJoint(std::string jointName);

	float m_joint_capsuleRadius = 0.2f;

	// hand collision
	physx::PxMaterial* m_handMaterial = nullptr;
	float m_handStaticFriction = 0.f;
	float m_handDynamicFriction = 0.f;
	float m_handRestitution = 0.3f;
};