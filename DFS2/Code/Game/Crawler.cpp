#include "Engine/core/Clock.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/Window.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/DebugRender.hpp"
#include "Engine/Renderer/DebugRenderGeometry.hpp"
#include "Engine/Math/OpenXRMathUtils.hpp"
#include "Engine/Model/ObjModel.hpp"
#include "Engine/Animation/Skeleton.hpp"
#include "Engine/Animation/ProceduralAnimation/IKTwoBonesSolver.hpp"
#include "Engine/Math/SecondOrderDynamics.hpp"
#include "ThirdParty/Physx/include/PxPhysicsAPI.h"
#include "Engine/Math/PhysXMathUtils.hpp"
#include "Engine/core/Time.hpp"
#include "Game/Crawler.hpp"
#include "Game/Game.hpp"
#include "Game/Player.hpp"
#include <vector>
#include <stdio.h>

extern Game* g_theGame;
extern InputSystem* g_theInput;
extern Window* g_theWindow;
extern Clock* g_theGameClock;

using namespace std;
using namespace physx;

CrawlerLeg::CrawlerLeg(Chain* c, Vec3 initializePos)
		: m_chain(c)
		, m_footDefaultOffset(initializePos)
{
}

Crawler::Crawler(Vec3 const& pos /*= Vec3()*/)
		: Actor(pos)
{

}

Crawler::~Crawler()
{
	if (m_movingTargetVertexBuffer)
	{
		delete m_movingTargetVertexBuffer;
		m_movingTargetVertexBuffer = nullptr;
	}
}

void Crawler::Startup()
{
	// set window camera
	Vec3 cameraPos = Vec3(2.f, 0.f, 0.f);
	EulerAngles playerStartRotation = EulerAngles(0.f, 0.f, 0.f);

	m_position = cameraPos;
	m_orientation = playerStartRotation;

	m_idleTimer = new Timer(m_idlePeroid);
	m_idleTimer->Start();

	CreateCrawlerSkeleton();
	
	m_speed = m_maxMovingSpeed;

	m_neckJointCurrentOrientation = m_orientation;

	m_secondOrderModifierOnY = new SecondOrderDynamics(0.6f, 0.2f, -.5f, Vec3::ZERO);
	m_secondOrderModifier = new SecondOrderDynamics(1.5f, 1.3f, -.5f, Vec3::ZERO);
	// m_secondOrderModifierOnY = new SecondOrderDynamics(0.6f, 0.5f, -2.f, Vec3::ZERO);
	// m_secondOrderModifierOnX = new SecondOrderDynamics(0.6f, 0.5f, -2.f, Vec3::ZERO);
}

void Crawler::CreateCrawlerSkeleton()
{
	// create skeleton
	Actor* actor = static_cast<Actor*>(this);
	m_skeleton = new Skeleton("CrawlerSkeleton", actor);

	// create all chains
	Quat quat_0;
	Quat quat_15 = Quat::CreateRotationAroundZAxis(15.f);
	Quat quat_30 = Quat::CreateRotationAroundZAxis(30.f);
	Quat quat_45 = Quat::CreateRotationAroundZAxis(45.f);
	Quat quat_90 = Quat::CreateRotationAroundZAxis(90.f);
	Quat quat_N90 = Quat::CreateRotationAroundZAxis(-90.f);
	Quat quat_Y_N90 = Quat::CreateRotationAroundYAxis(-90.f);

	Joint* rootJoint = m_skeleton->AddJoint("root", Vec3(), quat_Y_N90);
	Joint* pelvisJoint = m_skeleton->AddJoint("pelvis", Vec3(m_pelvisHeight, 0.f, 0.f), ConvertEulerAnglesToQuat(EulerAngles(0.f, 90.f, 0.f)), rootJoint);
	Joint* neckJoint = m_skeleton->AddJoint("neck", Vec3(0.f, 0.f, 0.32f), Quat(), pelvisJoint);
	Joint* headJoint = m_skeleton->AddJoint("head", Vec3(-0.165f, 0.f, 0.f), Quat(), neckJoint);

	Joint* hip_front_L = m_skeleton->AddJoint("hip_front_L", Vec3(0.3f, 0.3f, 0.f), ConvertEulerAnglesToQuat(EulerAngles(45.f, 30.f, 0.f)), pelvisJoint);
	Joint* hip_front_R = m_skeleton->AddJoint("hip_front_R", Vec3(0.3f, -0.3f, 0.f), ConvertEulerAnglesToQuat(EulerAngles(-45.f, 30.f, 0.f)), pelvisJoint);

	Joint* hip_back_L = m_skeleton->AddJoint("hip_back_L", Vec3(-0.3f, 0.3f, 0.f), ConvertEulerAnglesToQuat(EulerAngles(135.f, 30.f, 0.f)), pelvisJoint);	// 90 + 30 
	Joint* hip_back_R = m_skeleton->AddJoint("hip_back_R", Vec3(-0.3f, -0.3f, 0.f), ConvertEulerAnglesToQuat(EulerAngles(-135.f, 30.f, 0.f)), pelvisJoint); // -90 - 30 

	Joint* leg_upper_FL = m_skeleton->AddJoint("leg_upper_FL", Vec3(m_legBoneLength, 0.f, 0.f), ConvertEulerAnglesToQuat(EulerAngles(0.f, 60.f, 0.f)), hip_front_L);
	Joint* leg_upper_FR = m_skeleton->AddJoint("leg_upper_FR", Vec3(m_legBoneLength, 0.f, 0.f), ConvertEulerAnglesToQuat(EulerAngles(0.f, 60.f, 0.f)), hip_front_R);

	Joint* leg_upper_BL = m_skeleton->AddJoint("leg_upper_BL", Vec3(m_legBoneLength, 0.f, 0.f), ConvertEulerAnglesToQuat(EulerAngles(0.f, 60.f, 0.f)), hip_back_L);
	Joint* leg_upper_BR = m_skeleton->AddJoint("leg_upper_BR", Vec3(m_legBoneLength, 0.f, 0.f), ConvertEulerAnglesToQuat(EulerAngles(0.f, 60.f, 0.f)), hip_back_R);

	Joint* foot_FL = m_skeleton->AddJoint("foot_FL", Vec3(m_footBoneLength, 0.f, 0.f), ConvertEulerAnglesToQuat(EulerAngles(0.f, 0.f, 0.f)), leg_upper_FL);
	Joint* foot_FR = m_skeleton->AddJoint("foot_FR", Vec3(m_footBoneLength, 0.f, 0.f), ConvertEulerAnglesToQuat(EulerAngles(0.f, 0.f, 0.f)), leg_upper_FR);

	Joint* foot_BL = m_skeleton->AddJoint("foot_BL", Vec3(m_footBoneLength, 0.f, 0.f), ConvertEulerAnglesToQuat(EulerAngles(0.f, 0.f, 0.f)), leg_upper_BL);
	Joint* foot_BR = m_skeleton->AddJoint("foot_BR", Vec3(m_footBoneLength, 0.f, 0.f), ConvertEulerAnglesToQuat(EulerAngles(0.f, 0.f, 0.f)), leg_upper_BR);

	// create four leg chains
	TwoJointsIKSolver* IKSolver_leg_FL = new TwoJointsIKSolver(hip_front_L, leg_upper_FL, foot_FL);
	IKSolver_leg_FL->m_usePoleVector = true;
	Chain* chain_leg_FL = new Chain(IKSolver_leg_FL);

	TwoJointsIKSolver* IKSolver_leg_FR = new TwoJointsIKSolver(hip_front_R, leg_upper_FR, foot_FR);
	IKSolver_leg_FR->m_usePoleVector = true;
	Chain* chain_leg_FR = new Chain(IKSolver_leg_FR);

	TwoJointsIKSolver* IKSolver_leg_BL = new TwoJointsIKSolver(hip_back_L, leg_upper_BL, foot_BL);
	IKSolver_leg_BL->m_usePoleVector = true;
	Chain* chain_leg_BL = new Chain(IKSolver_leg_BL);

	TwoJointsIKSolver* IKSolver_leg_BR = new TwoJointsIKSolver(hip_back_R, leg_upper_BR, foot_BR);
	IKSolver_leg_BR->m_usePoleVector = true;
	Chain* chain_leg_BR = new Chain(IKSolver_leg_BR);

	// after initialize the default position, record them as default foot offset
	Vec3 FL_offset = foot_FL->GetWorldPosition() - rootJoint->GetWorldPosition();
	Vec3 FR_offset = foot_FR->GetWorldPosition() - rootJoint->GetWorldPosition();
	Vec3 BL_offset = foot_BL->GetWorldPosition() - rootJoint->GetWorldPosition();
	Vec3 BR_offset = foot_BR->GetWorldPosition() - rootJoint->GetWorldPosition();

	// create and store all crawler legs
	CrawlerLeg* leg_FL = new CrawlerLeg(chain_leg_FL, FL_offset);
	CrawlerLeg* leg_FR = new CrawlerLeg(chain_leg_FR, FR_offset);
	CrawlerLeg* leg_BL = new CrawlerLeg(chain_leg_BL, BL_offset);
	CrawlerLeg* leg_BR = new CrawlerLeg(chain_leg_BR, BR_offset);

	m_legs.push_back(leg_FL);
	m_legs.push_back(leg_BR);
	m_legs.push_back(leg_FR);
	m_legs.push_back(leg_BL);

	// solve the legs to get initial default leg offset
	for (int legIndex = 0; legIndex < m_legs.size(); ++legIndex)
	{
		CrawlerLeg* leg = m_legs[legIndex];
		Chain* chain = m_legs[legIndex]->m_chain;

		leg->m_chainLength = leg->m_chain->GetTotalBoneLength();

		Vec3 currentFootGroundPos = chain->GetEndJoint()->GetWorldPosition();
		currentFootGroundPos.z = 0.f;
		chain->m_solver->SetSolverTarget(currentFootGroundPos);
		chain->m_solver->SetPoleVector(currentFootGroundPos + Vec3(0.f, 0.f, -1.f));
		chain->m_solver->SolveIK();

		leg->m_footDefaultOffset = chain->GetEndJoint()->GetWorldPosition() - m_position;
		leg->m_legRootDefaultLocalOrientation = chain->GetRootJoint()->GetLocalQuat();
		leg->m_legKneeDefaultLocalOrientation = chain->GetMidJoint()->GetLocalQuat();
		leg->m_legRootRecordedWorldOrientation = chain->GetRootJoint()->GetWorldQuat();
		leg->m_legRecordedForwordOrientation = m_orientation;
	}

	m_skeleton->Startup();
	// m_skeleton->m_drawSkeletonDebug = true;

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// load models
	ObjModel* footModel = new ObjModel();
	footModel->LoadXml("Data/Models/Crawler_Foot.xml");
	footModel->m_color = Rgba8::WHITE;

	leg_upper_FL->m_objModels.push_back(footModel);
	leg_upper_FR->m_objModels.push_back(footModel);
	leg_upper_BL->m_objModels.push_back(footModel);
	leg_upper_BR->m_objModels.push_back(footModel);

	ObjModel* legModel = new ObjModel();
	legModel->LoadXml("Data/Models/Crawler_Leg.xml");
	legModel->m_color = Rgba8::WHITE;

	hip_front_L->m_objModels.push_back(legModel);
	hip_front_R->m_objModels.push_back(legModel);
	hip_back_L->m_objModels.push_back(legModel);
	hip_back_R->m_objModels.push_back(legModel);

	ObjModel* baseModel = new ObjModel();
	baseModel->LoadXml("Data/Models/Crawler_Base.xml");
	baseModel->m_color = Rgba8::WHITE;

	pelvisJoint->m_objModels.push_back(baseModel);

	ObjModel* headModel = new ObjModel();
	headModel->LoadXml("Data/Models/Crawler_Head.xml");
	headModel->m_color = Rgba8::WHITE;

	neckJoint->m_objModels.push_back(headModel);

	ObjModel* gunModel = new ObjModel();
	gunModel->LoadXml("Data/Models/Crawler_Gun.xml");
	gunModel->m_color = Rgba8::WHITE;

	headJoint->m_objModels.push_back(gunModel);
}

