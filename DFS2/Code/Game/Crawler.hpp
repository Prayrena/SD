#pragma once
#include "Engine/Animation/Actor.hpp"
#include "Engine/core/RaycastUtils.hpp"

class	VertexBuffer;
class	IndexBuffer;
class   Chain;
class   SecondOrderDynamics;

enum class CrawlerAnimMode : unsigned char
{
	ADJUSTING,	// linear blending, not doing walking and rotating at all
	ROTATING,	// Multiplicative Blending
	WALKING,	// Multiplicative Blending
	IDLE,		// additive, add the offset of the pelvis moving upside down on top of result
	NUM_MODE
};

struct CrawlerLeg
{
	CrawlerLeg(Chain* c, Vec3 initializePos);
	Chain* m_chain = nullptr;

	Vec3	m_footDefaultOffset = Vec3::ZERO;

	Vec3	m_animCycleStartPos = Vec3::ZERO;
	Vec3	m_animCycleStartPos_rotating = Vec3::ZERO;
	Vec3	m_animCycleStartPos_walking = Vec3::ZERO;
	Quat	m_animCycleStartRootOrientation = Quat();
	Vec3	m_animStayPos = Vec3::ZERO;
	//Vec3	m_animStayPos = Vec3::ZERO;
	//Vec3	m_animStayPos = Vec3::ZERO;
	Vec3	m_animStayPos_rotating = Vec3::ZERO;
	Vec3	m_animStayPos_walking = Vec3::ZERO;
	Quat	m_animStayRootOrientation = Quat();

	Quat	m_legRootDefaultLocalOrientation = Quat();
	Quat	m_legKneeDefaultLocalOrientation = Quat();

	Quat	m_legRootRecordedWorldOrientation = Quat();	// the body forward orientation that the leg remembered
	EulerAngles m_legRecordedForwordOrientation = EulerAngles();

	float	m_chainLength = 0.f;
	bool	m_isGrounded = false;
	bool	m_shouldMove = false;
	bool	m_shouldAdjust = false;
	bool	m_shouldRotate = false;

	Timer* m_walkingTimer = nullptr;
	Timer* m_rotatingTimer = nullptr;
	Timer* m_adjustingTimer = nullptr;

	CrawlerAnimMode m_legLastFrameAnimState = CrawlerAnimMode::IDLE;
	CrawlerAnimMode m_legCurrentAnimState = CrawlerAnimMode::IDLE;
};

// crawler rotate according to the camera's orientation
// the gun adjust up and down based on the result of
// the raycast from the muzzle and raycast result from the camera

//struct AnimState_Weight
//{
//	CrawlerAnimMode m_mode = CrawlerAnimMode::IDLE;
//	float m_weight = 0.f;
//};

class Crawler : public Actor
{
public:
	Crawler(Vec3 const& pos = Vec3());
	~Crawler(); // Actor's constructor is called first, then this

	virtual void Startup() override;
	virtual void Update() override;
	virtual void Render() const override;

	// std::map<int, float> m_animState_WeightMap;

	bool	m_legsShouldStartToWalk = false;

	double m_idleMaxDuration = 0.f;
	double m_adjustMaxDuration = 0.f;
	double m_walkMaxDuration = 0.f;
	double m_rotationMaxDuration = 0.f;

	double m_totalUpdateDuration = 0.f;

	void  SetAnimStateWeight(CrawlerAnimMode animState, float weight);
	float GetAnimStateWeight(CrawlerAnimMode animState);
	float GetAnimStateLastFrameWeight(CrawlerAnimMode animState);

	void  UpdateAnimationBlending();
	void	AlignLegRootJointWithTarget(CrawlerLeg* leg);

	float m_animWeights[static_cast<int>(CrawlerAnimMode::NUM_MODE)] = { 0.f, 0.f, 0.f, 0.f };
	float m_animWeightsLastFrame[static_cast<int>(CrawlerAnimMode::NUM_MODE)] = { 0.f, 0.f, 0.f, 0.f };
	Skeleton* m_animSkeletons[static_cast<int>(CrawlerAnimMode::NUM_MODE)] = {};

	Mat44 GetMovementTransformMatrix();
	Camera*	m_windowsCamera = nullptr;
	Camera*	m_screenCamera = nullptr;

	bool  m_WinCameraIsInControl = true;

	float m_moveSpeed = 3.f; // 1 units per second
	float m_turnRate = 90.f; // 90 degrees per second
	float m_floatingSpeed = 2.f; // 2 units per second going upwards or downwards