void Crawler::UpdateAnimationState()
{
	// update animation status
	// the crawler will make rotating be a priority
	// if (m_crawlerCurrentFrameState == CrawlerAnimMode::ROTATING)
	// {
	// 	if (CheckIfLegShouldRotate() || m_bodyNeedsRotating)	// if the crawler is rotating, no need to move other parts
	// 	{
	// 		return;
	// 	}
	// }
	// 
	constexpr float minSpeedThreshold = 0.001f; // m/s
	// // this is when not using animation blending
	// if (m_velocity.GetLengthSquared() > (minSpeedThreshold * minSpeedThreshold) || m_isDriving)
	// {
	// 	m_crawlerCurrentFrameState = CrawlerAnimMode::WALKING;
	// }
	// else if (CheckIfCrawlerNeedsToAdjustPose())
	// {
	// 	m_crawlerCurrentFrameState = CrawlerAnimMode::ADJUSTING;
	// }
	// else
	// {
	// 	m_crawlerCurrentFrameState = CrawlerAnimMode::IDLE;
	// }

	CheckIfLegShouldRotate();
	// when using anim blending
	if (m_velocity.GetLengthSquared() > (minSpeedThreshold * minSpeedThreshold) || m_isDriving)
	{
		SetAnimStateWeight(CrawlerAnimMode::WALKING, 1.f);
		SetAnimStateWeight(CrawlerAnimMode::ADJUSTING, 0.f);
	}
	else
	{
		SetAnimStateWeight(CrawlerAnimMode::WALKING, 0.f);

		if (CheckIfCrawlerNeedsToAdjustPose())
		{
			SetAnimStateWeight(CrawlerAnimMode::ADJUSTING, 1.f);
		}
		else
		{
			SetAnimStateWeight(CrawlerAnimMode::ADJUSTING, 0.f);
		}
	}

	// let's keep the weight of the idle to be 1 all the time
	if (GetAnimStateWeight(CrawlerAnimMode::ADJUSTING) == 0.f && GetAnimStateWeight(CrawlerAnimMode::ROTATING) == 0.f && GetAnimStateWeight(CrawlerAnimMode::WALKING) == 0.f)
	{
		SetAnimStateWeight(CrawlerAnimMode::IDLE, 1.f);
	}
	else
	{
		SetAnimStateWeight(CrawlerAnimMode::IDLE, 0.f);
	}

	// for any given time, should the sum of all the animation state up to 1.f?
	// or, say each state based on it current status, like input value, speed range fraction, ending timer fraction(every leg takes 0.75s to adjust, overall is 3s)
	// the sum up of all the weights are over 1.f and we are blending them in order by value comparison
	// like say 0.5f walking, 1.f rotation, adjust 0.5, idle 1.f, what is the blending order and t value for each Nlerp?
}

void Crawler::AlignLegRootJointWithTarget(CrawlerLeg* leg)
{
	Joint* A = leg->m_chain->GetRootJoint();
	Joint* B = leg->m_chain->GetMidJoint();
	Joint* C = leg->m_chain->GetEndJoint();

	Vec3 A_pos = A->GetWorldPosition();
	Vec3 B_pos = B->GetWorldPosition();
	Vec3 C_pos = C->GetWorldPosition();
	Vec3& target = leg->m_chain->m_solver->m_target;
	Vec3 worldUp = Vec3(0.f, 0.f, 1.f);

	// Compute ABC plane normal
	Vec3 AB_dir = (B_pos - A_pos).GetNormalized();
	Vec3 AC_dir = (C_pos - A_pos).GetNormalized();

	// Project the new target direction onto horizontal plane
	Vec3 AT_dir = (target - A_pos).GetNormalized();
	Quat rotationAroundWorldUp = Quat::GetQuatFromTwoVectors(AC_dir, AT_dir);

	Quat localRotation = A->GetWorldQuat().ConvertWorldRotationToLocal(rotationAroundWorldUp);
	A->m_localRotation = (A->m_localRotation * localRotation).GetNormalized();

	B->m_localRotation = leg->m_legKneeDefaultLocalOrientation;
}


void Crawler::UpdateAnimationBlending()
{
	// todo: based on weight of the rotating and walking, readjust the weight to make it the sum of the weight become 1
	float walkingWeight = GetAnimStateWeight(CrawlerAnimMode::WALKING);
	float rotatingWeight = GetAnimStateWeight(CrawlerAnimMode::ROTATING);
	bool isBlending = false;
	if (walkingWeight != 0.f && rotatingWeight != 0.f)
	{
		isBlending = true;
	}

	if (walkingWeight + rotatingWeight > 1.f)
	{
		float weightSum = walkingWeight + rotatingWeight;
		SetAnimStateWeight(CrawlerAnimMode::WALKING, walkingWeight / weightSum);
		SetAnimStateWeight(CrawlerAnimMode::ROTATING, rotatingWeight / weightSum);
		
		// SetAnimStateWeight(CrawlerAnimMode::WALKING, 1.f);
		// SetAnimStateWeight(CrawlerAnimMode::ROTATING, 0.f);
	}

	// if (GetAnimStateWeight(CrawlerAnimMode::WALKING) != 0.f || GetAnimStateWeight(CrawlerAnimMode::ROTATING) != 0.f ||GetAnimStateWeight(CrawlerAnimMode::ADJUSTING) != 0.f )
	// {
	// 	SetAnimStateWeight(CrawlerAnimMode::IDLE, 0.f);
	// }

	// based on current skeleton, we blend current pose with new animation pose
	for (int i = 0; i < (int)CrawlerAnimMode::NUM_MODE; ++i)
	{
		if( i == int(CrawlerAnimMode::IDLE))
		{
			double timeAtStart = GetCurrentTimeSeconds();

			if (!m_idleTimer)
			{
				m_idleTimer = new Timer(m_idlePeroid);
				m_idleTimer->Start();
			}

			if (GetAnimStateLastFrameWeight(CrawlerAnimMode::IDLE) == 0.f)
			{
				m_idleTimer->Restart();
				continue;
			}

			// set the pelvis up and down by sin for acting breathing anim
			if (m_idleTimer->HasPeroidElapsed())
			{
				m_idleTimer->Start();
			}
			float heightOffsetMultipler = m_idleHeightOffset * sinf(m_idleTimer->GetElapsedTime() * 2.f) + 1.f;

			Joint* pelvis = m_skeleton->GetJointByName("pelvis");

			pelvis->SetLocalBonePos(Vec3(m_pelvisHeight * heightOffsetMultipler, 0.f, 0.f));

			for (int legIndex = 0; legIndex < m_legs.size(); ++legIndex)
			{
				CrawlerLeg* leg = m_legs[legIndex];
				Vec3 footWorldPos = m_inputPos + m_orientation.GetAsMatrix_XFwd_YLeft_ZUp().TransformVectorQuantity3D(leg->m_footDefaultOffset);
				footWorldPos.z = 0.f;

				Chain* chain = m_legs[legIndex]->m_chain;
				chain->m_solver->SetSolverTarget(footWorldPos);

				chain->m_solver->SetPoleVector(footWorldPos + Vec3(0.f, 0.f, -1.f));

				chain->m_solver->SolveIK();
			}

			double timeAtEnd = GetCurrentTimeSeconds();
			double timeElapsed = timeAtEnd - timeAtStart;

			m_idleMaxDuration = max(m_idleMaxDuration, timeElapsed);
		}

		else if (i == int(CrawlerAnimMode::ROTATING))
		{
			double timeAtStart = GetCurrentTimeSeconds();

			// rotate the mech orientation
			EulerAngles desiredOrientation = g_theGame->m_player->m_orientation;
			desiredOrientation.m_pitchDegrees = 0.f;
			desiredOrientation.m_rollDegrees = 0.f;

			float deltaSeconds = g_theGameClock->GetDeltaSeconds();

			m_orientation.m_yawDegrees = GetTurnedTowardDegrees(m_orientation.m_yawDegrees, desiredOrientation.m_yawDegrees, deltaSeconds * m_neckRotationSpeed * 1.5f);
			
			float angleDiff = abs(desiredOrientation.m_yawDegrees - m_orientation.m_yawDegrees);
			float fraction = GetFractionWithinRange(angleDiff, 0.f, 90.f);
			rotatingWeight = RangeMapClamped(fraction, 0.f, 1.f, 0.f, GetAnimStateWeight(CrawlerAnimMode::ROTATING));

			// if in current frame we switch to rotating animation state from other states
			// we'll set up its timer and stay pos
			if (GetAnimStateLastFrameWeight(CrawlerAnimMode::ROTATING) == 0.f)
			{
				for (int legIndex = 0; legIndex < m_legs.size(); ++legIndex)
				{
					CrawlerLeg* leg = m_legs[legIndex];

					if (leg->m_rotatingTimer)
					{
						leg->m_rotatingTimer->Stop();
					}
					else
					{
						leg->m_rotatingTimer = new Timer(m_rotatingPeroid);
					}
					 
					// Joint* j = leg->m_chain->GetEndJoint();
					// leg->m_animStayPos_rotating = j->GetWorldPosition();
					// leg->m_animStayPos_rotating.z = 0.f;
				}

				continue;
			}

			if (isBlending)
			{
				continue;
			}

			// for each leg, if the leg should move and the moving legs nums is <= half
			// allow it to move the cycle
			// int num = GetNumLegsShouldMove();
			int numOfLegsAllowToMove = 2;

			for (int legIndex = 0; legIndex < m_legs.size(); ++legIndex)
			{
				CrawlerLeg* leg = m_legs[legIndex];
				Timer* timer = leg->m_rotatingTimer;
				float rotatingFraction = timer->GetElapsedFraction();

				if ((rotatingFraction > 0.f && rotatingFraction < 1.f))
				{
					--numOfLegsAllowToMove;
				}
			}

			for (int legIndex = 0; legIndex < m_legs.size(); ++legIndex)
			{
				CrawlerLeg* leg = m_legs[legIndex];
				Chain* chain = m_legs[legIndex]->m_chain;
				Timer* timer = m_legs[legIndex]->m_rotatingTimer;
				// Timer* walkingTimer = m_legs[legIndex]->m_walkingTimer;
				Joint* legRootJoint = chain->GetRootJoint();
				// if this leg is allowed to move or this leg is already moving
				// only if the timer has not finished
				float rotatingFraction = timer->GetElapsedFraction();

				bool permissionToMove = false;
				// if (isBlending)
				// {
				// 	permissionToMove = (!walkingTimer->IsStopped() && ((numOfLegsAllowToMove > 0 && leg->m_shouldRotate) || (rotatingFraction > 0.f && rotatingFraction < 1.f)) );
				// }
				// else
				// {
				// }
				permissionToMove = ((numOfLegsAllowToMove > 0 && timer->IsStopped() && leg->m_shouldRotate) || (rotatingFraction > 0.f && rotatingFraction < 1.f));

				if (permissionToMove)
				{

					// if the leg just started to move, record where it starts to move
					if (timer->IsStopped())
					{
						timer->Start();
						leg->m_animCycleStartPos_rotating = leg->m_chain->GetEndJoint()->GetWorldPosition();
						leg->m_animStayRootOrientation = leg->m_chain->GetRootJoint()->m_localRotation;
						leg->m_animStayPos_rotating.z = 0.f;
						--numOfLegsAllowToMove;
					}

					Vec3 newFootPos;

					// based on timer's fraction, lerp the foot position and leg root rotation
					leg->m_chain->GetRootJoint()->m_localRotation = Quat::Nlerp(leg->m_animStayRootOrientation, leg->m_legRootDefaultLocalOrientation, rotatingFraction);

					// horizontal
					Vec3 footExpectedPos = m_position + m_orientation.GetAsMatrix_XFwd_YLeft_ZUp().TransformVectorQuantity3D(leg->m_footDefaultOffset);
					Vec3 footOffsetHorizontal = Interpolate(leg->m_animCycleStartPos_rotating, footExpectedPos, rotatingFraction);
					// if (isBlending && !leg->m_walkingTimer->IsStopped())
					// {
					// 	footOffsetHorizontal = Interpolate(leg->m_animCycleStartPos_walking, footExpectedPos, rotatingFraction);
					// }
					// else
					// {
					// 	footOffsetHorizontal = Interpolate(leg->m_animCycleStartPos_rotating, footExpectedPos, rotatingFraction);
					// }

					// upward
					float cycleFraction = RangeMapClamped(rotatingFraction, 0.f, 1.f, 0.f, PI);
					Vec3 footOffsetUpward = Vec3(0.f, 0.f, sinf(cycleFraction) * 0.3f);

					// string debugStr = Stringf("disp: %.2f", footOffsetUpward.z);
					// DebugAddScreenText(debugStr, Vec2(), 20.f, Vec2(0.5f, 0.5f), -1.f, Rgba8::WHITE);

					newFootPos = footOffsetHorizontal + footOffsetUpward;

					chain->m_solver->SetSolverTarget(newFootPos);
					chain->m_solver->SetPoleVector(newFootPos + Vec3(0.f, 0.f, -1.f));
					AlignLegRootJointWithTarget(leg);
					chain->m_solver->SolveIK();

					// DebugAddWorldWireSphere(newFootPos, 0.05f, m_walkingTimerPeroid_max * 0.3f, Rgba8::BLUE, Rgba8::BLUE, DebugRenderMode::X_RAY);
				}
				else
				{
					// if the timer has finished, update the leg forward orientation
					if (timer->HasPeroidElapsed())
					{
						leg->m_legRecordedForwordOrientation = m_orientation;
						leg->m_legRootRecordedWorldOrientation = legRootJoint->GetWorldQuat();
					}

					// if the leg is not permitted to move yet
					// the leg root need to change local rotation, in order to maintain the m_recordedBodyOrientation
					leg->m_rotatingTimer->Stop();
					leg->m_legCurrentAnimState = CrawlerAnimMode::IDLE;

					// keep the root to remain its world rotation
					legRootJoint->m_localRotation = legRootJoint->m_parent->GetWorldQuat().GetInversed() * leg->m_legRootRecordedWorldOrientation;

					Vec3 footExpectedPos = m_position + leg->m_legRecordedForwordOrientation.GetAsMatrix_XFwd_YLeft_ZUp().TransformVectorQuantity3D(leg->m_footDefaultOffset);

					// solve by the stay pos
					chain->m_solver->SetSolverTarget(footExpectedPos);
					chain->m_solver->SetPoleVector(footExpectedPos + Vec3(0.f, 0.f, -1.f));
					AlignLegRootJointWithTarget(leg);
					chain->m_solver->SolveIK();

					// DebugAddWorldWireSphere(footExpectedPos, 0.05f, m_walkingTimerPeroid_max * 0.3f, Rgba8::SALMON_PINK, Rgba8::SALMON_PINK, DebugRenderMode::X_RAY);
					// if (leg->m_walkingTimer->IsStopped())
					// {
					// }
					// else
					// {
					// 	chain->m_solver->SetSolverTarget(footExpectedPos);
					// 	chain->m_solver->SetPoleVector(footExpectedPos + Vec3(0.f, 0.f, -1.f));
					// 	chain->m_solver->SolveIKWithWeight(GetAnimStateWeight(CrawlerAnimMode::ROTATING));
					// }
				}
			}

			double timeAtEnd = GetCurrentTimeSeconds();
			double timeElapsed = timeAtEnd - timeAtStart;

			m_rotationMaxDuration = max(m_rotationMaxDuration, timeElapsed);
		}

		else if (i == int(CrawlerAnimMode::ADJUSTING))
		{
			double timeAtStart = GetCurrentTimeSeconds();

			// if in current frame we switch to walking animation state from other states
			// we'll set up its timer and stay pos
			if (GetAnimStateLastFrameWeight(CrawlerAnimMode::ADJUSTING) == 0.f)
			{
				for (int legIndex = 0; legIndex < m_legs.size(); ++legIndex)
				{
					CrawlerLeg* leg = m_legs[legIndex];

					if (leg->m_adjustingTimer)
					{
						leg->m_adjustingTimer->Stop();
					}
					else
					{
						leg->m_adjustingTimer = new Timer(m_adjustingTimerPeroid);
					}

					// Joint* j = leg->m_chain->GetEndJoint();
					// leg->m_animStayPos = j->GetWorldPosition();
				}
			}

			if (GetAnimStateWeight(CrawlerAnimMode::ADJUSTING) == 0.f)
			{
				continue;
			}

			// for each leg, if the leg should move and the moving legs nums is <= half
			// allow it to move the cycle
			// int num = GetNumLegsShouldMove();
			int numOfLegsAllowToMove = 2;

			for (int legIndex = 0; legIndex < m_legs.size(); ++legIndex)
			{
				CrawlerLeg* leg = m_legs[legIndex];
				Timer* timer = leg->m_adjustingTimer;
				float adjustingFraction = timer->GetElapsedFraction();
				if ((adjustingFraction > 0.f && adjustingFraction < 1.f))
				{
					--numOfLegsAllowToMove;
				}
			}

			for (int legIndex = 0; legIndex < m_legs.size(); ++legIndex)
			{
				CrawlerLeg* leg = m_legs[legIndex];
				Chain* chain = m_legs[legIndex]->m_chain;
				Timer* timer = leg->m_adjustingTimer;
				// if this leg is allowed to move or this leg is already moving
				// only if the timer has not finished
				float adjustingFraction = timer->GetElapsedFraction();
				if ((numOfLegsAllowToMove > 0 && adjustingFraction == 0.f && leg->m_shouldAdjust) || (adjustingFraction > 0.f && adjustingFraction < 1.f))
				{

					// if the leg just started to move, record where it starts to move
					if (adjustingFraction == 0.f)
					{
						timer->Start();
						leg->m_animCycleStartPos = leg->m_chain->GetEndJoint()->GetWorldPosition();
						leg->m_animStayRootOrientation = leg->m_chain->GetRootJoint()->m_localRotation;
						leg->m_animStayPos.z = 0.f;
						--numOfLegsAllowToMove;
					}

					Vec3 newFootPos;

					// based on timer's fraction, lerp the foot position and leg root rotation
					leg->m_chain->GetRootJoint()->m_localRotation = Quat::Nlerp(leg->m_animStayRootOrientation, leg->m_legRootDefaultLocalOrientation, adjustingFraction);

					// horizontal
					Vec3 footExpectedPos = m_inputPos + m_orientation.GetAsMatrix_XFwd_YLeft_ZUp().TransformVectorQuantity3D(leg->m_footDefaultOffset);
					Vec3 footOffsetHorizontal = Interpolate(leg->m_animCycleStartPos, footExpectedPos, adjustingFraction);

					// upward
					float cycleFraction = RangeMapClamped(adjustingFraction, 0.f, 1.f, 0.f, PI);
					Vec3 footOffsetUpward = Vec3(0.f, 0.f, sinf(cycleFraction) * 0.05f);

					// string debugStr = Stringf("disp: %.2f", footOffsetUpward.z);
					// DebugAddScreenText(debugStr, Vec2(), 20.f, Vec2(0.5f, 0.5f), -1.f, Rgba8::WHITE);

					newFootPos = footOffsetHorizontal + footOffsetUpward;

					chain->m_solver->SetSolverTarget(newFootPos);
					chain->m_solver->SetPoleVector(newFootPos + Vec3(0.f, 0.f, -1.f));
					AlignLegRootJointWithTarget(leg);
					chain->m_solver->SolveIK();

					// DebugAddWorldWireSphere(leg->m_animCycleStartPos, 0.05f, m_walkingTimerPeroid_max * 0.3f, Rgba8::RED, Rgba8::RED, DebugRenderMode::X_RAY);
					// DebugAddWorldWireSphere(footExpectedPos, 0.05f, m_walkingTimerPeroid_max * 0.3f, Rgba8::GREEN, Rgba8::GREEN, DebugRenderMode::X_RAY);
				}
				else
				{
					// // if the timer is finished, reset all of the leg to default
					if (timer->HasPeroidElapsed())
					{
						leg->m_chain->GetRootJoint()->m_localRotation = leg->m_legRootDefaultLocalOrientation;
						Vec3 footExpectedPos = m_inputPos + m_orientation.GetAsMatrix_XFwd_YLeft_ZUp().TransformVectorQuantity3D(leg->m_footDefaultOffset);
						chain->m_solver->SetSolverTarget(footExpectedPos);
						chain->m_solver->SetPoleVector(footExpectedPos + Vec3(0.f, 0.f, -1.f));
						AlignLegRootJointWithTarget(leg);
						chain->m_solver->SolveIK();

						leg->m_legRootRecordedWorldOrientation = chain->GetRootJoint()->GetWorldQuat();
						leg->m_legRecordedForwordOrientation = m_orientation;
					}
				}
			}

			double timeAtEnd = GetCurrentTimeSeconds();
			double timeElapsed = timeAtEnd - timeAtStart;

			m_adjustMaxDuration = max(m_adjustMaxDuration, timeElapsed);
		}

		else if (i == int(CrawlerAnimMode::WALKING))
		{
			double timeAtStart = GetCurrentTimeSeconds();

			// if in current frame we switch to walking animation state from other states
			// we'll set up its timer and stay pos
			if (GetAnimStateLastFrameWeight(CrawlerAnimMode::WALKING) == 0.f)
			{
				for (int legIndex = 0; legIndex < m_legs.size(); ++legIndex)
				{
					CrawlerLeg* leg = m_legs[legIndex];
					Timer*& timer = leg->m_walkingTimer;
					if (timer)
					{
						timer->Stop();
					}
					else
					{
						timer = new Timer(m_walkingTimerPeroid_max);
					}

					// Joint* j = leg->m_chain->GetEndJoint();
					// leg->m_animStayPos_walking = j->GetWorldPosition();
					// leg->m_animStayPos_walking.z = 0.f;
				}
			}

			if (GetAnimStateWeight(CrawlerAnimMode::WALKING) == 0.f)
			{
				continue;
			}

			// for each leg, if the leg should move and the moving legs nums is <= half
			// allow it to move the cycle
			// int num = GetNumLegsShouldMove();
			int numOfLegsAllowToMove = 2;

			for (int legIndex = 0; legIndex < m_legs.size(); ++legIndex)
			{
				CrawlerLeg* leg = m_legs[legIndex];
				Timer* timer = leg->m_walkingTimer;
				float walkingFraction = timer->GetElapsedFraction();
				if ((walkingFraction > 0.f && walkingFraction < 1.f))
				{
					--numOfLegsAllowToMove;
				}
			}

			for (int legIndex = 0; legIndex < m_legs.size(); ++legIndex)
			{
				CrawlerLeg* leg = m_legs[legIndex];
				Chain* chain = m_legs[legIndex]->m_chain;
				Timer* timer = m_legs[legIndex]->m_walkingTimer;
				// if this leg is allowed to move or this leg is already moving
				// only if the timer has not finished
				float walkingFraction = timer->GetElapsedFraction();
				// float rotatingFraction = leg->m_rotatingTimer->GetElapsedFraction();

				if ((numOfLegsAllowToMove > 0 && m_legsShouldStartToWalk && timer->IsStopped()) || (walkingFraction > 0.f && walkingFraction < 1.f))
				{

					leg->m_legCurrentAnimState = CrawlerAnimMode::WALKING;

					// if in current frame we switch to walking animation state from other states
					// we'll set up its timer and go

					// if the leg just started to move, record where it starts to move
					if (timer->IsStopped())
					{
						timer->Start();
						leg->m_animCycleStartPos_walking = leg->m_chain->GetEndJoint()->GetWorldPosition();
						leg->m_animCycleStartPos_walking.z = 0.f;
						--numOfLegsAllowToMove;
					}	

					Vec3 newFootPos;

					// if the crawler is running faster, the timer's period should be shortened
					float newPeriodDuration = RangeMapClamped(m_speed, 0.f, m_maxMovingSpeed, m_walkingTimerPeroid_max, m_walkingTimerPeroid_min);
					timer->m_period = newPeriodDuration;

					// based on timer's fraction, located new world pos
					// calculate about the step
					Vec3 stepLength = CalculateLegWalkStepByCrawlerVelocity(leg, true);
					// Vec3 stepLength = m_orientation.GetAsMatrix_XFwd_YLeft_ZUp().TransformVectorQuantity3D(Vec3(1.f, 0.f, 0.f));
					Vec3 footOffsetForward = stepLength * walkingFraction;

					float cycleFraction = RangeMapClamped(walkingFraction, 0.f, 1.f, 0.f, PI);
					Vec3 footOffsetUpward = Vec3(0.f, 0.f, sinf(cycleFraction) * 0.5f);

					newFootPos = leg->m_animCycleStartPos_walking + footOffsetForward + footOffsetUpward;

					// string debugStr = Stringf("forward length: %.2f", stepLength.GetLength());
					// DebugAddScreenText(debugStr, Vec2(), 20.f, Vec2(0.5f, 0.5f), -1.f, Rgba8::WHITE);


					if (isBlending)
					{
						Vec3 footExpectedRotationOffset = Interpolate(Vec3::ZERO, m_orientation.GetAsMatrix_XFwd_YLeft_ZUp().TransformVectorQuantity3D(leg->m_footDefaultOffset), walkingFraction);
						newFootPos += footExpectedRotationOffset * rotatingWeight * 2.f;

						// string debugStr = Stringf("rotating weight: %.2f", rotatingWeight);
						// DebugAddScreenText(debugStr, Vec2(), 20.f, Vec2(0.5f, 0.5f), -1.f, Rgba8::WHITE);
					}

					chain->m_solver->SetSolverTarget(newFootPos);
					chain->m_solver->SetPoleVector(newFootPos + Vec3(0.f, 0.f, -1.f));
					AlignLegRootJointWithTarget(leg);
					chain->m_solver->SolveIK();
					// chain->m_solver->SolveIKWithWeight(GetAnimStateWeight(CrawlerAnimMode::WALKING));

					// DebugAddWorldWireSphere(newFootPos, 0.05f,  0.05f, Rgba8::BLUE, Rgba8::BLUE, DebugRenderMode::X_RAY);
				}
				else
				{
					// if the leg's walking cycle end
					if (walkingFraction > 1.f)
					{
						Joint* j = leg->m_chain->GetEndJoint();
						leg->m_animStayPos_walking = j->GetWorldPosition();
						leg->m_animStayPos_walking.z = 0.f;
					}

					// if the leg is in walking cycle and not allowed to walk
					// stop its timer
					timer->Stop();

					// keep the upper leg part model upright
					leg->m_chain->GetRootJoint()->m_localRotation = leg->m_legRootDefaultLocalOrientation;

					Vec3 currentPos = chain->GetEndJoint()->GetWorldPosition();

					chain->m_solver->SetSolverTarget(leg->m_animStayPos_walking);
					chain->m_solver->SetPoleVector(leg->m_animStayPos_walking + Vec3(0.f, 0.f, -1.f));
					AlignLegRootJointWithTarget(leg);
					chain->m_solver->SolveIK();


					// if (!isBlending)
					// {
					// 	chain->m_solver->SetSolverTarget(leg->m_animStayPos_walking);
					// 	chain->m_solver->SetPoleVector(leg->m_animStayPos_walking + Vec3(0.f, 0.f, -1.f));
					// 	chain->m_solver->SolveIK();
					// }
					// else
					// {
					// 	// chain->m_solver->SetSolverTarget(footExpectedPos);
					// 	// chain->m_solver->SetPoleVector(footExpectedPos + Vec3(0.f, 0.f, -1.f));
					// 	// chain->m_solver->SolveIKWithWeight(GetAnimStateWeight(CrawlerAnimMode::ROTATING));
					// }

					// DebugAddWorldWireSphere(leg->m_animStayPos_walking, 0.05f, m_walkingTimerPeroid_max * 0.3f, Rgba8::RED, Rgba8::RED, DebugRenderMode::X_RAY);
					// 
					// currentPos = chain->GetEndJoint()->GetWorldPosition();
					// DebugAddWorldWireSphere(currentPos, 0.05f, m_walkingTimerPeroid_max * 0.3f, Rgba8::SALMON_PINK, Rgba8::SALMON_PINK, DebugRenderMode::X_RAY);
				}
			}

			double timeAtEnd = GetCurrentTimeSeconds();
			double timeElapsed = timeAtEnd - timeAtStart;

			m_walkMaxDuration = max(m_walkMaxDuration, timeElapsed);
		}
	}

	// if we are blending two animation and the leg both timer are stopped, we enforce the leg to stay at position
	// if (isBlending)
	// {
	// 	for (int legIndex = 0; legIndex < m_legs.size(); ++legIndex)
	// 	{
	// 		CrawlerLeg* leg = m_legs[legIndex];
	// 		Chain* chain = leg->m_chain;
	// 
	// 		if (leg->m_walkingTimer->IsStopped() && leg->m_rotatingTimer->IsStopped())
	// 		{
	// 			chain->m_solver->SetSolverTarget(leg->m_animStayPos_walking);
	// 			chain->m_solver->SetPoleVector(leg->m_animStayPos_walking + Vec3(0.f, 0.f, -1.f));
	// 			chain->m_solver->SolveIK();
	// 		}
	// 	}
	// }

	DebuggerPrintf("*******************************************************\n");
	std::string UMsg = Stringf(" idle duration: %.8f\n", m_idleMaxDuration);
	DebuggerPrintf(UMsg.c_str()); 
	
	UMsg = Stringf(" adjust duration: %.8f\n", m_adjustMaxDuration);
	DebuggerPrintf(UMsg.c_str());

	UMsg = Stringf(" walking duration: %.8f\n", m_walkMaxDuration);
	DebuggerPrintf(UMsg.c_str());

	UMsg = Stringf(" rotate duration: %.8f\n", m_rotationMaxDuration);
	DebuggerPrintf(UMsg.c_str()); 
	
	UMsg = Stringf(" update duration: %.8f\n", m_totalUpdateDuration);
	DebuggerPrintf(UMsg.c_str()); 

}