	float m_mousePitchSensitiveMultiplier = 0.09f;
	float m_mouseYawSensitiveMultiplier = 0.09f;

	float m_controllerPitchSensitiveMultiplier = 3.f;
	float m_controllerYawSensitiveMultiplier = 3.f;
	float m_controllerMovementMultiplier = 0.15f;
	float m_controllerRollMultiplier = 12.f;

	void	UpdateInput();
	void	UpdateTargetPositionByMouseRaycast();
	void	UpdateDebugRenderMessages();

	// calculate the average position of the foot and check if the average is too far left behind, change its frequency of the timer

	void	ControlMovingTargetByArrowKeys();
	void	CreateBufferForDebugTarget();
	void	RenderDebugTargets() const;
	Mat44	GetModelMatrix_EndEffectorTarget() const;
	Mat44	GetModelMatrix_PoleVector() const;
	float	m_debugRadius = 0.05f;
	Vec3	m_movingTarget_endEffector = Vec3();	// target
	Vec3	m_movingTarget_poleVector = Vec3();	// pole vector
	bool	m_target1_inControl = true;

	Vec3	m_debugAxis = Vec3();
	Vec3	m_debugJointPos = Vec3();	// A
	Vec3	m_debugJoint2Pos = Vec3();	// C
	float   m_debugAngle = 0.f;
	bool	m_solveTarget = false;
	Rgba8	m_movingTargetColor = Rgba8::CYSTAL_BLUE;
	Rgba8	m_movingPolveVectorColor = Rgba8::CANDLE_YELLOW;

	Chain*			m_debugChain = nullptr;
	unsigned int	m_addingCounter = 1;

	std::vector<Vertex_PCU> m_debugVerts_endEffectorTarget;
	VertexBuffer*	m_movingTargetVertexBuffer = nullptr;

	std::vector<Vertex_PCU> m_debugVerts_PoleVector;
	VertexBuffer* m_movingTarget2VertexBuffer = nullptr;


	void CreateCrawlerSkeleton();

	// Skeleton* m_defaultPoseSkeleton = nullptr;

	Timer*		m_FPSTimer = nullptr;
	std::string m_FPSString;

	// raycast debug - use raycast target on ground
	Vec3	m_raycastedTarget;
	bool	m_targetLocked = false;

	// control debug - use arrow keys to move the sphere
	std::vector<CrawlerLeg*> m_legs;

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// procedural animation
	void UpdateAnimationState();
	void UpdateAnimationStateMachine();
	void UpdateToeGroundedStatus();
	bool AreAllLegsGrounded();
	void UpdateIfLegShouldMove();
	bool CheckIfCrawlerNeedsToAdjustPose();

	bool CheckIfLegShouldRotate();

	Vec3 CalculateLegWalkStepByCrawlerVelocity(CrawlerLeg* leg, bool IsPredicting = false);
	int	 GetNumLegsShouldMove();

	Timer*  m_idleTimer = nullptr;
	float	m_idlePeroid = 999.f;
	float	m_idleHeightOffset = 0.05f;
	float	m_pelvisHeight = 0.6f;
	float	m_legBoneLength = 0.635f;
	float	m_footBoneLength = 0.35f;

	float	m_walkingTimerPeroid_max = 0.5f;
	float	m_walkingTimerPeroid_min = 0.25f; // 0.15f;
	float	m_walkingStep = 0.5f;

	float	m_adjustingTimerPeroid = 0.45f;

	EulerAngles m_neckJointCurrentOrientation = EulerAngles();
	float m_neckRotationSpeed = 75.f;			// degrees/s
	float m_legRotationAngleBoundary = 25.f;	// the max rotation a leg going to make 

	bool  m_rotatingCounterClockwise = false;
	float m_rotatingPeroid = 0.3f;

	CrawlerAnimMode m_crawlerCurrentFrameState = CrawlerAnimMode::IDLE;
	CrawlerAnimMode m_crawlerLastFrameState = CrawlerAnimMode::IDLE;

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// crawler settings
	float m_maxMovingSpeed =  3.f; // 6.f;

	Vec3 m_inputPos;
	SecondOrderDynamics* m_secondOrderModifierOnY = nullptr;
	SecondOrderDynamics* m_secondOrderModifier = nullptr;

	float m_maxPredictionDist = 0.3f;

	Quat m_neckQuatLastFrame = Quat();

	bool m_isDriving = false;
	bool m_bodyNeedsRotating = false;
};