void Crawler::UpdateAnimationStateMachine()
{
	switch (m_crawlerCurrentFrameState)
	{
	case CrawlerAnimMode::IDLE:
	{
		if (m_crawlerCurrentFrameState != m_crawlerLastFrameState)
		{
			m_idleTimer = new Timer(m_idlePeroid);
			m_idleTimer->Start();
		}

		// set the pelvis up and down by sin for acting breathing anim
		if (m_idleTimer->HasPeroidElapsed())
		{
			m_idleTimer->Start();
		}
		float heightOffsetMultipler = m_idleHeightOffset * sinf(m_idleTimer->GetElapsedTime() * 2.f) + 1.f;

		Joint* pelvis = m_skeleton->GetJointByName("pelvis");

		pelvis->SetLocalBonePos(Vec3(m_pelvisHeight * heightOffsetMultipler, 0.f, 0.f));

		// keep the foot default offset
		for (int legIndex = 0; legIndex < m_legs.size(); ++legIndex)
		{
			CrawlerLeg* leg = m_legs[legIndex];
			Chain* chain = m_legs[legIndex]->m_chain;
			// Joint* legRootJoint = chain->GetRootJoint();

			// the leg root need to change local rotation, in order to maintain the m_recordedBodyOrientation
			leg->m_legCurrentAnimState = CrawlerAnimMode::IDLE;

			leg->m_legRootRecordedWorldOrientation = chain->GetRootJoint()->GetWorldQuat();
			leg->m_legRecordedForwordOrientation = m_orientation;
		}
	} break;

	case CrawlerAnimMode::ROTATING:
	{
		EulerAngles desiredOrientation = g_theGame->m_player->m_orientation;
		desiredOrientation.m_pitchDegrees = 0.f;
		desiredOrientation.m_rollDegrees = 0.f;

		// float deltaSeconds = g_theGameClock->GetDeltaSeconds();

		// if in current frame we switch to rotating animation state from other states
		// we'll set up its timer and stay pos
		if (m_crawlerCurrentFrameState != m_crawlerLastFrameState)
		{
			for (int legIndex = 0; legIndex < m_legs.size(); ++legIndex)
			{
				CrawlerLeg* leg = m_legs[legIndex];

				if (leg->m_chain->m_timer)
				{
					delete leg->m_chain->m_timer;
				}
				leg->m_chain->m_timer = new Timer(m_rotatingPeroid);

				Joint* j = leg->m_chain->GetEndJoint();
				leg->m_animStayPos = j->GetWorldPosition();
			}
		}

		// for each leg, if the leg should move and the moving legs nums is <= half
		// allow it to move the cycle
		// int num = GetNumLegsShouldMove();
		int numOfLegsAllowToMove = 2;
		for (int legIndex = 0; legIndex < m_legs.size(); ++legIndex)
		{
			CrawlerLeg* leg = m_legs[legIndex];
			Chain* chain = m_legs[legIndex]->m_chain;
			Timer* timer = m_legs[legIndex]->m_chain->m_timer;
			Joint* legRootJoint = chain->GetRootJoint();
			// if this leg is allowed to move or this leg is already moving
			// only if the timer has not finished
			float rotatingFraction = leg->m_chain->m_timer->GetElapsedFraction();
			if (((numOfLegsAllowToMove > 0 && timer->IsStopped() && leg->m_shouldRotate) || (rotatingFraction > 0.f && rotatingFraction < 1.f)) && m_bodyNeedsRotating)
			{
				--numOfLegsAllowToMove;

				// if the leg just started to move, record where it starts to move
				if (timer->IsStopped())
				{
					chain->m_timer->Start();
					leg->m_animCycleStartPos = leg->m_chain->GetEndJoint()->GetWorldPosition();
					leg->m_animStayRootOrientation = leg->m_chain->GetRootJoint()->m_localRotation;
					leg->m_animStayPos.z = 0.f;
				}

				Vec3 newFootPos;

				// if more than two legs are rotating, lower the timer's period
				if (numOfLegsAllowToMove < 0)
				{
					// float newPeriodDuration = RangeMapClamped(m_speed, 0.f, m_maxMovingSpeed, m_walkingTimerPeroid_max, m_walkingTimerPeroid_min);
					if (numOfLegsAllowToMove == -1)
					{
						timer->m_period = m_rotatingPeroid * 0.5f;
					}
					else
					{
						timer->m_period = m_rotatingPeroid * 0.25f;
					}
					rotatingFraction = leg->m_chain->m_timer->GetElapsedFraction();
				}
				else
				{
					timer->m_period = m_rotatingPeroid * 0.5f;
				}

				// based on timer's fraction, lerp the foot position and leg root rotation
				leg->m_chain->GetRootJoint()->m_localRotation = Quat::Nlerp(leg->m_animStayRootOrientation, leg->m_legRootDefaultLocalOrientation, rotatingFraction);

				// horizontal
				Vec3 footExpectedPos = m_position + m_orientation.GetAsMatrix_XFwd_YLeft_ZUp().TransformVectorQuantity3D(leg->m_footDefaultOffset);
				Vec3 footOffsetHorizontal = Interpolate(leg->m_animCycleStartPos, footExpectedPos, rotatingFraction);

				// upward
				float cycleFraction = RangeMapClamped(rotatingFraction, 0.f, 1.f, 0.f, PI);
				Vec3 footOffsetUpward = Vec3(0.f, 0.f, sinf(cycleFraction) * 0.05f);

				// string debugStr = Stringf("disp: %.2f", footOffsetUpward.z);
				// DebugAddScreenText(debugStr, Vec2(), 20.f, Vec2(0.5f, 0.5f), -1.f, Rgba8::WHITE);

				newFootPos = footOffsetHorizontal + footOffsetUpward;

				// keep the upper leg part model upright before solving
				leg->m_chain->GetRootJoint()->m_localRotation = leg->m_legRootDefaultLocalOrientation;

				// solve new world pos
				chain->m_solver->SetSolverTarget(newFootPos);
				chain->m_solver->SetPoleVector(newFootPos + Vec3(0.f, 0.f, -1.f));
				chain->m_solver->SolveIK();

				// DebugAddWorldWireSphere(footExpectedPos, 0.05f, m_walkingTimerPeroid_max * 0.3f, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::X_RAY);
			}
			else
			{
				// if the timer has finished, update the leg forward orientation
				if (timer->HasPeroidElapsed())
				{
					leg->m_legRecordedForwordOrientation = m_orientation;
					leg->m_legRootRecordedWorldOrientation = legRootJoint->GetWorldQuat();
				}

				// if the leg is not permitted to move yet
				// the leg root need to change local rotation, in order to maintain the m_recordedBodyOrientation
				leg->m_chain->m_timer->Stop();
				leg->m_legCurrentAnimState = CrawlerAnimMode::IDLE;

				// keep the root to remain its world rotation
				legRootJoint->m_localRotation = legRootJoint->m_parent->GetWorldQuat().GetInversed() * leg->m_legRootRecordedWorldOrientation;

				Vec3 footExpectedPos = m_position + leg->m_legRecordedForwordOrientation.GetAsMatrix_XFwd_YLeft_ZUp().TransformVectorQuantity3D(leg->m_footDefaultOffset);


				chain->m_solver->SetSolverTarget(footExpectedPos);
				chain->m_solver->SetPoleVector(footExpectedPos + Vec3(0.f, 0.f, -1.f));
				chain->m_solver->SolveIK();
			}
		}
	}break;

	case CrawlerAnimMode::ADJUSTING:
	{
		// if in current frame we switch to walking animation state from other states
		// we'll set up its timer and stay pos
		if (m_crawlerCurrentFrameState != m_crawlerLastFrameState)
		{
			for (int legIndex = 0; legIndex < m_legs.size(); ++legIndex)
			{
				CrawlerLeg* leg = m_legs[legIndex];

				if (leg->m_chain->m_timer)
				{
					delete leg->m_chain->m_timer;
				}
				leg->m_chain->m_timer = new Timer(m_adjustingTimerPeroid);

				Joint* j = leg->m_chain->GetEndJoint();
				leg->m_animStayPos = j->GetWorldPosition();
			}
		}

		// for each leg, if the leg should move and the moving legs nums is <= half
		// allow it to move the cycle
		// int num = GetNumLegsShouldMove();
		int numOfLegsAllowToMove = 1;
		for (int legIndex = 0; legIndex < m_legs.size(); ++legIndex)
		{
			CrawlerLeg* leg = m_legs[legIndex];
			Chain* chain = m_legs[legIndex]->m_chain;
			Timer* timer = m_legs[legIndex]->m_chain->m_timer;
			// if this leg is allowed to move or this leg is already moving
			// only if the timer has not finished
			float adjustingFraction = leg->m_chain->m_timer->GetElapsedFraction();
			if ((numOfLegsAllowToMove > 0 && timer->IsStopped() && leg->m_shouldAdjust) || (adjustingFraction > 0.f && adjustingFraction < 1.f))
			{
				--numOfLegsAllowToMove;

				// if the leg just started to move, record where it starts to move
				if (timer->IsStopped())
				{
					chain->m_timer->Start();
					leg->m_animCycleStartPos = leg->m_chain->GetEndJoint()->GetWorldPosition();
					leg->m_animStayRootOrientation = leg->m_chain->GetRootJoint()->m_localRotation;
					leg->m_animStayPos.z = 0.f;
				}

				Vec3 newFootPos;

				// if the crawler is running faster, the timer's period should be shortened
				float newPeriodDuration = RangeMapClamped(m_speed, 0.f, m_maxMovingSpeed, m_walkingTimerPeroid_max, m_walkingTimerPeroid_min);
				timer->m_period = newPeriodDuration;

				// based on timer's fraction, lerp the foot position and leg root rotation
				leg->m_chain->GetRootJoint()->m_localRotation = Quat::Nlerp(leg->m_animStayRootOrientation, leg->m_legRootDefaultLocalOrientation, adjustingFraction);

				// horizontal
				Vec3 footExpectedPos = m_position + m_orientation.GetAsMatrix_XFwd_YLeft_ZUp().TransformVectorQuantity3D(leg->m_footDefaultOffset);
				Vec3 footOffsetHorizontal = Interpolate(leg->m_animCycleStartPos, footExpectedPos, adjustingFraction);

				// upward
				float cycleFraction = RangeMapClamped(adjustingFraction, 0.f, 1.f, 0.f, PI);
				Vec3 footOffsetUpward = Vec3(0.f, 0.f, sinf(cycleFraction) * 0.05f);

				// string debugStr = Stringf("disp: %.2f", footOffsetUpward.z);
				// DebugAddScreenText(debugStr, Vec2(), 20.f, Vec2(0.5f, 0.5f), -1.f, Rgba8::WHITE);

				newFootPos = footOffsetHorizontal + footOffsetUpward;

				// solve new world pos
				chain->m_solver->SetSolverTarget(newFootPos);
				chain->m_solver->SetPoleVector(newFootPos + Vec3(0.f, 0.f, -1.f));
				chain->m_solver->SolveIK();

				leg->m_legCurrentAnimState = CrawlerAnimMode::ADJUSTING;
				// DebugAddWorldWireSphere(footExpectedPos, 0.05f, m_walkingTimerPeroid_max * 0.3f, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::X_RAY);
			}
			else
			{
				// // if the timer is finished, reset all of the leg to default
				if (timer->HasPeroidElapsed())
				{
					leg->m_chain->GetRootJoint()->m_localRotation = leg->m_legRootDefaultLocalOrientation;
					Vec3 footExpectedPos = m_position + m_orientation.GetAsMatrix_XFwd_YLeft_ZUp().TransformVectorQuantity3D(leg->m_footDefaultOffset);
					chain->m_solver->SetSolverTarget(footExpectedPos);
					chain->m_solver->SetPoleVector(footExpectedPos + Vec3(0.f, 0.f, -1.f));
					chain->m_solver->SolveIK();

					leg->m_legRootRecordedWorldOrientation = chain->GetRootJoint()->GetWorldQuat();
					leg->m_legRecordedForwordOrientation = m_orientation;
				}

				// if the leg is not permitted to move yet
				leg->m_chain->m_timer->Stop();
				leg->m_legCurrentAnimState = CrawlerAnimMode::IDLE;
			}
		}
	}break;

	case CrawlerAnimMode::WALKING:
	{
		// if in current frame we switch to walking animation state from other states
		// we'll set up its timer and stay pos
		if (m_crawlerCurrentFrameState != m_crawlerLastFrameState)
		{
			for (int legIndex = 0; legIndex < m_legs.size(); ++legIndex)
			{
				CrawlerLeg* leg = m_legs[legIndex];

				if (leg->m_chain->m_timer)
				{
					delete leg->m_chain->m_timer;
				}
				leg->m_chain->m_timer = new Timer(m_walkingTimerPeroid_max);

				Joint* j = leg->m_chain->GetEndJoint();
				leg->m_animStayPos = j->GetWorldPosition();
			}
		}

		// for each leg, if the leg should move and the moving legs nums is <= half
		// allow it to move the cycle
		// int num = GetNumLegsShouldMove();
		int numOfLegsAllowToMove = 2;
		for (int legIndex = 0; legIndex < m_legs.size(); ++legIndex)
		{
			CrawlerLeg* leg = m_legs[legIndex];
			Chain* chain = m_legs[legIndex]->m_chain;
			Timer* timer = m_legs[legIndex]->m_chain->m_timer;
			// if this leg is allowed to move or this leg is already moving
			// only if the timer has not finished
			float walkingFraction = leg->m_chain->m_timer->GetElapsedFraction();
			if ((numOfLegsAllowToMove > 0 && leg->m_shouldMove && timer->IsStopped()) || (walkingFraction > 0.f && walkingFraction < 1.f))
			{
				--numOfLegsAllowToMove;

				leg->m_legCurrentAnimState = CrawlerAnimMode::WALKING;

				// if in current frame we switch to walking animation state from other states
				// we'll set up its timer and go

				// if the leg just started to move, record where it starts to move
				if (timer->IsStopped())
				{
					chain->m_timer->Start();
					leg->m_animCycleStartPos = leg->m_chain->GetEndJoint()->GetWorldPosition();
					leg->m_animStayPos.z = 0.f;
				}

				Vec3 newFootPos;

				// if the crawler is running faster, the timer's period should be shortened
				float newPeriodDuration = RangeMapClamped(m_speed, 0.f, m_maxMovingSpeed, m_walkingTimerPeroid_max, m_walkingTimerPeroid_min);
				timer->m_period = newPeriodDuration;

				// based on timer's fraction, located new world pos
				// calculate about the step
				Vec3 stepLength = CalculateLegWalkStepByCrawlerVelocity(leg, true);
				Vec3 footOffsetForward = stepLength * walkingFraction;

				float cycleFraction = RangeMapClamped(walkingFraction, 0.f, 1.f, 0.f, PI);
				Vec3 footOffsetUpward = Vec3(0.f, 0.f, sinf(cycleFraction) * 0.5f);

				newFootPos = leg->m_animCycleStartPos + footOffsetForward + footOffsetUpward;

				// solve new world pos
				chain->m_solver->SetSolverTarget(newFootPos);
				chain->m_solver->SetPoleVector(newFootPos + Vec3(0.f, 0.f, -1.f));
				chain->m_solver->SolveIK();

				// DebugAddWorldWireSphere(newFootPos, 0.05f, m_walkingTimerPeroid_max * 0.3f, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::X_RAY);
			}
			else
			{
				// if the leg is in walking cycle and not allowed to walk
				// stop its timer
				leg->m_chain->m_timer->Stop();
				leg->m_legCurrentAnimState = CrawlerAnimMode::IDLE;

				// if the leg's animation state changes, update its stay position
				if (leg->m_legLastFrameAnimState != leg->m_legCurrentAnimState)
				{
					leg->m_animStayPos = chain->GetEndJoint()->GetWorldPosition();
					leg->m_animStayPos.z = 0.f;
				}

				// keep the upper leg part model upright
				leg->m_chain->GetRootJoint()->m_localRotation = leg->m_legRootDefaultLocalOrientation;

				// solve by the stay pos
				chain->m_solver->SetSolverTarget(leg->m_animStayPos);
				chain->m_solver->SetPoleVector(leg->m_animStayPos + Vec3(0.f, 0.f, -1.f));
				chain->m_solver->SolveIK();
			}
		}
	}break;
	}
}

Vec3 Crawler::CalculateLegWalkStepByCrawlerVelocity(CrawlerLeg* leg, bool IsPredicting /*= false*/)
{
	// get the time left for this leg to move
	Timer* timer = leg->m_walkingTimer;
	float timerPeroid = timer->m_period;

	// get the predicted crawler position
	Vec3 expectedCrawlerPos = timerPeroid * m_velocity + m_position;

	Vec3 additionWalkingDisp = Vec3::ZERO;
	if (IsPredicting)
	{ 
		// if the crawler moving faster, we want the foot to move further to the default offset
		float predictDist = RangeMapClamped(m_speed, 0.f, m_maxMovingSpeed, m_maxPredictionDist * 0.2f, m_maxPredictionDist);
		// float predictDist = 0.3f;
		// float predictDist = m_maxPredictionDist;
		additionWalkingDisp = m_velocityDir * predictDist;

		// string debugStr = Stringf("dist: %.2f", predictDist); //disp.GetLength());
		// // string debugStr = Stringf("disp: %.2f, %.2f, %.2f", disp.x, disp.y, disp.z);
		// DebugAddScreenText(debugStr, Vec2(), 20.f, Vec2(0.5f, 0.5f), -1.f, Rgba8::WHITE);
	}

	Vec3 expectedFootPos = expectedCrawlerPos + m_orientation.GetAsMatrix_XFwd_YLeft_ZUp().TransformVectorQuantity3D(leg->m_footDefaultOffset) + additionWalkingDisp;
	Vec3 lastFootStayPos = leg->m_animCycleStartPos_walking;

	Vec3 disp = expectedFootPos - lastFootStayPos;

	return disp;
}

void Crawler::UpdateToeGroundedStatus()
{
	// check if each leg's toe is on the ground to support the body
	for (int legIndex = 0; legIndex < m_legs.size(); ++legIndex)
	{
		CrawlerLeg* leg = m_legs[legIndex];
		Chain* chain = leg->m_chain;
		if (chain->GetEndJoint()->GetWorldPosition().z <= 0.01f)
		{
			leg->m_isGrounded = true;
		}
		else
		{
			leg->m_isGrounded = false;
		}
	}
}

bool Crawler::AreAllLegsGrounded()
{
	for (int legIndex = 0; legIndex < m_legs.size(); ++legIndex)
	{
		CrawlerLeg* leg = m_legs[legIndex];
		if (!leg->m_isGrounded)
		{
			return false;
		}
	}
	return true;
}

void Crawler::UpdateIfLegShouldMove()
{
	// calculate the distance from the end effector to the root joint 
	// see if it is exceeding the total bone length
	for (int legIndex = 0; legIndex < m_legs.size(); ++legIndex)
	{
		CrawlerLeg* leg = m_legs[legIndex];
		Chain* chain = leg->m_chain;
		float crtDistRootToEnd = chain->GetDistanceFromRootToEnd();

		Vec3 footExpectedPos = m_position + m_orientation.GetAsMatrix_XFwd_YLeft_ZUp().TransformVectorQuantity3D(leg->m_footDefaultOffset);
		Vec3 footCurrentPos = chain->GetEndJoint()->GetWorldPosition();

		// if current is advance of the expected pos
		// no need to move
		Vec3 disp = footCurrentPos - footExpectedPos;
		float dotProduct_dir = DotProduct3D(m_velocityDir, disp);
		if (dotProduct_dir > 0.f)
		{
			leg->m_shouldMove = false;
			continue;
		}

		// float dist = GetDistanceSquared3D(footExpectedPos, footCurrentPos);

		// string debugStr = Stringf("dist: %.2f", dist); //disp.GetLength());
		// string debugStr = Stringf("disp: %.2f, %.2f, %.2f", disp.x, disp.y, disp.z);
		// DebugAddScreenText(debugStr, Vec2(), 20.f, Vec2(0.5f, 0.5f), -1.f, Rgba8::WHITE);

		// if the chain is stretched and need to move
		// or the foot is not backed to the original place
		if (crtDistRootToEnd >= (leg->m_chainLength * 0.85f) || crtDistRootToEnd <= (leg->m_chainLength * 0.6f) ) // || disp.GetLength() > 0.01f)
		{
			leg->m_shouldMove = true;
			m_legsShouldStartToWalk = true;
		}
		else
		{
			leg->m_shouldMove = false;
		}
	}
}

bool Crawler::CheckIfCrawlerNeedsToAdjustPose()
{
	// check if four legs are back to default place
	// if not, then no need to act the breathing of pelvis going up and down
	bool result = false;
	for (int legIndex = 0; legIndex < m_legs.size(); ++legIndex)
	{
		// CrawlerLeg* leg = m_legs[legIndex];
		// Chain* chain = m_legs[legIndex]->m_chain;
		// 
		// Vec3 footExpectedPos = m_position + m_orientation.GetAsMatrix_XFwd_YLeft_ZUp().TransformVectorQuantity3D(leg->m_footDefaultOffset);
		// Vec3 footCurrentPos = chain->GetEndJoint()->GetWorldPosition();
		// footExpectedPos.z = 0.f;
		// footCurrentPos.z = 0.f;
		// 
		// float dist = GetDistanceSquared3D(footExpectedPos, footCurrentPos);
		// Quat currentRootRotation = leg->m_chain->GetRootJoint()->m_localRotation;
		// if ((dist >= 0.01f || currentRootRotation.IsMostEqual(leg->m_legRootDefaultLocalOrientation, 0.1f)) && !m_bodyNeedsRotating)
		// {
		// 	leg->m_shouldAdjust = true;
		// 	result = true;
		// }
		// else
		// {
		// 	leg->m_shouldAdjust = false;
		// }

		CrawlerLeg* leg = m_legs[legIndex];
		Chain* chain = m_legs[legIndex]->m_chain;

		Vec3 footExpectedPos = m_inputPos + m_orientation.GetAsMatrix_XFwd_YLeft_ZUp().TransformVectorQuantity3D(leg->m_footDefaultOffset);
		Vec3 footCurrentPos = chain->GetEndJoint()->GetWorldPosition();
		footExpectedPos.z = 0.f;
		footCurrentPos.z = 0.f;

		float dist = GetDistance3D(footExpectedPos, footCurrentPos);
		// Quat currentRootRotation = leg->m_chain->GetRootJoint()->m_localRotation;
		// bool orientationStored = currentRootRotation.IsMostEqual(leg->m_legRootDefaultLocalOrientation, 0.05f);
		if (dist >= 0.01f) //0.03125f
		{
			leg->m_shouldAdjust = true;
			result = true;
		}
		else
		{
			leg->m_shouldAdjust = false;
		}
	}

	return result;
}

bool Crawler::CheckIfLegShouldRotate()
{
	bool shouldRotate = false;
	for (int legIndex = 0; legIndex < m_legs.size(); ++legIndex)
	{
		CrawlerLeg* leg = m_legs[legIndex];

		float rotationDif = abs(m_orientation.m_yawDegrees - leg->m_legRecordedForwordOrientation.m_yawDegrees);

		if (rotationDif >= m_legRotationAngleBoundary)
		{
			leg->m_shouldRotate = true;
			shouldRotate = true;
		}
		else
		{
			leg->m_shouldRotate = false;
		}
	}

	return shouldRotate;
}

int Crawler::GetNumLegsShouldMove()
{
	int num = 0;
	for (int legIndex = 0; legIndex < m_legs.size(); ++legIndex)
	{
		CrawlerLeg* leg = m_legs[legIndex];
		if (leg->m_shouldMove)
		{
			++num;
		}
	}

	return num;
}

Mat44 Crawler::GetModelMatrix_EndEffectorTarget() const
{
	Mat44 transformMat;
	transformMat.SetTranslation3D(m_movingTarget_endEffector);
	return transformMat;
}

Mat44 Crawler::GetModelMatrix_PoleVector() const
{
	Mat44 transformMat;
	transformMat.SetTranslation3D(m_movingTarget_poleVector);
	return transformMat;
}

void Crawler::Update()
{
	double timeAtStart = GetCurrentTimeSeconds();

	UpdateInput();
	UpdateAnimationState();
	
	UpdateToeGroundedStatus();
	UpdateIfLegShouldMove();
	// UpdateAnimationStateMachine();
	UpdateAnimationBlending();
	UpdateDebugRenderMessages();

	m_crawlerLastFrameState = m_crawlerCurrentFrameState;
	for (int legIndex = 0; legIndex < m_legs.size(); ++legIndex)
	{
		CrawlerLeg* leg = m_legs[legIndex];
		leg->m_legLastFrameAnimState = leg->m_legCurrentAnimState;
	}


	for (int i = 0; i < (int)CrawlerAnimMode::NUM_MODE; ++i)
	{
		m_animWeightsLastFrame[i] = m_animWeights[i];
	}

	double timeAtEnd = GetCurrentTimeSeconds();
	double timeElapsed = timeAtEnd - timeAtStart;

	m_totalUpdateDuration = max(m_totalUpdateDuration, timeElapsed);
}

void Crawler::UpdateInput()
{
	m_velocity = Vec3::ZERO;
	Mat44 rotationMat = m_orientation.GetAsMatrix_XFwd_YLeft_ZUp();

	float deltaSeconds = g_theGameClock->GetDeltaSeconds();


	if (g_theInput->IsKeyDown('W'))
	{
		m_velocity += rotationMat.TransformVectorQuantity3D(Vec3(m_speed * deltaSeconds, 0.f, 0.f));
	}
	if (g_theInput->IsKeyDown('S'))
	{
		m_velocity += rotationMat.TransformVectorQuantity3D(Vec3(-(m_speed * deltaSeconds), 0.f, 0.f));
	}
	if (g_theInput->IsKeyDown('A'))
	{
		m_velocity += rotationMat.TransformVectorQuantity3D(Vec3(0.f, m_speed * deltaSeconds, 0.f));
	}
	if (g_theInput->IsKeyDown('D'))
	{
		m_velocity += rotationMat.TransformVectorQuantity3D(Vec3(0.f, -(m_speed * deltaSeconds), 0.f));
	}

	m_inputPos += m_velocity;
	// Vec3 newY = m_inputPos;
	// newY.x = 0.f;
	// m_secondOrderModifierOnY->Update(deltaSeconds, newY);	
	// Vec3 newX = m_inputPos;
	// newX.y = 0.f;
	// m_secondOrderModifierOnX->Update(deltaSeconds, newX);

	m_secondOrderModifier->Update(deltaSeconds, m_inputPos);

	// m_position = Vec3( m_secondOrderModifierOnX->m_goal_position.x, m_secondOrderModifierOnY->m_goal_position.y, 0.f);
	m_position = m_secondOrderModifier->m_goal_position;

	// m_velocityDir = Vec3( m_secondOrderModifierOnX->m_goal_velocity.x, m_secondOrderModifierOnY->m_goal_velocity.y,  0.f).GetNormalized();
	m_velocityDir = m_secondOrderModifier->m_goal_velocity.GetNormalized();

	if (m_velocity == Vec3::ZERO)
	{
		m_isDriving = false;
	}
	else
	{
		m_isDriving = true;
	}

	// string debugStr = Stringf("disp: %.2f, %.2f, %.2f", forceDir.x, forceDir.y, forceDir.z);
	// DebugAddScreenText(debugStr, Vec2(), 20.f, Vec2(0.5f, 0.5f), -1.f, Rgba8::WHITE);

	// std::string debugTargetPosition = Stringf("%.2f", m_speed);
	// DebugAddScreenText(debugTargetPosition, Vec2(), 20.f, Vec2(0.5f, 0.5f), -1.f); 

	// always rotate head to aim at the target
	Joint* neckJoint = m_skeleton->GetJointByName("neck");
	EulerAngles desiredOrientation = g_theGame->m_player->m_orientation;
	desiredOrientation.m_pitchDegrees = 0.f;
	desiredOrientation.m_rollDegrees = 0.f;

	m_neckJointCurrentOrientation.m_yawDegrees = GetTurnedTowardDegrees(m_neckJointCurrentOrientation.m_yawDegrees, desiredOrientation.m_yawDegrees, deltaSeconds * m_neckRotationSpeed);

	Quat desiredPointingQuat = neckJoint->m_parent->GetWorldQuat().GetInversed() * 
									ConvertEulerAnglesToQuat(m_neckJointCurrentOrientation);

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// neck tilting
	Quat currentDir = neckJoint->GetWorldQuat();
	Vec3 currentFwd = (currentDir * Vec3(0.f, 0.f, -1.f)).GetNormalized();

	Quat rotationDelta = Quat::GetQuatFromTwoVectors(currentFwd, m_velocityDir);
	Quat newWorldRotation = rotationDelta * currentDir;

	// compute what local rotation in the neck's local space
	Quat parentWorldRotation = neckJoint->m_parent->GetWorldQuat();
	Quat newLocalRotation = (parentWorldRotation.GetInversed() * newWorldRotation).GetNormalized();

	// use modifier's goal velocity and input velocity to check the influence of the inertia
	float fraction = DotProduct3D(m_secondOrderModifier->m_goal_velocity.GetNormalized(), m_velocity.GetNormalized());
	float cos30 = cosf(ConvertDegreesToRadians(30.f));
	fraction = RangeMapClamped(fraction, cos30, 1.f, 0.4f, 0.2f);
	fraction *= m_secondOrderModifier->m_goal_velocity.GetLength();

	// blend the tilting tendency and rotate to player target
	neckJoint->m_localRotation = Quat::Nlerp(neckJoint->m_localRotation, newLocalRotation, fraction);
	neckJoint->m_localRotation = Quat::Nlerp(neckJoint->m_localRotation, desiredPointingQuat, 0.9f);

	// string debugStr = Stringf("fraction: %.2f", fraction);
	// DebugAddScreenText(debugStr, Vec2(), 20.f, Vec2(0.5f, 0.5f), -1.f, Rgba8::WHITE);

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	float averageYawDegrees = 0.f;
	for (int legIndex = 0; legIndex < (int)m_legs.size(); ++legIndex)
	{
		CrawlerLeg* leg = m_legs[legIndex];
		// Chain* chain = m_legs[legIndex]->m_chain;
		// Timer* timer = m_legs[legIndex]->m_chain->m_timer;
		// Joint* legRootJoint = chain->GetRootJoint();
		averageYawDegrees += leg->m_legRecordedForwordOrientation.m_yawDegrees;
	}


	float angleDiff = abs(desiredOrientation.m_yawDegrees - averageYawDegrees * (1.f / (float)m_legs.size()));
	m_bodyNeedsRotating = angleDiff > m_legRotationAngleBoundary;

	if (m_orientation != desiredOrientation || m_bodyNeedsRotating)
	{
		m_crawlerCurrentFrameState = CrawlerAnimMode::ROTATING;
		SetAnimStateWeight(CrawlerAnimMode::ROTATING, 1.f);
		// float rate = 0.5f * g_theGameClock->GetDeltaSeconds();

		// if (GetAnimStateWeight(CrawlerAnimMode::ROTATING) < 1.f)
		// {
		// 	SetAnimStateWeight(CrawlerAnimMode::ROTATING, GetAnimStateWeight(CrawlerAnimMode::ROTATING) + rate);
		// }
	}
	else 
	{
		m_crawlerCurrentFrameState = CrawlerAnimMode::IDLE;
		SetAnimStateWeight(CrawlerAnimMode::ROTATING, 0.f);
		// if (GetAnimStateWeight(CrawlerAnimMode::ROTATING) < 1.f)
		// {
		// 	SetAnimStateWeight(CrawlerAnimMode::ROTATING, GetAnimStateWeight(CrawlerAnimMode::ROTATING) - 0.002f);
		// }
		// 
		// if (GetAnimStateWeight(CrawlerAnimMode::ROTATING) < 0.f)
		// {
		// 	SetAnimStateWeight(CrawlerAnimMode::ROTATING, 0.f);
		// }
	}
}

void Crawler::Render() const
{
	m_skeleton->Render();
}

void Crawler::SetAnimStateWeight(CrawlerAnimMode animState, float weight)
{
	m_animWeights[static_cast<int>(animState)] = weight;
}

float Crawler::GetAnimStateWeight(CrawlerAnimMode animState)
{
	return m_animWeights[static_cast<int>(animState)];
}

float Crawler::GetAnimStateLastFrameWeight(CrawlerAnimMode animState)
{
	return m_animWeightsLastFrame[static_cast<int>(animState)];
}

Mat44 Crawler::GetMovementTransformMatrix()
{
	// transform coordinate from local to world matrix
	// [translate][rotate]
	Mat44 transformMat;
	transformMat.SetTranslation3D(m_position);

	EulerAngles movingAngle = m_orientation;
	movingAngle.m_pitchDegrees = 0.f;
	movingAngle.m_rollDegrees = 0.f;
	Mat44 orientationMat = movingAngle.GetAsMatrix_XFwd_YLeft_ZUp();
	// orientationMat.Append(transformMat);
	// return orientationMat;
	transformMat.Append(orientationMat);
	return transformMat;
}

void Crawler::UpdateDebugRenderMessages()
{
	Vec2  textPos = Vec2(1.f, 0.f);
	Vec2  spacing = Vec2(0.f, 0.04f);

	Vec2  fpsAlignment = Vec2(0.99f, 0.98f);
	Vec2  slot2 = fpsAlignment - spacing;
	Vec2  slot3 = slot2 - spacing;
	Vec2  slot4 = slot3 - spacing;
	Vec2  slot5 = slot4 - spacing;
	Vec2  slot6 = slot5 - spacing;

	Vec2  slot7 = slot6 - spacing;
	Vec2  slot8 = slot7 - spacing;
	Vec2  slot9 = slot8 - spacing;
	Vec2  renderEmissiveAlignment = slot9 - spacing;

	Vec2  useDiffuseMapAlignment = renderEmissiveAlignment - spacing;
	Vec2  useNormalMapAlignment = useDiffuseMapAlignment - spacing;
	Vec2  useSpecularMapAlignment = useNormalMapAlignment - spacing;
	Vec2  useGlossinessMapAlignment = useSpecularMapAlignment - spacing;
	Vec2  useEmissiveMapAlignment = useGlossinessMapAlignment - spacing;

	float fontSize = 24.f;

	// if (m_FPSTimer->DecrementPeroidIfElapsed())
	// {
	// 	m_FPSString = Stringf("FPS: %s", std::to_string(Clock::GetSystemClock().GetFrameRatePerSecond()).c_str());
	// }
	// DebugAddScreenText(m_FPSString, Vec2(), fontSize, fpsAlignment, -1.f);
	// 
	// std::string takeCameraControl;
	// if (m_WinCameraIsInControl)
	// {
	// 	takeCameraControl = "Synchronize Camera(F): false";
	// }
	// else
	// {
	// 	takeCameraControl = "Synchronize Camera(F): true";
	// }
	// DebugAddScreenText(takeCameraControl, Vec2(), fontSize, slot2, -1.f);

	// string crawlerState;
	// switch (g_theGame->m_crawlers[0]->m_crawlerCurrentFrameState)
	// {
	// case CrawlerAnimMode::ADJUSTING:
	// {
	// 	crawlerState = Stringf("Anim State: %s", "Adjust");
	// }break;
	// case CrawlerAnimMode::WALKING:
	// {
	// 	crawlerState = Stringf("Anim State: %s", "Walk");
	// }break;
	// case CrawlerAnimMode::IDLE:
	// {
	// 	crawlerState = Stringf("Anim State: %s", "Idle");
	// }break;
	// case CrawlerAnimMode::ROTATING:
	// {
	// 	crawlerState = Stringf("Anim State: %s", "Rotate");
	// }break;
	// }
	// DebugAddScreenText(crawlerState, Vec2(), fontSize, slot2, -1.f, Rgba8::GREEN);

	// AABB2 bounds = g_theGame->m_screenCamera.GetCameraBounds();
	// float screenHeigt = bounds.m_maxs.y - bounds.m_mins.y;
	// float textLineHeight = screenHeigt / m_gameModeConfig.m_numMessageOnScreen;
	// float fontHeight = m_gameModeConfig.m_lineHeightAndTextBoxRatio * textLineHeight;
	// 
	// //----------------------------------------------------------------------------------------------------------------------------------------------------
	// std::vector<Vertex_PCU> messageVerts;
	// AABB2 currentLineBox(Vec2(bounds.m_mins.x, (bounds.m_maxs.y - textLineHeight)), Vec2(bounds.m_maxs.x, bounds.m_maxs.y));
	// m_gameModeConfig.m_font->AddVertsForTextInBox2D(messageVerts, m_modeName, currentLineBox, fontHeight, Vec2(0.f, 0.5f),
	// 	m_modeNameLineColor, 0.3f, m_gameModeConfig.m_cellAspect, TextDrawMode::OVERRUN);


	string crawlerState;
	for (int i = 0; i < (int)CrawlerAnimMode::NUM_MODE; ++i)
	{
		if (i == (int)CrawlerAnimMode::IDLE)
		{
			crawlerState = Stringf("IDLE: %.2f", GetAnimStateWeight(CrawlerAnimMode::IDLE));
			DebugAddScreenText(crawlerState, Vec2(), fontSize, slot2, -1.f, Rgba8::GREEN);
		}
		else if (i == (int)CrawlerAnimMode::ADJUSTING)
		{
			crawlerState = Stringf("ADJUST: %.2f", GetAnimStateWeight(CrawlerAnimMode::ADJUSTING));
			DebugAddScreenText(crawlerState, Vec2(), fontSize, slot3, -1.f, Rgba8::GREEN);
		}
		else if (i == (int)CrawlerAnimMode::WALKING)
		{
			crawlerState = Stringf("WALK: %.2f", GetAnimStateWeight(CrawlerAnimMode::WALKING));
			DebugAddScreenText(crawlerState, Vec2(), fontSize, slot4, -1.f, Rgba8::GREEN);
		}
		else if (i == (int)CrawlerAnimMode::ROTATING)
		{
			crawlerState = Stringf("ROTATE: %.2f", GetAnimStateWeight(CrawlerAnimMode::ROTATING));
			DebugAddScreenText(crawlerState, Vec2(), fontSize, slot5, -1.f, Rgba8::GREEN);
		}
	}

	// std::string lockTarget;
	// if (m_targetLocked)
	// {
	// 	lockTarget = "Target Locked(G): true";
	// }
	// else
	// {
	// 	lockTarget = "Target Locked(G): false";
	// }
	// DebugAddScreenText(lockTarget, Vec2(), fontSize, slot3, -1.f);

	// std::string debugBodyRotation;
	// if (m_bodyNeedsRotating)
	// {
	// 	debugBodyRotation = "Body needs rotation : True";
	// }
	// else 
	// {
	// 	debugBodyRotation = "Body needs rotation : false";
	// }
	// DebugAddScreenText(debugBodyRotation, Vec2(), fontSize, slot3, -1.f);

	// std::string targetInControl;
	// if (m_target1_inControl)
	// {
	// 	targetInControl = "Controlling solving target(G)";
	// 	DebugAddScreenText(targetInControl, Vec2(), fontSize, slot3, -1.f, m_movingTargetColor);
	// }
	// else
	// {
	// 	targetInControl = "Controlling pole vector(G)";
	// 	DebugAddScreenText(targetInControl, Vec2(), fontSize, slot3, -1.f, m_movingPolveVectorColor);
	// }

	// std::string debugTargetPosition = Stringf("Target Position (Arrows/Z/X): ( %.2f, %.2f, %.2f )", m_movingTarget_endEffector.x, m_movingTarget_endEffector.y, m_movingTarget_endEffector.z);
	// DebugAddScreenText(debugTargetPosition, Vec2(), fontSize, slot4, -1.f);

	// Joint* pelvis = m_skeleton->GetJointByName("pelvis");
	// std::string debugTargetPosition = Stringf("Offset %.2f", pelvis->GetBoneLocalPosition().x);
	// DebugAddScreenText(debugTargetPosition, Vec2(), fontSize, slot9, -1.f);

	// std::string debugJointPosition = Stringf("JointC Position : ( %.1f, %.1f, %.1f )", m_debugJoint2Pos.x, m_debugJoint2Pos.y, m_debugJoint2Pos.z);
	// DebugAddScreenText(debugJointPosition, Vec2(), fontSize, Vec2(0.98f, 0.02f), -1.f);	
	// 
	// std::string debugAngleDegrees = Stringf("Rotation Angle: %.1f", ConvertRadiansToDegrees(m_debugAngle));
	// DebugAddScreenText(debugAngleDegrees, Vec2(), fontSize, Vec2(0.98f, 0.02f) + spacing, -1.f);

	// std::string solvingStatus;
	// if (m_solveTarget)
	// {
	// 	solvingStatus = "Solving (C) : O";
	// 	DebugAddScreenText(solvingStatus, Vec2(), fontSize, slot5, -1.f, Rgba8::GREEN);
	// }
	// else
	// {
	// 	solvingStatus = "Solving (C) : X";
	// 	DebugAddScreenText(solvingStatus, Vec2(), fontSize, slot5, -1.f, Rgba8::RED);
	// }
	// 
	// std::string debugChain;
	// if (m_debugChain)
	// {
	// 	if (m_debugChain->m_solver->m_debugMode)
	// 	{
	// 		debugChain = "Debug chain (I): O";
	// 		DebugAddScreenText(debugChain, Vec2(), fontSize, slot6, -1.f, Rgba8::GREEN);
	// 	}
	// 	else
	// 	{
	// 		debugChain = "Debug chain (I): X";
	// 		DebugAddScreenText(debugChain, Vec2(), fontSize, slot6, -1.f, Rgba8::RED);
	// 	}
	// }
	// else
	// {
	// 	debugChain = "Debug chain (I): X";
	// 	DebugAddScreenText(debugChain, Vec2(), fontSize, slot6, -1.f, Rgba8::RED);
	// }
	// 
	// std::string debugSolverStep = "Press K to step solver forward";
	// DebugAddScreenText(debugSolverStep, Vec2(), fontSize, slot7, -1.f);
	// 
	// string rightJoystickValue = Stringf("right joystick:(%.2f, %.2f)", g_theVRPlayer->m_hands[Side::RIGHT].m_joyStickPos.x,  g_theVRPlayer->m_hands[Side::RIGHT].m_joyStickPos.y);
	// DebugAddScreenText(rightJoystickValue, Vec2(), fontSize, slot8, -1.f, Rgba8::GREEN);
	// 
	// DebugAddWorldArrow(m_debugJointPos, m_debugJointPos + m_debugAxis, 0.01f, 0.f, Rgba8::BRIGHT_ORANGE);
}

