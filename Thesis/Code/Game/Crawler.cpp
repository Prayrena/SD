#include "Engine/core/Clock.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/Window.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/DebugRender.hpp"
#include "Engine/Renderer/DebugRenderGeometry.hpp"
#include "Engine/Math/OpenXRMathUtils.hpp"
#include "Engine/Model/ObjModel.hpp"
#include "Engine/Math/PhysXMathUtils.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/UI/HealthBar.hpp"
#include "Game/Crawler.hpp"
#include "Game/Game.hpp"
#include "Game/openxr_program.h"
#include "Game/VRPlayer.hpp"
#include "Game/WinPlayer.hpp"
#include <vector>
#include <stdio.h>

extern OpenXrProgram* g_theApp;
extern Game* g_theGame;
extern InputSystem* g_theInput;
extern Window* g_theWindow;
extern ThePhysX* g_thePhysX;
extern VRPlayer* g_theVRPlayer;
extern WinPlayer* g_theWinPlayer;
extern RandomNumberGenerator* g_rng;
extern Clock* g_theGameClock;

using namespace std;
using namespace physx;

CrawlerLeg::CrawlerLeg(Chain* c, Vec3 initializePos)
		: m_chain(c)
		, m_footDefaultOffset(initializePos)
{
	string value = "hello";
	printf("%s\n", value.c_str());
	printf(value.c_str());
}

Crawler::Crawler(Vec3 const& pos /*= Vec3()*/, EulerAngles const& angle /*= EulerAngles()*/)
		: Actor(pos)
{
	m_initialAngle = angle;
}

Crawler::~Crawler()
{
	for (auto const& pair : m_skeleton->m_allJoints)
	{
		Joint* joint = pair.second;
		if (joint->m_rigidActor)
		{
			joint->m_rigidActor->release();
		}
	}

	if (m_skeleton)
	{
		delete m_skeleton;
	}
}

void Crawler::Startup()
{
	CreateCrawlerSkeleton();
	CreateDynamicRigidBodyForCrawler();

	// m_headRigidActor = CreateCapsuleShapeDynamicActorForTwoJoint(m_skeleton->GetJointByName("neck"), m_skeleton->GetJointByName("eye"), m_headCollisionRadius, m_headMass);
	// m_headRigidActor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
	for (int legIndex = 0; legIndex < m_legs.size(); ++legIndex) //
	{
		CrawlerLeg* leg = m_legs[legIndex];
		CreateLegDynamicActors(leg);
	}
	
	m_directionChangingTimer = new Timer(m_directionChangingPeroid);
	m_directionChangingTimer->Start();

	m_deathClearTimer = new Timer(m_deathClearDuration);
	m_setClearTimer = new Timer(m_setClearDuration);

	m_currentHealth = 500.f;
	m_maxHealth = 500.f;
	m_healthBar = new HealthBar(this, Vec2(0.5f, 0.1f), Vec2(0.1f, 0.2f), Vec3(0.f, 0.f, 1.8f), Rgba8::WHITE, Rgba8::RED);

	// m_isDead = true;
}

void Crawler::CreateCrawlerSkeleton()
{
	// create skeleton
	Actor* actor = static_cast<Actor*>(this);
	m_skeleton = new Skeleton("CrawlerSkeleton", actor);

	// create all joints
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
	Joint* eyeJoint = m_skeleton->AddJoint("eye", Vec3(1.1f, 0.f, 0.f), Quat(), neckJoint);

	(void)eyeJoint;
	 
	Joint* hip_front_L = m_skeleton->AddJoint("hip_front_L", Vec3(0.3f, 0.3f, 0.f),  ConvertEulerAnglesToQuat(EulerAngles(45.f, 30.f, 0.f)), pelvisJoint);
	Joint* hip_front_R = m_skeleton->AddJoint("hip_front_R", Vec3(0.3f, -0.3f, 0.f), ConvertEulerAnglesToQuat(EulerAngles(-45.f, 30.f, 0.f)), pelvisJoint);

	Joint* hip_back_L = m_skeleton->AddJoint("hip_back_L", Vec3(-0.3f, 0.3f, 0.f), ConvertEulerAnglesToQuat(EulerAngles(135.f, 30.f, 0.f)), pelvisJoint);
	Joint* hip_back_R = m_skeleton->AddJoint("hip_back_R", Vec3(-0.3f, -0.3f, 0.f), ConvertEulerAnglesToQuat(EulerAngles(-135.f, 30.f, 0.f)), pelvisJoint);

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
		leg->m_legRecordedForwordOrientation = EulerAngles();  //m_orientation;
	}

	m_skeleton->Startup();

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// load models
	leg_upper_FL->m_objModels.push_back(g_theGame->m_footModel);
	leg_upper_FR->m_objModels.push_back(g_theGame->m_footModel);
	leg_upper_BL->m_objModels.push_back(g_theGame->m_footModel);
	leg_upper_BR->m_objModels.push_back(g_theGame->m_footModel);

	hip_front_L->m_objModels.push_back(g_theGame->m_legModel);
	hip_front_R->m_objModels.push_back(g_theGame->m_legModel);
	hip_back_L->m_objModels.push_back(g_theGame->m_legModel);
	hip_back_R->m_objModels.push_back(g_theGame->m_legModel);

	pelvisJoint->m_objModels.push_back(g_theGame->m_baseModel);
	
	neckJoint->m_objModels.push_back(g_theGame->m_headModel);
	
	headJoint->m_objModels.push_back(g_theGame->m_gunModel);
}

// // Store a reference to the Actor in PhysX's userData field
// rigidActor->userData = this; // Store the VRPlayer pointer
// DebuggerPrintf("VRPlayer joint dynamic actor userData set: %p ", this);
// 
// rigidActor->setActorFlag(PxActorFlag::eVISUALIZATION, true);
// 
// m_physXMaterial = g_thePhysX->GetPhysXMaterialByStinrg("player");
// // using createExclusiveShape, the shape is automatically attached to the actor it is created for
// PxShape* aCapsuleShape = PxRigidActorExt::createExclusiveShape(*rigidActor,
// 	PxCapsuleGeometry(m_joint_capsuleRadius, boneLength * 0.5f), *m_physXMaterial);
// 
// PxFilterData filterData;
// filterData.word0 = 1; // Custom collision group
// filterData.word1 = 1; // Collides with same group
// filterData.word2 = 0;
// filterData.word3 = 0;
// 
// aCapsuleShape->setSimulationFilterData(filterData);
// aCapsuleShape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, true);
// 
// // set up mass and inertia and add to the physX scene
// PxRigidDynamic* dynamicActor = rigidActor->is<PxRigidDynamic>();
// if (dynamicActor)
// {
// 	PxRigidBodyExt::updateMassAndInertia(*dynamicActor, m_totalMass * 0.05f);
// 	g_thePhysX->m_scene->addActor(*rigidActor);
// }
// else
// {
// 	ERROR_AND_DIE("Failed to create dynamic rigid actor");
// }

void Crawler::CreateLegDynamicActors(CrawlerLeg* leg)
{
	Chain* chain = leg->m_chain;
	leg->m_upperLegActor = CreateCapsuleShapeDynamicActorForTwoJoint(chain->GetRootJoint(), chain->GetMidJoint(), m_upperLegCollisionRadius, m_upperLegMass);
	leg->m_lowerLegActor = CreateCapsuleShapeDynamicActorForTwoJoint(chain->GetMidJoint(), chain->GetEndJoint(), m_lowerLegCollisionRadius, m_lowerLegMass);

	PxRigidDynamic* crawlerMainBody = m_rigidActor->is<PxRigidDynamic>();

	// connect upper arm and lower arm with a D6 joint
	Vec3 rootJointPosInGame = chain->GetRootJoint()->GetWorldPosition();
	Vec3 midJointPosInGame = chain->GetMidJoint()->GetWorldPosition();
	Vec3 endJointPosInGame = chain->GetEndJoint()->GetWorldPosition();
	PxVec3 rootJointPos = GetPxVec3(GetGameToPhysXSpaceMat().TransformPosition3D(rootJointPosInGame));
	PxVec3 midJointPos = GetPxVec3(GetGameToPhysXSpaceMat().TransformPosition3D(midJointPosInGame));
	PxVec3 endJointPos = GetPxVec3(GetGameToPhysXSpaceMat().TransformPosition3D(endJointPosInGame));
	PxVec3 crawlerMainBodyPos = crawlerMainBody->getGlobalPose().p;

	// Vec3 dir_mainBody = rootJointPosInGame - GetPhysXToGameSpaceMat().TransformPosition3D(Vec3(crawlerMainBody->getGlobalPose().p));
	// Vec3 dir_upperLeg = rootJointPosInGame - GetPhysXToGameSpaceMat().TransformPosition3D(Vec3(leg->m_upperLegActor->getGlobalPose().p));
	PxVec3 dir_mainBody = rootJointPos - crawlerMainBody->getGlobalPose().p;
	PxVec3 dir_upperLeg = rootJointPos - leg->m_upperLegActor->getGlobalPose().p;
	// PxQuat rotationMainBody = GetPxQuat(TransformQuatFromGameToPhysX(Quat::GetQuatFromTwoVectors(Vec3(0.f, 1.f, 0.f), dir_mainBody)));
	// PxQuat rotationUpperLeg = GetPxQuat(TransformQuatFromGameToPhysX(Quat::GetQuatFromTwoVectors(Vec3(0.f, 1.f, 0.f), dir_upperLeg)));

	// Define the actor's "up" vector in local space (this depends on your model!)
	// Vec3 mainBodyLocalUp = GetPhysXToGameSpaceMat().TransformVectorQuantity3D(Vec3(crawlerMainBody->getGlobalPose().q.rotate(PxVec3(0.f, 1.f, 0.f))));
	// Vec3 upperLegLocalUp = GetPhysXToGameSpaceMat().TransformVectorQuantity3D(Vec3(leg->m_upperLegActor->getGlobalPose().q.rotate(PxVec3(0.f, 1.f, 0.f))));
	PxVec3 mainBodyLocalUp = crawlerMainBody->getGlobalPose().q.rotate(PxVec3(0.f, 1.f, 0.f));
	PxVec3 upperLegLocalUp = leg->m_upperLegActor->getGlobalPose().q.rotate(PxVec3(0.f, 1.f, 0.f));
	PxQuat rotationMainBody = PxShortestRotation(dir_mainBody, dir_mainBody);
	PxQuat rotationUpperLeg = PxShortestRotation(dir_upperLeg, dir_upperLeg);
	  
	// // // Compute rotation to align each actor's local "up" with the joint direction
	// PxQuat rotationMainBody = GetPxQuat(TransformQuatFromGameToPhysX(
	// 	Quat::GetQuatFromTwoVectors(mainBodyLocalUp, dir_mainBody)
	// ));
	// 
	// PxQuat rotationUpperLeg = GetPxQuat(TransformQuatFromGameToPhysX(
	// 	Quat::GetQuatFromTwoVectors(upperLegLocalUp, dir_upperLeg)
	// ));

	PxVec3 localPos_rootJoint_mainBody = (crawlerMainBody->getGlobalPose().transformInv(PxTransform(rootJointPos))).p;
	PxTransform mainBodyFrame(localPos_rootJoint_mainBody, GetPxQuat(Quat())); 

	PxVec3 localPos_rootJoint_upperLeg = (leg->m_upperLegActor->getGlobalPose().transformInv(PxTransform(rootJointPos))).p;
	PxTransform upperLegFrame(localPos_rootJoint_upperLeg, GetPxQuat(Quat())); 

	PxD6Joint* mainBodyToUpperLegJoint = PxD6JointCreate(*g_thePhysX->m_physics,
		crawlerMainBody, mainBodyFrame,
		leg->m_upperLegActor, upperLegFrame);
	
	chain->GetRootJoint()->m_D6Joint = mainBodyToUpperLegJoint;

	mainBodyToUpperLegJoint->setMotion(PxD6Axis::eSWING1, PxD6Motion::eFREE);
	mainBodyToUpperLegJoint->setMotion(PxD6Axis::eSWING2, PxD6Motion::eFREE);
	mainBodyToUpperLegJoint->setMotion(PxD6Axis::eTWIST, PxD6Motion::eFREE);

	PxD6JointDrive upperLegDrive(6000.0f, 500.0f, FLT_MAX, true);
	// Without eACCELERATION flag: The joint applies forces based on the mass of the bodies, so heavier bodies will experience slower acceleration than lighter bodies.
	// With eACCELERATION : The joint applies mass - independent acceleration, meaning both heavy and light bodies will experience the same acceleration, making it useful for controlling precise movements without mass - based scaling.
	upperLegDrive.flags |= PxD6JointDriveFlag::eACCELERATION;

	mainBodyToUpperLegJoint->setDrive(PxD6Drive::eSLERP, upperLegDrive);

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// connect upper arm and lower arm with a D6 joint
	dir_upperLeg = midJointPos - rootJointPos;
	PxVec3 dir_lowerLeg = midJointPos - endJointPos;

	// Define the actor's "up" vector in local space (this depends on your model!)
	Vec3 lowerLegLocalUp = GetPhysXToGameSpaceMat().TransformVectorQuantity3D(Vec3(leg->m_lowerLegActor->getGlobalPose().q.rotate(PxVec3(0.f, 1.f, 0.f))));

	// Compute rotation to align each actor's local "up" with the joint direction
	// rotationUpperLeg = GetPxQuat(TransformQuatFromGameToPhysX(
	// 	Quat::GetQuatFromTwoVectors(upperLegLocalUp, dir_upperLeg)
	// ));	
	// 
	// PxQuat rotationlowerLeg = GetPxQuat(TransformQuatFromGameToPhysX(
	// 	Quat::GetQuatFromTwoVectors(lowerLegLocalUp, dir_lowerLeg)
	// ));	
	// rotationUpperLeg = GetPxQuat(TransformQuatFromGameToPhysX(
	// 	Quat::GetQuatFromTwoVectors(upperLegLocalUp, dir_upperLeg)
	// ));
	// 
	// PxQuat rotationlowerLeg = GetPxQuat(TransformQuatFromGameToPhysX(
	// 	Quat::GetQuatFromTwoVectors(lowerLegLocalUp, dir_lowerLeg)
	// ));

	PxVec3 localPos_midJoint_upperLeg = (leg->m_upperLegActor->getGlobalPose().transformInv(PxTransform(midJointPos))).p;
	upperLegFrame = PxTransform(localPos_midJoint_upperLeg, GetPxQuat(Quat())); // Attach at root joint

	PxVec3 localPos_endJoint_midJoint = (leg->m_lowerLegActor->getGlobalPose().transformInv(PxTransform(midJointPos))).p;
	PxTransform lowerLegFrame(localPos_endJoint_midJoint, GetPxQuat(Quat())); // Relative to main body

	PxD6Joint* upperLegToLowerLegJoint = PxD6JointCreate(*g_thePhysX->m_physics,
		leg->m_upperLegActor, upperLegFrame,
		leg->m_lowerLegActor, lowerLegFrame);

	chain->GetMidJoint()->m_D6Joint = upperLegToLowerLegJoint;

	upperLegToLowerLegJoint->setMotion(PxD6Axis::eSWING1, PxD6Motion::eFREE);
	upperLegToLowerLegJoint->setMotion(PxD6Axis::eSWING2, PxD6Motion::eFREE);
	upperLegToLowerLegJoint->setMotion(PxD6Axis::eTWIST, PxD6Motion::eFREE);
	
	PxD6JointDrive lowerLegDrive(5100.0f, 450.0f, FLT_MAX, true);
	lowerLegDrive.flags |= PxD6JointDriveFlag::eACCELERATION;

	upperLegToLowerLegJoint->setDrive(PxD6Drive::eSLERP, lowerLegDrive);
}

void Crawler::AlignLegD6JointsAndUpdateJointsRotation(CrawlerLeg* leg)
{
	Quat adjustment = ConvertEulerAnglesToQuat(EulerAngles(-90.f, 0.f, 0.f));
		
	Joint* rootLegJoint = leg->m_chain->GetRootJoint();
	Joint* midLegJoint = leg->m_chain->GetMidJoint();

	Quat upperArmRotation = SetD6JointDriveAlignWithJointOrientation(rootLegJoint, adjustment);
	Quat lowerArmRotation = SetD6JointDriveAlignWithJointOrientation(midLegJoint, adjustment, false);
	
	// use physical animation data to present the blending result
	rootLegJoint->m_localRotation = rootLegJoint->m_parent->GetWorldQuat().GetInversed() * upperArmRotation;
	midLegJoint->m_localRotation = rootLegJoint->GetWorldQuat().GetInversed() * lowerArmRotation;
}

void Crawler::AlignLegD6JointsWithSkeleton(CrawlerLeg* leg)
{
	Quat adjustment = ConvertEulerAnglesToQuat(EulerAngles(-90.f, 0.f, 0.f));

	Joint* rootLegJoint = leg->m_chain->GetRootJoint();
	Joint* midLegJoint = leg->m_chain->GetMidJoint();

	Quat upperArmRotation = SetD6JointDriveAlignWithJointOrientation(rootLegJoint, adjustment);
	Quat lowerArmRotation = SetD6JointDriveAlignWithJointOrientation(midLegJoint, adjustment, false);
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

Quat Crawler::SetD6JointDriveAlignWithJointOrientation(Joint* skeletonJoint, Quat const& rotationAdjustment /*= Quat()*/, bool swing2RotationOnly /*= false*/)
{
	PxD6Joint* D6Joint = skeletonJoint->m_D6Joint;
	PxRigidActor* actor0 = nullptr;		// the rigid actor that the D6 joint is on
	PxRigidActor* actor1 = nullptr;		// the rigid actor that the D6 joint drive controls
	if (!D6Joint)
	{
		return Quat();
	}
	D6Joint->getActors(actor0, actor1);

	// get two joints world rotation
	Quat desiredJointOrientation = skeletonJoint->GetWorldQuat();
	Quat actor0_worldOrientation = TransformQuatFromPhysXToGameAndGetInversed(Quat(actor0->getGlobalPose().q));
	Quat D6Joint_localOrientation = TransformQuatFromPhysXToGameAndGetInversed(Quat(D6Joint->getLocalPose(PxJointActorIndex::eACTOR0).q));
	Quat D6JointWorldRotation = (actor0_worldOrientation * D6Joint_localOrientation).GetNormalized();

	// calculate the local rotation the D6 joint needs to make
	desiredJointOrientation *= rotationAdjustment; // this adjustment might caused by local frame of the joint
	Quat driveRotation = (D6JointWorldRotation.GetInversed() * desiredJointOrientation).GetNormalized();

	// do not change the D6 joint's location but update its orientation
	PxTransform t;
	t.q = GetPxQuat(TransformQuatFromGameToPhysXAndGetInversed(driveRotation));
	// t.p = GetPxVec3(GetGameToPhysXSpaceMat().TransformPosition3D(skeletonJoint->GetWorldPosition()));
	t.p = D6Joint->getDrivePosition().p;
	if (swing2RotationOnly)
	{
		// Rotate a basis vector and measure angle between input and reference
		PxVec3 original = PxVec3(1, 0, 0);               // Reference direction
		PxVec3 rotated = t.q.rotate(original);            // Rotated by full q

		// Project both onto XY plane
		original.x = 0;
		rotated.x = 0;
		original.normalize();
		rotated.normalize();

		// Find angle in XY plane (swing2 is rotation around Z)
		float dot = original.dot(rotated);
		dot = PxClamp(dot, -1.0f, 1.0f); // Prevent NaNs
		float angle = PxAcos(dot);

		// Determine sign of angle via cross product
		float sign = (original.cross(rotated).x >= 0.f) ? 1.0f : -1.0f;

		t.q = PxQuat(sign * angle, PxVec3(1.f, 0.f, 0.f));
	}
	D6Joint->setDrivePosition(t);

	// get the joint's world rotation by getting the orientation of the dynamic actor's that it's controlling
	return TransformQuatFromPhysXToGameAndGetInversed(Quat(actor1->getGlobalPose().q)) * rotationAdjustment.GetInversed();
}

physx::PxRigidDynamic* Crawler::CreateCapsuleShapeDynamicActorForTwoJoint(Joint* A, Joint* B, float radius, float mass)
{
	// create the capsule collision with joint positions
	Vec3 A_worldPos = A->GetWorldPosition();
	Vec3 B_worldPos = B->GetWorldPosition();
	Vec3 jointMiddlePos = (A_worldPos + B_worldPos) * 0.5f;
	float halfHeight = GetDistance3D(jointMiddlePos, B_worldPos) - radius;

	PxCapsuleGeometry capsuleGeom(radius, halfHeight);

	// align the capsule with the direction of the two joints
	Vec3 dir = (B_worldPos - A_worldPos);

	// the way to get the rotation using phsyX math library
	// PxVec3 up = GetPxVec3(GetGameToPhysXSpaceMat().TransformVectorQuantity3D(Vec3(0.f, 1.f, 0.f)));
	// PxQuat rotation = PxShortestRotation(up, GetPxVec3(GetGameToPhysXSpaceMat().TransformVectorQuantity3D(dir)));

	// the way to get the rotation in our math library
	PxQuat rotation = GetPxQuat(TransformQuatFromGameToPhysXAndGetInversed(Quat::GetQuatFromTwoVectors(Vec3(0.f, 1.f, 0.f), dir)));
	
	PxTransform transform(GetPxVec3(GetGameToPhysXSpaceMat().TransformVectorQuantity3D(jointMiddlePos)), rotation);
	PxRigidDynamic* actor = g_thePhysX->m_physics->createRigidDynamic(transform);

	PxMaterial* crawlerMaterial = g_thePhysX->GetPhysXMaterialByStinrg("crawler");
	PxShape* shape = PxRigidActorExt::createExclusiveShape(*actor, capsuleGeom, *crawlerMaterial);

	PxFilterData filterData;
	filterData.word0 = 2; // Custom collision group for crawler
	filterData.word1 = 1; // Collides with VR player
	filterData.word2 = 0;
	filterData.word3 = 0;

	shape->setSimulationFilterData(filterData);

	actor->userData = this;

	PxRigidBodyExt::updateMassAndInertia(*actor, mass);
	g_thePhysX->m_scene->addActor(*actor);

	A->m_rigidActor = actor;
	// todo: this method is wrong, find out why
	// Quat jointQuat = Quat::GetQuatFromTwoVectors(Vec3(0.f, 1.f, 0.f), dir);
	// Quat rigidActorQuat = TransformQuatFromPhysXToGame(Quat(actor->getGlobalPose().q));
	// A->m_relativeRotationToRigidActor = (rigidActorQuat.GetInversed() * jointQuat).GetNormalized();
	// 
	// A->m_relativeDispFromRigidActor = A->GetWorldPosition() - GetPhysXToGameSpaceMat().TransformPosition3D(Vec3(actor->getGlobalPose().p));
	// A->m_relativeDispFromRigidActor = rigidActorQuat.RotateVector(A->m_relativeDispFromRigidActor);

	// actor->attachShape(*shape);	// already createExclusiveShape
	// shape->release();	// ensure that this function doesn't keep unnecessary extra references

	// actor->attachShape(*shape);	// already createExclusiveShape
	// shape->release();	// ensure that this function doesn't keep unnecessary extra references

	Mat44 jointModelMatrix = A->GetWorldTransformMat(); // in game space
	Mat44 actorModelMatrix = GetPhysXToGameSpaceMat().SimilarityTransformation( Mat44(actor->getGlobalPose()) ); // convert PxTransform to Mat44

	// Store the inverse of actorModelMatrix, multiplied by jointModelMatrix
	Mat44 inverse = actorModelMatrix.GetOrthonormalInverse();
	A->m_relativeModelMatrixToRigidActor = inverse.MatMultiply(jointModelMatrix);

	// for debug
	// actor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);

	return actor;
}

void Crawler::UpdateDynamicActorForTwoJoints(Joint* A, Joint* B, physx::PxRigidDynamic* actor)
{
	if (!actor)
	{
		return;
	}

	// get the updated world positions of the joints
	Vec3 A_worldPos = A->GetWorldPosition();
	Vec3 B_worldPos = B->GetWorldPosition();

	// calculate the new joint middle position
	Vec3 jointMiddlePos = (A_worldPos + B_worldPos) * 0.5f;

	// recalculate the direction
	Vec3 dir = (B_worldPos - A_worldPos);

	// recalculate the rotation to align the capsule with the direction of the two joints
	PxQuat rotation = GetPxQuat(TransformQuatFromGameToPhysXAndGetInversed(Quat::GetQuatFromTwoVectors(Vec3(0.f, 1.f, 0.f), dir)));

	// create a new transform for the rigid actor based on the updated position and rotation
	PxTransform newTransform(GetPxVec3(GetGameToPhysXSpaceMat().TransformPosition3D(jointMiddlePos)), rotation);

	Vec3 actorPos = GetPhysXToGameSpaceMat().TransformPosition3D(Vec3(actor->getGlobalPose().p));

	// update the rigid actor's pose
	actor->setKinematicTarget(newTransform);
}

void Crawler::UpdateAnimState()
{
	m_skeleton->m_rootJoint->m_D6Joint->setDrivePosition(m_desiredRootD6JointTransform);

	// when the actor's speed is lower than a certain value
	// we can assume the crawler is recovered from hit
	// is the speed is increasing, we assume it is knocked off and accelerating
	// we also need to check if the main body is get back upright
	if (m_crawlerCurrentFrameState == CrawlerAnimMode::HIT && (m_speedLastFrame < m_speed || m_speed >= m_speedToRecoverFromHit))
	{
		// Log::Write(Log::Level::Info, Stringf("SpeedWhenHit: %.2f", m_speed));
		// update the max speed after hit
		m_maxSpeedAfterHit = max(m_maxSpeedAfterHit, m_speed);
		return;
	}	
	else if(m_crawlerCurrentFrameState == CrawlerAnimMode::HIT)
	{
		// reset max hit speed after hit
		m_maxSpeedAfterHit = 0.f;
		// float timeRecover = m_panicClock->GetTotalSeconds(); 
		m_crawlerCurrentFrameState = CrawlerAnimMode::ROTATING;
		// Log::Write(Log::Level::Info, Stringf("Recovered: %.2f", timeRecover));
	}

	// lock 
	// m_skeleton->m_rootJoint->m_D6Joint->setMotion(PxD6Axis::eSWING2, PxD6Motion::eLOCKED);

	// the crawler will make rotating be a priority
	if (m_crawlerCurrentFrameState == CrawlerAnimMode::ROTATING)
	{
		// Vec3 crossDotproduct = CrossProduct3D(m_orientation.GetForwardIBasis(), (g_theWinPlayer->m_movingTarget_endEffector - m_position).GetNormalized());
		// if (crossDotproduct.z > 0.f)
		// {
		// 	m_rotatingCounterClockwise = true;
		// }
		// else
		// {
		// 	m_rotatingCounterClockwise = false;
		// }

		if (CheckIfLegShouldRotate())
		{
			return;	// if the crawler is rotating, no need to do other animations
		}
	}

	// update animation status
	constexpr float minSpeedThreshold = 0.03f; // m/s
	if (m_isDriving || m_speed > minSpeedThreshold)
	{
		m_crawlerCurrentFrameState = CrawlerAnimMode::WALKING;
	}
	else if (m_crawlerCurrentFrameState == CrawlerAnimMode::ADJUSTING)
	{
		if (!CheckIfCrawlerNeedsToAdjustPose())
		{
			m_crawlerCurrentFrameState = CrawlerAnimMode::IDLE;
		}
		else
		{
			return;
		}
	}
	else // if (AreAllLegsGrounded())
	{
		m_crawlerCurrentFrameState = CrawlerAnimMode::IDLE;
	}
}
 
void Crawler::UpdateProceduralAnimationByCurrentState()
{
	if (m_isDead)
	{
		m_crawlerCurrentFrameState = CrawlerAnimMode::DEAD;
	}

	switch (m_crawlerCurrentFrameState)
	{
		case CrawlerAnimMode::IDLE : 
		{
			if (m_crawlerCurrentFrameState != m_crawlerLastFrameState)
			{
				if (m_idleTimer)
				{
					m_idleTimer->Restart();	
				}
				else
				{
					m_idleTimer = new Timer(m_idlePeroid);
				}
			}

			// set the pelvis up and down by sin for acting breathing anim
			if (m_idleTimer->HasPeroidElapsed())
			{
				m_idleTimer->Start();
			}
			float heightOffsetMultipler = m_idleHeightOffset * sinf(m_idleTimer->GetElapsedTime() * 1.25f);

			Joint* pelvis = m_skeleton->GetJointByName("pelvis");

			pelvis->SetLocalBonePos(Vec3(m_pelvisHeight * (heightOffsetMultipler + 1.f), 0.f, 0.f));

			// keep the foot default offset
			for (int legIndex = 0; legIndex < m_legs.size(); ++legIndex)
			{
				CrawlerLeg* leg = m_legs[legIndex];
				Vec3 footWorldPos = m_position + m_orientation.GetAsMatrix_XFwd_YLeft_ZUp().TransformVectorQuantity3D(leg->m_footDefaultOffset);
				footWorldPos.z = 0.f;

				Chain* chain = m_legs[legIndex]->m_chain;
				chain->m_solver->SetSolverTarget(footWorldPos);

				chain->m_solver->SetPoleVector(footWorldPos + Vec3(0.f, 0.f, -1.f));

				chain->m_solver->SolveIK();

				AlignLegD6JointsWithSkeleton(leg);
				// AlignLegD6JointsAndUpdateJointsRotation(leg);
			}
		} break;

		case CrawlerAnimMode::ROTATING:
		{
			// if in current frame we switch to walking animation state from other states
			// weFll set up its timer and stay pos
			if (m_crawlerCurrentFrameState != m_crawlerLastFrameState)
			{
				for (int legIndex = 0; legIndex < m_legs.size(); ++legIndex)
				{
					CrawlerLeg* leg = m_legs[legIndex];

					if (leg->m_chain->m_timer)
					{
						// change the rotation peroid by the rotation changes
						float rotationSpeed = GetFractionWithinRange(abs(m_desiredOrientation.m_yawDegrees - m_orientation.m_yawDegrees), m_legRotationAngleBoundary, 150.f);
						rotationSpeed = GetClamped(rotationSpeed, 0.f, 1.f);
						float peroid = RangeMapClamped(rotationSpeed, 0.f, 1.f, m_rotatingPeroid, m_rotatingPeroid * 0.4f);

						leg->m_chain->m_timer->m_period = peroid;
						leg->m_chain->m_timer->Stop();
					}
					else
					{
						leg->m_chain->m_timer = new Timer(m_rotatingPeroid);
					}

					Joint* j = leg->m_chain->GetEndJoint();
					leg->m_animStayPos = j->GetWorldPosition();
					leg->m_animStayPos.z = 0.f;
				}
			}

			// for each leg, if the leg should move and the moving legs nums is <= half
			// allow it to move the cycle
			// int num = GetNumLegsShouldMove();
			int numOfLegsAllowToMove = 2;
			for (int legIndex = 0; legIndex < m_legs.size(); ++legIndex)
			{
				CrawlerLeg* leg = m_legs[legIndex];
				// if this leg is allowed to move or this leg is already moving
				// only if the timer has not finished
				float rotatingFraction = leg->m_chain->m_timer->GetElapsedFraction();
				if ((rotatingFraction > 0.f && rotatingFraction < 1.f)) --numOfLegsAllowToMove;
			}

			for (int legIndex = 0; legIndex < m_legs.size(); ++legIndex)
			{
				CrawlerLeg* leg = m_legs[legIndex];
				Chain* chain = m_legs[legIndex]->m_chain;
				Timer* timer = m_legs[legIndex]->m_chain->m_timer;
				Joint* legRootJoint = chain->GetRootJoint();
				// if this leg is allowed to move or this leg is already moving
				// only if the timer has not finished
				float rotatingFraction = leg->m_chain->m_timer->GetElapsedFraction();

				if ( (numOfLegsAllowToMove > 0 && timer->IsStopped() ) || (rotatingFraction > 0.f && rotatingFraction < 1.f) )	// && leg->m_shouldRotate
				{
					if (numOfLegsAllowToMove > 0 && timer->IsStopped())// && leg->m_shouldRotate)
					{
						--numOfLegsAllowToMove;
					}

					// if the leg just started to move, record where it starts to move
					if (timer->IsStopped())
					{
						chain->m_timer->Start();
						leg->m_animCycleStartPos = leg->m_chain->GetEndJoint()->GetWorldPosition();
						leg->m_animStayRootOrientation = leg->m_chain->GetRootJoint()->m_localRotation;
						leg->m_animStayPos.z = 0.f;

						float symbol = GetShortestAngularDispDegrees(m_orientation.m_yawDegrees, m_desiredOrientation.m_yawDegrees);
						if (symbol > 0.f)
						{
							symbol = 1.f;
						}
						else
						{
							symbol = -1.f;
						}
						EulerAngles newOrientation = m_orientation;
						newOrientation.m_yawDegrees += 30.f * symbol;


						leg->m_footRotationExpectedPos = m_position + newOrientation.GetAsMatrix_XFwd_YLeft_ZUp().TransformVectorQuantity3D(leg->m_footDefaultOffset);
						leg->m_legRecordedForwordOrientation = m_orientation;
					}

					Vec3 newFootPos;

					// if more than two legs are rotating, lower the timer's period
					// if (numOfLegsAllowToMove < 0)
					// {
					// 	int i = 0;
					// }
					// else
					// {
					// 	timer->m_period = m_rotatingPeroid * 0.5f;
					// }

					// based on timer's fraction, lerp the foot position and leg root rotation
					// leg->m_chain->GetRootJoint()->m_localRotation = Quat::Nlerp(leg->m_animStayRootOrientation, leg->m_legRootDefaultLocalOrientation, rotatingFraction);

					// horizontal


					Vec3 footOffsetHorizontal = Interpolate(leg->m_animCycleStartPos, leg->m_footRotationExpectedPos, rotatingFraction);

					// upward
					float cycleFraction = RangeMapClamped(rotatingFraction, 0.f, 1.f, 0.f, PI);
					Vec3 footOffsetUpward = Vec3(0.f, 0.f, sinf(cycleFraction) * 0.35f);

					// string debugStr = Stringf("disp: %.2f", footOffsetUpward.z);
					// DebugAddScreenText(debugStr, Vec2(), 20.f, Vec2(0.5f, 0.5f), -1.f, Rgba8::WHITE);

					newFootPos = footOffsetHorizontal + footOffsetUpward;

					// keep the upper leg part model upright before solving
					// leg->m_chain->GetRootJoint()->m_localRotation = leg->m_legRootDefaultLocalOrientation;

					// solve new world pos
					chain->m_solver->SetSolverTarget(newFootPos);
					chain->m_solver->SetPoleVector(newFootPos + Vec3(0.f, 0.f, -1.f));
					AlignLegRootJointWithTarget(leg);
					chain->m_solver->SolveIK();

					AlignLegD6JointsAndUpdateJointsRotation(leg);

					DebugAddWorldWireSphere(newFootPos, 0.05f, m_walkingTimerPeroid_max * .5f, Rgba8::BLUE, Rgba8::BLUE, DebugRenderMode::X_RAY);// leg->m_footRotationExpectedPos
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
					AlignLegRootJointWithTarget(leg);
					chain->m_solver->SolveIK();

					AlignLegD6JointsAndUpdateJointsRotation(leg);
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
						leg->m_chain->m_timer->m_period = m_adjustingTimerPeroid;
						leg->m_chain->m_timer->Stop();
					}
					else
					{
						leg->m_chain->m_timer = new Timer(m_adjustingTimerPeroid);
					}

					Joint* j = leg->m_chain->GetEndJoint();
					leg->m_animStayPos = j->GetWorldPosition();
					leg->m_animStayPos.z = 0.f;
				}
			}

			// for each leg, if the leg should move and the moving legs nums is <= half
			// allow it to move the cycle
			// int num = GetNumLegsShouldMove();
			int numOfLegsAllowToMove = 2;

			for (int legIndex = 0; legIndex < m_legs.size(); ++legIndex)
			{
				CrawlerLeg* leg = m_legs[legIndex];
				float adjustingFraction = leg->m_chain->m_timer->GetElapsedFraction();
				if ((adjustingFraction > 0.f && adjustingFraction < 1.f)) --numOfLegsAllowToMove;
			}

			for (int legIndex = 0; legIndex < m_legs.size(); ++legIndex)
			{
				CrawlerLeg* leg = m_legs[legIndex];
				Chain* chain = m_legs[legIndex]->m_chain;
				Timer* timer = m_legs[legIndex]->m_chain->m_timer;
				// if this leg is allowed to move or this leg is already moving
				// only if the timer has not finished
				float adjustingFraction = leg->m_chain->m_timer->GetElapsedFraction();

				if ( (numOfLegsAllowToMove > 0 && timer->IsStopped() && leg->m_shouldAdjust) || (adjustingFraction > 0.f && adjustingFraction < 1.f)	)
				{

					// if the leg just started to move, record where it starts to move
					if (timer->IsStopped())
					{
						--numOfLegsAllowToMove;

						chain->m_timer->Start();
						leg->m_animCycleStartPos = leg->m_chain->GetEndJoint()->GetWorldPosition();
						leg->m_animStayRootOrientation = leg->m_chain->GetRootJoint()->m_localRotation;
						// leg->m_animStayPos.z = 0.f;
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
					Vec3 footOffsetUpward = Vec3(0.f, 0.f, sinf(cycleFraction) * 0.1f);

					// string debugStr = Stringf("disp: %.2f", footOffsetUpward.z);
					// DebugAddScreenText(debugStr, Vec2(), 20.f, Vec2(0.5f, 0.5f), -1.f, Rgba8::WHITE);

					newFootPos = footOffsetHorizontal; // + footOffsetUpward;
					 

					// solve new world pos
					chain->m_solver->SetSolverTarget(newFootPos);
					chain->m_solver->SetPoleVector(newFootPos + Vec3(0.f, 0.f, -1.f));
					AlignLegRootJointWithTarget(leg);
					chain->m_solver->SolveIK();

					AlignLegD6JointsAndUpdateJointsRotation(leg);

					DebugAddWorldWireSphere(footExpectedPos, 0.05f, m_walkingTimerPeroid_max * 0.3f, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::X_RAY);
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
					 
					AlignLegD6JointsAndUpdateJointsRotation(leg);
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
						leg->m_chain->m_timer->m_period = m_walkingTimerPeroid_max;
						leg->m_chain->m_timer->Stop();
					}
					else
					{
						leg->m_chain->m_timer = new Timer(m_walkingTimerPeroid_max);
					}

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
				// if this leg is allowed to move or this leg is already moving
				// only if the timer has not finished
				float rotatingFraction = leg->m_chain->m_timer->GetElapsedFraction();
				if ((rotatingFraction > 0.f && rotatingFraction < 1.f)) --numOfLegsAllowToMove;
			}

			for (int legIndex = 0; legIndex < m_legs.size(); ++legIndex)
			{
				CrawlerLeg* leg = m_legs[legIndex];
				Chain* chain = m_legs[legIndex]->m_chain;
				Timer* timer = m_legs[legIndex]->m_chain->m_timer;
				// if this leg is allowed to move or this leg is already moving
				// only if the timer has not finished
				float walkingFraction = leg->m_chain->m_timer->GetElapsedFraction();

				if ((numOfLegsAllowToMove > 0 && timer->IsStopped() && m_legsShouldStartToWalk) || (walkingFraction > 0.f && walkingFraction < 1.f)) // && leg->m_shouldMove
				{
					if ((numOfLegsAllowToMove > 0 && timer->IsStopped() && m_legsShouldStartToWalk))
					{
						--numOfLegsAllowToMove;
					}

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
					AlignLegRootJointWithTarget(leg);
					chain->m_solver->SolveIK();

					AlignLegD6JointsAndUpdateJointsRotation(leg);

					DebugAddWorldWireSphere(newFootPos, 0.05f, m_walkingTimerPeroid_max * 0.3f, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::X_RAY);
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

					AlignLegD6JointsAndUpdateJointsRotation(leg);
				}
			}
		}break;

		case CrawlerAnimMode::HIT:
		{
			// get the weight for panic animation and walking animation
			// float panicFraction = m_panicTimer->GetElapsedFraction();
			float panicFraction = GetFractionWithinRange(m_speed, m_speedToRecoverFromHit, m_maxSpeedAfterHit);
			float walkingAnimWeight = 1.f - panicFraction;

			// Log::Write(Log::Level::Info, Stringf("panicFraction: %.2f", panicFraction));
			
			// change the pelvis to perform squatting action
			// if (!m_squatTimer->IsStopped())
			// {	
			// 	float squatFraction = m_squatTimer->GetElapsedFraction();
			// 
			// 	Joint* pelvisJoint = m_skeleton->GetJointByName("pelvis");
			// 	pelvisJoint->
			// }

			// for panic animation, we interpolate the leg towards the legs' collision actors pose
			for (int legIndex = 0; legIndex < m_legs.size(); ++legIndex)
			{
				CrawlerLeg* leg = m_legs[legIndex];
				Chain* chain = leg->m_chain;

				if (leg->m_wasHit)
				{
					InterpolateJointWithItsD6Joint(chain->GetRootJoint(), panicFraction);
					InterpolateJointWithItsD6Joint(chain->GetMidJoint(), panicFraction);
				}
			}

			// adjust D6 joint stiffness to adjust the pose
			float stiffness = GetFractionWithinRange(panicFraction, 90.f, 30.f);
			PxD6JointDrive swingDrive(stiffness, 12.0f, PX_MAX_F32); // Stiffness, damping, force limit

			PxD6Joint* D6Joint = m_skeleton->GetJointByName("root")->m_D6Joint;
			D6Joint->setDrive(PxD6Drive::eSLERP, swingDrive);       // Stabilize swing motions
			D6Joint->setDrivePosition(m_desiredRootD6JointTransform);

			PxRigidDynamic* dynamicActor = m_rigidActor->is<PxRigidDynamic>();
			PxVec3 angularVelocity = dynamicActor->getAngularVelocity();
			Log::Write(Log::Level::Info, Fmt("Angular Velocity: X = %.02f,Y = %.02f, Z = %.02f", angularVelocity.x, angularVelocity.y, angularVelocity.z));

			// if the 
			// if (panicFraction == 1.f)
			// {
				//for (int legIndex = 0; legIndex < m_legs.size(); ++legIndex)
				//{
				//	CrawlerLeg* leg = m_legs[legIndex];

				//	if (!leg->m_chain->m_timer)
				//	{
				//		leg->m_chain->m_timer = new Timer(m_walkingTimer_hit);
				//	}

				//	Joint* j = leg->m_chain->GetEndJoint();
				//	leg->m_animStayPos = j->GetWorldPosition();
				//	leg->m_legCurrentAnimState = CrawlerAnimMode::HIT;

				//	if (leg->m_chain->m_timer->IsStopped())
				//	{
				//		// Joint* j = leg->m_chain->GetEndJoint();
				//		leg->m_animStayPos = m_position + m_orientation.GetAsMatrix_XFwd_YLeft_ZUp().TransformVectorQuantity3D(leg->m_footDefaultOffset);
				//		leg->m_legCurrentAnimState = CrawlerAnimMode::HIT;
				//	}
				//}
				// break;
			// }
			// else if(m_speed < m_speedToRecoverFromHit && panicFraction > 0.5f)
			// {
			// 	for (int legIndex = 0; legIndex < m_legs.size(); ++legIndex)
			// 	{
			// 		CrawlerLeg* leg = m_legs[legIndex];
			// 		Chain* chain = m_legs[legIndex]->m_chain;
			// 		if (!leg->m_wasHit)
			// 		{
			// 			// if the leg is in walking cycle and not allowed to walk
			// 			// stop its timer
			// 			leg->m_chain->m_timer->Stop();
			// 			leg->m_legCurrentAnimState = CrawlerAnimMode::IDLE;
			// 
			// 			// if the leg's animation state changes, update its stay position
			// 			if (leg->m_legLastFrameAnimState != leg->m_legCurrentAnimState)
			// 			{
			// 				leg->m_animStayPos = chain->GetEndJoint()->GetWorldPosition();
			// 				leg->m_animStayPos.z = 0.f;
			// 			}
			// 
			// 			// keep the upper leg part model upright
			// 			leg->m_chain->GetRootJoint()->m_localRotation = leg->m_legRootDefaultLocalOrientation;
			// 
			// 			// solve by the stay pos
			// 			chain->m_solver->SetSolverTarget(leg->m_animStayPos);
			// 			chain->m_solver->SetPoleVector(leg->m_animStayPos + Vec3(0.f, 0.f, -1.f));
			// 			chain->m_solver->SolveIK();
			// 		}
			// 	}
			// }

			if (panicFraction > 0.5f)
			{
				for (int legIndex = 0; legIndex < m_legs.size(); ++legIndex)
				{
					CrawlerLeg* leg = m_legs[legIndex];
					Chain* chain = m_legs[legIndex]->m_chain;
					if (!leg->m_wasHit)
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

				return;
			}

			// for walking animation, we interpolate the leg towards the walking procedural animation pose
			int numOfLegsAllowToMove = 2;
			for (int legIndex = 0; legIndex < m_legs.size(); ++legIndex)
			{
				CrawlerLeg* leg = m_legs[legIndex];
				Chain* chain = m_legs[legIndex]->m_chain;
				Timer* timer = m_legs[legIndex]->m_chain->m_timer;
				// if this leg is allowed to move or this leg is already moving
				// only if the timer has not finished
				float walkingFraction = leg->m_chain->m_timer->GetElapsedFraction();
				if((walkingFraction > 0.f && walkingFraction < 1.f)) --numOfLegsAllowToMove;

				if (((numOfLegsAllowToMove > 0 && leg->m_shouldMove && timer->IsStopped()) || (walkingFraction > 0.f && walkingFraction < 1.f)) && !leg->m_wasHit)
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
					// float newPeriodDuration = RangeMapClamped(m_speed, 0.f, m_maxMovingSpeed, m_walkingTimerPeroid_max, m_walkingTimerPeroid_min);
					// timer->m_period = newPeriodDuration;

					// based on timer's fraction, located new world pos
					// calculate about the step
					Vec3 stepLength = CalculateLegWalkStepByCrawlerVelocity(leg, true);
					Vec3 footOffsetForward = stepLength * walkingFraction;

					float cycleFraction = RangeMapClamped(walkingFraction, 0.f, 1.f, 0.f, PI);
					Vec3 footOffsetUpward = Vec3(0.f, 0.f, sinf(cycleFraction) * 0.5f);

					newFootPos = leg->m_animCycleStartPos + footOffsetForward + footOffsetUpward;

					// solve new world pos for the copied chain
					chain->m_solver->SetSolverTarget(newFootPos);
					chain->m_solver->SetPoleVector(newFootPos + Vec3(0.f, 0.f, -1.f));
					chain->m_solver->SolveIKWithWeight(walkingAnimWeight);

					AlignLegD6JointsAndUpdateJointsRotation(leg);

					DebugAddWorldWireSphere(leg->m_animStayPos, 0.05f, m_walkingTimerPeroid_max * 0.3f, Rgba8::GREEN, Rgba8::GREEN, DebugRenderMode::X_RAY);
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
					if (!leg->m_wasHit)
					{
						chain->m_solver->SolveIK();
						AlignLegD6JointsAndUpdateJointsRotation(leg);
					}
					else
					{
						chain->m_solver->SolveIKWithWeight(walkingAnimWeight);
						AlignLegD6JointsAndUpdateJointsRotation(leg);
					}
					// DebugAddWorldWireSphere(leg->m_animStayPos, 0.05f, m_walkingTimerPeroid_max * 0.3f, Rgba8::RED, Rgba8::RED, DebugRenderMode::X_RAY);
				}
			 }

		} break;

		case CrawlerAnimMode::DEAD:
		{
			m_skeleton->m_renderByRigidActor = true;
		}break;
	}
}

void Crawler::InterpolateJointWithItsD6Joint(Joint* j, float weight)
{
	// get the orientation from its D6 joint, which means the orientation of the rigid actor
	PxD6Joint* D6Joint = j->m_D6Joint;
	if (!D6Joint)
	{
		ERROR_AND_DIE("This joint does not have a D6 joint");
	}
	PxRigidActor* actor0 = nullptr;		// the rigid actor that the D6 joint is on
	PxRigidActor* actor1 = nullptr;		// the rigid actor that the D6 joint drive controls
	D6Joint->getActors(actor0, actor1);
	Quat actor0_worldOrientation = TransformQuatFromPhysXToGameAndGetInversed(Quat(actor0->getGlobalPose().q));

	Quat D6Joint_localOrientation = TransformQuatFromPhysXToGameAndGetInversed(Quat(D6Joint->getLocalPose(PxJointActorIndex::eACTOR0).q));
	Quat D6JointWorldRotation = (actor0_worldOrientation * D6Joint_localOrientation).GetNormalized();

	Quat currentOrientation = j->GetWorldQuat();
	Quat desiredOrientation = Quat::Nlerp(currentOrientation, D6JointWorldRotation, weight);

	// no need to interpolate the position of each joint, we control through changing local rotation
	// Vec3 D6Joint_localPos = GetPhysXToGameSpaceMat().TransformPosition3D(Vec3(D6Joint->getLocalPose(PxJointActorIndex::eACTOR0).p));
	// Vec3 actor0_worldPos = GetPhysXToGameSpaceMat().TransformPosition3D(Vec3(actor0->getGlobalPose().p));
	// Vec3 D6Joint_worldPos = actor0_worldPos + actor0_worldOrientation.RotateVector(D6Joint_localPos);
	// Interpolate(j->GetWorldPosition(), D6Joint_worldPos, weight);

	j->SetJointLocalOrientationByWorldOrientation(desiredOrientation);
}

Vec3 Crawler::CalculateLegWalkStepByCrawlerVelocity(CrawlerLeg* leg, bool IsPredicting /*= false*/)
{
	// get the time left for this leg to move
	Timer* timer = leg->m_chain->m_timer;
	float timerPeroid = timer->m_period;

	// get the predicted crawler position
	Vec3 expectedCrawlerPos = timerPeroid * m_velocity + m_position;

	Vec3 additionWalkingDisp = Vec3::ZERO;
	if (IsPredicting)
	{
		// if the crawler moving faster, we want the foot to move further to the default offset
		float predictDist = RangeMapClamped(m_speed, 0.f, m_maxMovingSpeed, m_maxPredictionDist * 0.2f, m_maxPredictionDist);
		additionWalkingDisp = m_velocityDir * predictDist;
	}

	Vec3 expectedFootPos = expectedCrawlerPos + m_orientation.GetAsMatrix_XFwd_YLeft_ZUp().TransformVectorQuantity3D(leg->m_footDefaultOffset) + additionWalkingDisp;
	Vec3 lastFootStayPos = leg->m_animStayPos;

	Vec3 disp = expectedFootPos - lastFootStayPos;

	// string debugStr = Stringf("disp: %.2f, %.2f, %.2f", disp.x, disp.y, disp.z);
	// // string debugStr = Stringf("dist: %.2f", predictDist);
	// DebugAddScreenText(debugStr, Vec2(), 20.f, Vec2(0.5f, 0.5f), -1.f, Rgba8::WHITE);

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
	m_legsShouldStartToWalk = false;
	for (int legIndex = 0; legIndex < m_legs.size(); ++legIndex)
	{
		CrawlerLeg* leg = m_legs[legIndex];
		Chain* chain = leg->m_chain;
		float crtDistRootToEnd = chain->GetDistanceFromRootToEnd();

		Vec3 footExpectedPos = m_position + m_orientation.GetAsMatrix_XFwd_YLeft_ZUp().TransformVectorQuantity3D(leg->m_footDefaultOffset);
		Vec3 footCurrentPos = chain->GetEndJoint()->GetWorldPosition();

		// if current is advance of the expected pos
		// no need to move
		Vec3 disp =  footCurrentPos - footExpectedPos;
		float dotProduct_dir = DotProduct3D(m_velocityDir, disp);
		if (dotProduct_dir > 0.f)
		{
			leg->m_shouldMove = false;
			continue;
		}

		float dist = GetDistanceSquared3D(footExpectedPos, footCurrentPos);

		// if the chain is stretched and need to move
		// or the foot is not backed to the original place
		if (crtDistRootToEnd >= (leg->m_chainLength * 0.85f) || crtDistRootToEnd <= (leg->m_chainLength * 0.6f) || dist > 0.01f)
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

bool Crawler::CheckIfLegShouldRotate()
{
	bool shouldRotate = false;
	for (int legIndex = 0; legIndex < m_legs.size(); ++legIndex)
	{
		CrawlerLeg* leg = m_legs[legIndex];
		// Chain* chain = leg->m_chain;

		float rotationDif = abs(m_orientation.m_yawDegrees - leg->m_legRecordedForwordOrientation.m_yawDegrees);

		// float GetShortestAngularDispDegrees()

			// if (m_desiredOrientation.m_yawDegrees - m_orientation.m_yawDegrees > 0.f) // 150, 120
			// {
			// 	if (rotationDif > m_legRotationAngleBoundary)//  m_legRotationAngleBoundary)// + 60.f)
			// 	{
			// 		leg->m_shouldRotate = true;
			// 		shouldRotate = true;
			// 	}
			// 	else
			// 	{
			// 		leg->m_shouldRotate = false;
			// 	}
			// }
			// else // -150, -120
			// {
			// 	if (rotationDif < -m_legRotationAngleBoundary)//  m_legRotationAngleBoundary)// + 60.f)
			// 	{
			// 		leg->m_shouldRotate = true;
			// 		shouldRotate = true;
			// 	}
			// 	else
			// 	{
			// 		leg->m_shouldRotate = false;
			// 	}
			// }
		if (leg->m_chain->m_timer)
		{
			float rotatingFraction = leg->m_chain->m_timer->GetElapsedFraction();
			if ((rotatingFraction > 0.f && rotatingFraction < 1.f))
			{
				leg->m_shouldRotate = true;
				shouldRotate = true;
				continue;
			}
		}

		if (rotationDif > m_legRotationAngleBoundary)
		{
			leg->m_shouldRotate = true;
			shouldRotate = true;
		}
		else
		{
			leg->m_shouldRotate = false;
		}
	}

	// string debugStr;;
	// if (shouldRotate)
	// {
	// 	debugStr = "TRUE";
	// 	DebugAddScreenText(debugStr, Vec2(), 20.f, Vec2(0.5f, 0.5f), -1.f, Rgba8::GREEN);
	// }
	// else
	// {
	// 	debugStr = "FALSE";
	// 	DebugAddScreenText(debugStr, Vec2(), 20.f, Vec2(0.5f, 0.5f), -1.f, Rgba8::RED);
	// }

	return shouldRotate;
}

bool Crawler::CheckIfCrawlerNeedsToAdjustPose()
{
	// if the crawler is already in IDLE, no need to check
	if (m_crawlerCurrentFrameState == CrawlerAnimMode::IDLE)
	{
		return false;
	}

	// check if four legs are back to default place
	// if not, then no need to act the breathing of pelvis going up and down
	bool result = false;
	for (int legIndex = 0; legIndex < m_legs.size(); ++legIndex)
	{
		CrawlerLeg* leg = m_legs[legIndex];
		Chain* chain = m_legs[legIndex]->m_chain;

		Vec3 footExpectedPos = m_position + m_orientation.GetAsMatrix_XFwd_YLeft_ZUp().TransformVectorQuantity3D(leg->m_footDefaultOffset);
		Vec3 footCurrentPos = chain->GetEndJoint()->GetWorldPosition();
		footExpectedPos.z = 0.f;
		footCurrentPos.z = 0.f;

		float dist = GetDistance3D(footExpectedPos, footCurrentPos);
		Quat currentRootRotation = leg->m_chain->GetRootJoint()->m_localRotation;
		bool orientationStored = currentRootRotation.IsMostEqual( leg->m_legRootDefaultLocalOrientation, 0.05f );
		if ( dist >= 0.06f || !orientationStored) //0.03125f
		{
			leg->m_shouldAdjust = true;
			result = true;
		}
		else
		{
			leg->m_shouldAdjust = false;
		}
	}

	// string debugStr;;
	// if (result)
	// {
	// 	debugStr = "TRUE";
	// 	DebugAddScreenText(debugStr, Vec2(), 20.f, Vec2(0.5f, 0.5f), -1.f, Rgba8::GREEN);
	// }
	// else
	// {
	// 	debugStr = "FALSE";
	// 	DebugAddScreenText(debugStr, Vec2(), 20.f, Vec2(0.5f, 0.5f), -1.f, Rgba8::RED);
	// }

	return result;
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

void Crawler::CreateDynamicRigidBodyForCrawler()
{
	// get physX transform info
	Vec3 actorCenter = m_position + Vec3(0.f, 0.f, m_capsuleHalfHeight + m_capsuleRadius);
	Mat44 posMat(actorCenter);
	posMat = GetGameToPhysXSpaceMat().MatMultiply(posMat);
	PxVec3 pxPos = GetPxVec3(posMat.GetTranslation3D());
	Quat quat; // (m_orientation);
	PxQuat pxQuat = GetPxQuat(TransformQuatFromGameToPhysXAndGetInversed(quat));
	PxTransform pxTransform(pxPos, pxQuat);

	// create capsule actor and rotate to along the +Y direction
	m_rigidActor = g_thePhysX->m_physics->createRigidDynamic(pxTransform);
	m_rigidActor->setActorFlag(PxActorFlag::eVISUALIZATION, true);

	PxTransform relativePose(PxQuat(PxHalfPi, PxVec3(0.f, 0.f, 1.f)));
	m_physXMaterial = g_thePhysX->CreatePhysXMaterial("crawler", m_dynamicFriction, m_staticFriction, m_restitution);
	// using createExclusiveShape, the shape is automatically attached to the actor it is created for
	PxShape* aCapsuleShape = PxRigidActorExt::createExclusiveShape(*m_rigidActor,
		PxCapsuleGeometry(m_capsuleRadius, m_capsuleHalfHeight), *m_physXMaterial);
	aCapsuleShape->setLocalPose(relativePose);

	// bind joint with rigid actor
	Joint* bindingJoint = m_skeleton->GetJointByName("pelvis");
	bindingJoint->m_rigidActor = m_rigidActor->is<PxRigidDynamic>();
	bindingJoint->m_relativeDispFromRigidActor = bindingJoint->GetWorldPosition() - GetPhysXToGameSpaceMat().TransformPosition3D(Vec3(m_rigidActor->getGlobalPose().p));

	Quat jointQuat = bindingJoint->GetWorldQuat();
	Quat rigidActorQuat = TransformQuatFromPhysXToGameAndGetInversed(Quat(m_rigidActor->getGlobalPose().q));
	bindingJoint->m_relativeRotationToRigidActor = (rigidActorQuat.GetInversed() * jointQuat).GetNormalized();

	// set the relative matrix for the model and rigid body
	Mat44 jointModelMatrix = bindingJoint->GetWorldTransformMat(); // in game space
	Mat44 actorModelMatrix = GetPhysXToGameSpaceMat().SimilarityTransformation(Mat44(m_rigidActor->getGlobalPose())); // convert PxTransform to Mat44

	// Store the inverse of actorModelMatrix, multiplied by jointModelMatrix
	Mat44 inverse = actorModelMatrix.GetOrthonormalInverse();
	bindingJoint->m_relativeModelMatrixToRigidActor = inverse.MatMultiply(jointModelMatrix);

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// add capsule shape collision for the head part
	// create the capsule collision with joint positions
	Vec3 A_worldPos = m_skeleton->GetJointByName("neck")->GetWorldPosition();
	Vec3 B_worldPos = m_skeleton->GetJointByName("eye")->GetWorldPosition();
	Vec3 jointMiddlePos = (A_worldPos + B_worldPos) * 0.5f;
	float halfHeight = GetDistance3D(jointMiddlePos, B_worldPos) - m_headCollisionRadius;

	PxCapsuleGeometry capsuleGeom(m_headCollisionRadius, halfHeight);

	// align the capsule with the direction of the two joints
	Vec3 dir = (actorCenter - jointMiddlePos);

	// the way to get the rotation in our math library
	PxQuat rotation = GetPxQuat(TransformQuatFromGameToPhysXAndGetInversed(Quat::GetQuatFromTwoVectors(Vec3(0.f, 1.f, 0.f), dir)));

	PxTransform localPose(GetPxVec3(GetGameToPhysXSpaceMat().TransformVectorQuantity3D(jointMiddlePos - actorCenter)), rotation);

	// PxMaterial* crawlerMaterial = g_thePhysX->GetPhysXMaterialByStinrg("crawler");
	PxShape* shape = g_thePhysX->m_physics->createShape(capsuleGeom, *m_physXMaterial);
	shape->setLocalPose(localPose);
	m_rigidActor->attachShape(*shape);

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// add filter data to the collision shape
	PxFilterData filterData;
	filterData.word0 = 2; // Custom collision group for crawler
	filterData.word1 = 1; // Collides with others
	filterData.word2 = 0;
	filterData.word3 = 0;

	aCapsuleShape->setSimulationFilterData(filterData);
	aCapsuleShape->setQueryFilterData(filterData);
	aCapsuleShape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, true);	
	
	shape->setSimulationFilterData(filterData);
	shape->setQueryFilterData(filterData);
	shape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, true);

	// Store a reference to the Actor in PhysX's userData field
	m_rigidActor->userData = this; // Store the crawler pointer
	DebuggerPrintf("Crawler userData set: %p", this);

	// set up mass and inertia and add to the physX scene
	PxRigidDynamic* dynamicActor = m_rigidActor->is<PxRigidDynamic>();
	if (dynamicActor)
	{
		PxRigidBodyExt::setMassAndUpdateInertia(*dynamicActor, (m_totalMass - 4 * (m_upperLegMass + m_lowerLegMass)));

		// lower the gravity center to have more balance
		PxTransform lowerPos(PxVec3(0.0f, -m_capsuleHalfHeight * 0.25f, 0.0f));
		dynamicActor->setCMassLocalPose(lowerPos);

		g_thePhysX->m_scene->addActor(*m_rigidActor);
	}
	else
	{
		ERROR_AND_DIE("Failed to create dynamic rigid actor");
	}

	dynamicActor->wakeUp();

	PxD6Joint* joint = PxD6JointCreate(*g_thePhysX->m_physics, nullptr, PxTransform(PxIdentity), dynamicActor, dynamicActor->getGlobalPose());
	m_skeleton->GetJointByName("root")->m_D6Joint = joint;

	// set free motion
	joint->setMotion(PxD6Axis::eX, PxD6Motion::eFREE); // Free motion along X
	joint->setMotion(PxD6Axis::eY, PxD6Motion::eFREE); // Free motion along Y
	joint->setMotion(PxD6Axis::eZ, PxD6Motion::eFREE); // Free motion along Z

	// // Constrain swing axes (tilting)
	joint->setMotion(PxD6Axis::eSWING1, PxD6Motion::eFREE); // rotate around Z
	joint->setMotion(PxD6Axis::eSWING2, PxD6Motion::eFREE); // tilt back and force
	// joint->setMotion(PxD6Axis::eTWIST, PxD6Motion::eFREE); // Limit side-to-side tilt

	// have a swing limit will cause fall over while moving and rotating
	// // Set swing limits (+/-15 degrees)
	// PxJointLimitCone swingLimit(PxPi / 36.f, PxPi / 36.f);
	// PxJointAngularLimitPair limitPair(-PxPi / 36.f, PxPi / 36.f);
	// joint->setTwistLimit(limitPair);

	// // Allow free rotation around the vertical axis (twist)
	joint->setMotion(PxD6Axis::eTWIST, PxD6Motion::eFREE); // tilt side to side
	// joint->setMotion(PxD6Axis::eSWING1, PxD6Motion::eFREE); // Allow spinning
	// joint->setMotion(PxD6Axis::eSWING2, PxD6Motion::eFREE); // Allow spinning

	// PxD6JointDrive drive(1000.0f, 100.0f, PX_MAX_F32); // Stiffness, damping, force limit
	// joint->setDrive(PxD6Drive::eSWING, drive);    // Stabilize swing motion

	// todo: should I set it every frame, or set it once and update its drive desired position every frame?

	// PxD6JointDrive twistDrive(15.f, 10.f, PX_MAX_F32); // Stiffness, damping, force limit
	// joint->setDrive(PxD6Drive::eTWIST, twistDrive);     // Stabilize twist motion

	PxD6JointDrive swingDrive(10.f, 2.0f, PX_MAX_F32); // Stiffness, damping, force limit
	// PxD6JointDrive swingDrive(3.f, 2.0f, PX_MAX_F32); // Stiffness, damping, force limit
	joint->setDrive(PxD6Drive::eSLERP, swingDrive);       // Stabilize swing motion
}

void Crawler::ApplyPendingForces()
{
	if (g_theInput->WasKeyJustPressed(' ') && !m_isDead)
	{
		PxRigidDynamic* actor = nullptr;

		// int legIndex = g_rng->RollRandomIntInRange(0, 4);
		int legIndex = 4;
		if (legIndex == 4)
		{
			actor = m_rigidActor->is<PxRigidDynamic>();
		}
		else
		{
			actor = m_legs[legIndex]->m_upperLegActor;

			// int partIndex = g_rng->RollRandomIntInRange(0, 1);
			// if (partIndex == 0)
			// {
			// 	actor = m_legs[legIndex]->m_upperLegActor;
			// }
			// else if(m_legs[legIndex]->m_lowerLegActor)
			// {
			// 	actor = m_legs[legIndex]->m_lowerLegActor;
			// }
		}

		if (!actor)
		{
			return;
		}

		Vec3 actorPosInWorld = GetPhysXToGameSpaceMat().TransformPosition3D(Vec3(actor->getGlobalPose().p));
		float dirX = g_rng->RollRandomFloatInRange(-1.f, 1.f);
		float dirY = g_rng->RollRandomFloatInRange(-1.f, 1.f);
		float dirZ = g_rng->RollRandomFloatInRange(-1.f, 1.f);
		Vec3 forceDir = Vec3(dirX, dirY, dirZ).GetNormalized();
		PxVec3 velocityDir = GetPxVec3( GetGameToPhysXSpaceMat().TransformVectorQuantity3D(forceDir) );
		// PxVec3 force = GetPxVec3(GetGameToPhysXSpaceMat().TransformVectorQuantity3D(forceDir)) * m_totalMass * 1.f;

		WasHitByPlayer(actor, actor->getGlobalPose().p, forceDir, 3.3f);
		// PxRigidBodyExt::addForceAtPos(*actor, force, actor->getGlobalPose().p, PxForceMode::eIMPULSE, true);
		DebugAddWorldArrow(actorPosInWorld, actorPosInWorld + forceDir, 0.025f, 1.f, Rgba8::RED);
	}

	m_pendingForceMutex.lock();
	if (!m_pendingForces.empty())
	{
		for (auto force : m_pendingForces)
		{
			// add Force is apply force at the center of mass, which are not going rotate the actor
			// PxRigidDynamic* dynamicActor = force.rigidActor->is<PxRigidDynamic>();
			// dynamicActor->addForce(force.force, force.mode);
			PxRigidBodyExt::addForceAtPos(*(force.rigidActor), force.force, force.forceApplyPosition, PxForceMode::eFORCE, true);

			// we also going to add a little linear velocity to push it off
			float speedMultiplier = 30.f;
			Vec3 dir(force.hitActorVelocityDir);
			dir.y = 0.f;
			dir = dir.GetNormalized();
			Vec3 velocity = speedMultiplier * dir;
			force.rigidActor->setLinearVelocity(GetPxVec3(velocity));
		}

		m_crawlerCurrentFrameState = CrawlerAnimMode::HIT;

		// m_skeleton->m_rootJoint->m_D6Joint->setMotion(PxD6Axis::eSWING2, PxD6Motion::eFREE);

		for (int legIndex = 0; legIndex < m_legs.size(); ++legIndex)
		{
			CrawlerLeg* leg = m_legs[legIndex];

			if (!leg->m_chain->m_timer)
			{
				leg->m_chain->m_timer = new Timer(m_walkingTimer_hit);
			}

			Joint* j = leg->m_chain->GetEndJoint();
			leg->m_animStayPos = j->GetWorldPosition();
			leg->m_legCurrentAnimState = CrawlerAnimMode::HIT;

			if (leg->m_chain->m_timer->IsStopped())
			{
				// Joint* j = leg->m_chain->GetEndJoint();
				leg->m_animStayPos = m_position + m_orientation.GetAsMatrix_XFwd_YLeft_ZUp().TransformVectorQuantity3D(leg->m_footDefaultOffset);
				leg->m_legCurrentAnimState = CrawlerAnimMode::HIT;
			}
		}

		m_pendingForces.clear();

		// for the anim state
		if (m_panicClock)
		{
			m_panicClock->Reset();
		}
		else
		{
			m_panicClock = new Clock();
		}
	}
	m_pendingForceMutex.unlock();
}

void Crawler::WasHitByPlayer(physx::PxRigidDynamic* rigidActor, physx::PxVec3 const& pos, Vec3 const& normal, float hittingSpeed)
{
	if (hittingSpeed <= m_minHitSpeedToDamage || m_isDead)//  || abs(hittingSpeed - m_lastSpeed) < 0.5f)
	{
		return;
	}

	m_lastSpeed = hittingSpeed;

	float speed = GetClamped(hittingSpeed, m_minHitSpeedToDamage, m_maxHitSpeed);
	float speedFraction = GetFractionWithinRange(speed, m_minHitSpeedToDamage, m_maxHitSpeed);
	float damage = m_minDamage + (m_maxDamage - m_minDamage) * speedFraction;
	m_currentHealth -= damage;

	Log::Write(Log::Level::Info, Fmt("Speed: %.02f", speed));
	Log::Write(Log::Level::Info, Fmt("Damage: %.02f", damage));
	Log::Write(Log::Level::Info, Fmt("Health: %.02f", m_currentHealth));

	if (m_currentHealth <= 0.f)
	{
		m_isDead = true;
		m_lastBlowDir = normal;
		return;
	}
	float mass = rigidActor->getMass();
	Vec3 force = GetGameToPhysXSpaceMat().TransformVectorQuantity3D(normal * speed) * mass * 45.f;

	PxVec3 hittingForce = GetPxVec3(force);

	Log::Write(Log::Level::Info, Stringf("Force: %.6f", force.GetLength()));
	 
	// Defer force application
	// we cannot apply force by the onContact event because PhysX processes contacts during the simulation step
	// and modifying an actor's state (like adding force) inside this callback can lead to undefined behavior or be ignored
	m_pendingForces.push_back({rigidActor, pos, hittingForce, normal, PxForceMode::eIMPULSE });
	Log::Write(Log::Level::Info, Fmt("Force: X = %.02f,Y = %.02f, Z = %.02f", hittingForce.x, hittingForce.y, hittingForce.z));
	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// // based on speed fraction, we create a panic timer to recover from hit animation to walking
	// float duration = Interpolate(m_panicDurationForHit_min, m_panicDurationForHit_max, speedFraction);
	// if (!m_squatTimer)
	// {
	// 	m_squatTimer = new Timer(duration);
	// }
	// else
	// {
	// 	m_squatTimer->m_period = duration;
	// 	m_squatTimer->Restart();
	// }
}

void Crawler::WasHitByPlayer(physx::PxRigidDynamic* attackActor, physx::PxRigidDynamic* hitActor, physx::PxVec3 const& pos, Vec3 const& attackActorVelocityDir, float hittingSpeed)
{
	if (hittingSpeed <= m_minHitSpeedToDamage || m_isDead)//  || abs(hittingSpeed - m_lastSpeed) < 0.5f)
	{
		return;
	}

	// check if it is the attack actor is moving not the hit actor run into attack actor
	Vec3 vel(attackActor->getLinearVelocity());
	if (abs(vel.GetLengthSquared() - hittingSpeed * hittingSpeed) > 1.f)
	{
		return;
	}

	// check if it is the player mech hand that hit the crawler
	int index = INT_MAX;
	if (attackActor == g_theVRPlayer->m_skeleton->GetJointByName("lowerArm_r")->m_rigidActor) // || attackActor == g_theVRPlayer->m_skeleton->GetJointByName("upperArm_r")->m_rigidActor)
	{
		index = Side::RIGHT;
	}
	else if (attackActor == g_theVRPlayer->m_skeleton->GetJointByName("lowerArm_l")->m_rigidActor) // || attackActor == g_theVRPlayer->m_skeleton->GetJointByName("upperArm_l")->m_rigidActor)
	{
		index = Side::LEFT;
	}
	else return;

	m_lastSpeed = hittingSpeed;

	lock_guard<std::mutex> lock(m_attackSetMutex);
	if(m_attackRigidActorSet.find(attackActor) != m_attackRigidActorSet.end()) return;
	else m_attackRigidActorSet.insert(attackActor);

	if (m_setClearTimer->IsStopped())
	{
		m_setClearTimer->Start();
	}

	float speed = GetClamped(hittingSpeed, m_minHitSpeedToDamage, m_maxHitSpeed);
	float speedFraction = GetFractionWithinRange(speed, m_minHitSpeedToDamage, m_maxHitSpeed);
	// float speedFraction = GetFractionWithinRange(speed, m_minHitSpeedToDamage, m_maxHitSpeed);

	float damage = m_minDamage + (m_maxDamage - m_minDamage) * speedFraction;
	m_currentHealth -= damage;

	Log::Write(Log::Level::Info, Fmt("Speed: %.02f", speed));
	Log::Write(Log::Level::Info, Fmt("Damage: %.02f", damage));
	Log::Write(Log::Level::Info, Fmt("Health: %.02f", m_currentHealth));
	 
	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// trigger vibration for the controller
	if (index == Side::RIGHT || index == Side::LEFT)
	{
		float vibrationDuration = Interpolate(0.3f, 1.5f, speedFraction);
		float amplitude = Interpolate(0.45f, 0.8f, speedFraction);
		g_theVRPlayer->m_hands[index].StartControllerVibration(vibrationDuration, amplitude);
	}
	 
	if (m_currentHealth <= 0.f)
	{
		m_isDead = true;
		m_lastBlowDir = attackActorVelocityDir;
		return;
	}
	float mass = hitActor->getMass();
	PxVec3 hittingForce = GetPxVec3(GetGameToPhysXSpaceMat().TransformVectorQuantity3D(attackActorVelocityDir * speed) * mass * 8.f);

	// Log::Write(Log::Level::Info, Stringf("Speed: %.2f", hittingSpeed));

	// Defer force application
	// we cannot apply force by the onContact event because PhysX processes contacts during the simulation step
	// and modifying an actor's state (like adding force) inside this callback can lead to undefined behavior or be ignored
	m_pendingForceMutex.lock();
	m_pendingForces.push_back({ hitActor, pos, hittingForce, attackActorVelocityDir, PxForceMode::eIMPULSE });
	m_pendingForceMutex.unlock();
	Log::Write(Log::Level::Info, Fmt("Force: X = %.02f,Y = %.02f, Z = %.02f", hittingForce.x, hittingForce.y, hittingForce.z));
}

void Crawler::MarkHitCrawlerLeg(physx::PxRigidDynamic* hitActor)
{
	for (int legIndex = 0; legIndex < m_legs.size(); ++legIndex)
	{
		CrawlerLeg* leg = m_legs[legIndex];
		if (hitActor == leg->m_upperLegActor || hitActor == leg->m_lowerLegActor)
		{
			leg->m_wasHit = true;
		}
	}
}

void Crawler::UnmarkHitCrawlerLeg()
{
	for (int legIndex = 0; legIndex < m_legs.size(); ++legIndex)
	{
		m_legs[legIndex]->m_wasHit = false;
	}
}

void Crawler::PlayDeathPhysicalAnimation()
{
	if (m_isDead && !m_velocityApplied)
	{
		m_skeleton->BreakAllD6Joints();
		m_crawlerCurrentFrameState = CrawlerAnimMode::DEAD;
		ApplyLinearVelocityToAllRigidBodies();
		DisableCollisionWithPlayerMech();
		m_deathClearTimer->Start();
	}
}

void Crawler::DisableCollisionWithPlayerMech()
{
	// find every rigid body in the skeleton and change its filter data
	for (auto const& pair : m_skeleton->m_allJoints)
	{
		Joint* joint = pair.second;
		if (joint->m_rigidActor)
		{
			PxRigidDynamic* actor = joint->m_rigidActor->is<PxRigidDynamic>();

			// Get the number of shapes (a single actor can have multiple shapes)
			PxShape* shapes[2];
			uint32_t shapeCount = actor->getNbShapes();
			if (shapeCount == 2)
			{
				int i = 0;
				++i;
			}
			actor->getShapes(shapes, shapeCount);

			// Define new filter data
			PxFilterData newFilterData;
			newFilterData.word0 = 1; // change to player mech collision group
			newFilterData.word1 = 3; // collide with no one
			newFilterData.word2 = 0; 
			newFilterData.word3 = 0; 

			// Update all shapes collision filter
			for (uint32_t i = 0; i < shapeCount; ++i)
			{
				shapes[i]->setSimulationFilterData(newFilterData); 
				shapes[i]->setQueryFilterData(newFilterData); 
			}
		}
	}
}

void Crawler::ApplyLinearVelocityToAllRigidBodies()
{
	// find every rigid body in the skeleton and add random force	
	for (auto const& pair : m_skeleton->m_allJoints) 
	{
		Joint* joint = pair.second;
		if (joint->m_rigidActor)
		{
			Vec3 dir = g_rng->GetRandomDirectionInCone(m_lastBlowDir, m_deflectionAngle);
			// float mass = joint->m_rigidActor->getMass();
			// PxVec3 pos = joint->m_rigidActor->getGlobalPose().p;
			// Vec3 forceInGame = g_rng->RollRandomFloatInRange(m_blowAcceration_min, m_blowAcceration_max) * dir * mass;
			// Vec3 forceInGame =  dir * 0.000000001f;
			// PxVec3 hittingForce = GetPxVec3(GetGameToPhysXSpaceMat().TransformVectorQuantity3D(forceInGame));
			// 
			// Log::Write(Log::Level::Info, Stringf("Force: %.10f", forceInGame.GetLength()));
			// m_pendingForces.push_back({ joint->m_rigidActor, pos, hittingForce, PxForceMode::eIMPULSE });

			Vec3 velocity = g_rng->RollRandomFloatInRange(m_blowAcceration_min, m_blowAcceration_max) * dir;
			PxVec3 velocityInPhysX = GetPxVec3( GetGameToPhysXSpaceMat().TransformVectorQuantity3D(velocity) );
			joint->m_rigidActor->setLinearVelocity(velocityInPhysX);
		}
	}

	m_velocityApplied = true;
	m_skeleton->m_renderByRigidActor = true;
}

void Crawler::Update()
{
	// if (!m_poseAdjusted)
	// {
	// 	AdjustInitialPose();
	// 	UpdateProceduralAnimationByCurrentState();
	// }
	// else
	// {
	// }

	if (!m_isDead)
	{
		ApplyPendingForces();
		if (!m_isDead)
		{
			UpdateInput();
			UpdateActorInfoByRigidBody();

			UpdateAnimState();

			UpdateToeGroundedStatus();
			UpdateIfLegShouldMove();
			UpdateProceduralAnimationByCurrentState();

			m_crawlerLastFrameState = m_crawlerCurrentFrameState;

			// if (m_crawlerLastFrameState == CrawlerAnimMode::ADJUSTING && m_crawlerCurrentFrameState == CrawlerAnimMode::WALKING)
			// {
			// 	int i = 0;
			// 	++i;
			// }
		}
	}
	else
	{
		PlayDeathPhysicalAnimation();
		m_crawlerCurrentFrameState = CrawlerAnimMode::DEAD;
		UpdateProceduralAnimationByCurrentState();
	}

	for (int legIndex = 0; legIndex < m_legs.size(); ++legIndex)
	{
		CrawlerLeg* leg = m_legs[legIndex];
		leg->m_legLastFrameAnimState = leg->m_legCurrentAnimState;
	}

	if (m_setClearTimer->HasPeroidElapsed())
	{
		m_attackRigidActorSet.clear();
	}
}

void Crawler::UpdateInput()
{
	//if (m_WinCameraIsInControl)
	//{
	//	ControlActorByInput();
	//	g_theInput->SetCursorMode(true, true);
	//}
	//else
	//{
	//	g_theInput->SetCursorMode(false, false);
	//}

	Vec3 forceDir = Vec3::ZERO;
	Mat44 rotationMat = m_orientation.GetAsMatrix_XFwd_YLeft_ZUp();
	// float deltaSeconds = Clock::GetSystemClock().GetDeltaSeconds();

	if (g_theInput->WasKeyJustPressed('K'))
	{
		m_isDead = true;
	}

	// if (g_theInput->IsKeyDown(KEYCODE_UPARROW))
	// {
	// 	forceDir += rotationMat.TransformVectorQuantity3D(Vec3(1.f, 0.f, 0.f));
	// }
	// if (g_theInput->IsKeyDown(KEYCODE_DOWNARROW))
	// {
	// 	forceDir += rotationMat.TransformVectorQuantity3D(Vec3(-1.f, 0.f, 0.f));
	// }
	// if (g_theInput->IsKeyDown(KEYCODE_LEFTARROW))
	// {
	// 	forceDir += rotationMat.TransformVectorQuantity3D(Vec3(0.f, 1.f, 0.f));
	// }
	// if (g_theInput->IsKeyDown(KEYCODE_RIGHTARROW))
	// {
	// 	forceDir += rotationMat.TransformVectorQuantity3D(Vec3(0.f, -1.f, 0.f));
	// }
	// forceDir = forceDir.GetNormalized();

	// based on timer, update the crawler's aiming direction
	if (m_directionChangingTimer->HasPeroidElapsed())
	{
		// crawler update its remembered player position by the timer, not every frame
		// which will cause the problem that the crawler moves every time the players move
		RollRandomDesiredPositionInFrontOfPlayer();
	}
	Vec3 desiredPos = m_rememberedPlayerDir.GetForwardIBasis() * m_closestDistToPlayer + m_rememberedPlayerPos;

	// string debugStr = Stringf("pos: %.2f, %.2f, %.2f", desiredPos.x, desiredPos.y, desiredPos.z);
	// DebugAddScreenText(debugStr, Vec2(), 20.f, Vec2(0.5f, 0.5f), -1.f, Rgba8::WHITE);
	DebugAddWorldWireSphere(desiredPos, 0.05f, 0.5f, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::X_RAY);

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// rotating
	// when crawler is hit, no need to control the rotation while hit flying
	if (m_crawlerCurrentFrameState == CrawlerAnimMode::HIT)
	{
		return;
	}
	float dotProduct = 0.f;
	float rotatingBoundry = cosf(ConvertDegreesToRadians(m_legRotationAngleBoundary + 5.f));	// 2.f is because the drive cannot perfectly rotate the actor

	float distToDesiredPos = GetDistance3D(m_position, desiredPos);

	// get desired rotation 
	PxRigidDynamic* dynamicActor = m_rigidActor->is<PxRigidDynamic>();
	PxTransform poseInPhysX = dynamicActor->getGlobalPose();
	Quat crtQuat = Quat(poseInPhysX.q);

	float desiredYaw = 0.f;
	// float distToPlayer = GetDistance3D(m_position, m_rememberedPlayerPos);
	if (distToDesiredPos > 0.3f)	// when crawler is far from player, aim for desired position
	{
		desiredYaw = (desiredPos - m_position).GetYawDegrees();
		dotProduct = DotProduct3D(m_orientation.GetForwardIBasis(), (desiredPos - m_position).GetNormalized());
	}
	else // when crawler close to player, aim for the player
	{
		desiredYaw = (g_theVRPlayer->m_position - m_position).GetYawDegrees();
		dotProduct = DotProduct3D(m_orientation.GetForwardIBasis(), (m_rememberedPlayerPos - desiredPos).GetNormalized());
	}
	
	m_desiredOrientation = m_orientation;
	m_desiredOrientation.m_yawDegrees = desiredYaw;
	m_desiredOrientation.m_pitchDegrees = 0.f;
	m_desiredOrientation.m_rollDegrees = 0.f;
	Quat desiredJointOrientation = ConvertEulerAnglesToQuat(m_desiredOrientation);

	// get two joints world rotation
	Quat actor0_worldOrientation;	// the actor0 does not exist, so it is default Quat()
	PxD6Joint* D6Joint = m_skeleton->GetJointByName("root")->m_D6Joint;
	Quat D6Joint_localOrientation = TransformQuatFromPhysXToGameAndGetInversed(Quat(D6Joint->getLocalPose(PxJointActorIndex::eACTOR0).q));
	Quat D6JointWorldRotation = (actor0_worldOrientation * D6Joint_localOrientation).GetNormalized();

	// calculate the local rotation the D6 joint needs to make
	Quat driveRotation = (D6JointWorldRotation.GetInversed() * desiredJointOrientation).GetNormalized();

	// change the stiffness of the drive based on the difference of the rotation we need to make
	float stiffness = 45.f;
	PxD6JointDrive swingDrive(stiffness, 2.0f, PX_MAX_F32); // Stiffness, damping, force limit
	D6Joint->setDrive(PxD6Drive::eSLERP, swingDrive);       // Stabilize swing motions

	if (dotProduct < rotatingBoundry) // && (distToDesiredPos >= 0.3f))//  || distToPlayer > m_distToPlayerMax))	// only rotate body when the crawler is close to the desired pos
	{
		m_crawlerCurrentFrameState = CrawlerAnimMode::ROTATING;

		// change the stiffness of the drive based on the difference of the rotation we need to make
		stiffness = RangeMapClamped(desiredYaw - m_orientation.m_yawDegrees, 0.f, 180.f, 9.f, 12.f);
		PxD6JointDrive SwingDrive(stiffness, 2.0f, PX_MAX_F32); // Stiffness, damping, force limit
		D6Joint->setDrive(PxD6Drive::eSLERP, SwingDrive);       // Stabilize swing motions

		PxRigidActor* actor0 = nullptr;		// the rigid actor that the D6 joint is on
		PxRigidActor* actor1 = nullptr;		// the rigid actor that the D6 joint drive controls
		D6Joint->getActors(actor0, actor1);
		if (!D6Joint)
		{
			ERROR_AND_DIE("This skeleton joint does not have a D6 joint!");
		}

		m_desiredRootD6JointTransform.q = GetPxQuat(TransformQuatFromGameToPhysXAndGetInversed(driveRotation));
		m_desiredRootD6JointTransform.p = D6Joint->getDrivePosition().p;
		D6Joint->setDrivePosition(m_desiredRootD6JointTransform);

		m_isDriving = false;
	}
	else
	{
		if (m_crawlerCurrentFrameState != CrawlerAnimMode::IDLE)
		{
			m_crawlerCurrentFrameState = CrawlerAnimMode::WALKING;
		}

		// do not change the D6 joint's location but update its orientation
		m_desiredRootD6JointTransform.q = GetPxQuat(TransformQuatFromGameToPhysXAndGetInversed(driveRotation));
		m_desiredRootD6JointTransform.p = D6Joint->getDrivePosition().p;
		D6Joint->setDrivePosition(m_desiredRootD6JointTransform);
	}

	if (m_crawlerCurrentFrameState == CrawlerAnimMode::ROTATING) // && CheckIfLegShouldRotate())
	{
		return;
	}
	else
	{
		// apply force only when the crawler has not get the position and does not need rotation
		float acceleration = RangeMapClamped(distToDesiredPos, 0.f, 1.f, m_forceAcceleration * 0.f, m_forceAcceleration);	// when crawler get to distance, make the acceleration smaller
		float dist = (desiredPos - m_position).GetLength();

		if (m_speed < m_maxMovingSpeed && acceleration != 0.f && dist > 0.3f)
		{
			forceDir = (desiredPos - m_position).GetNormalized();
			Vec3 forceDirInPhysx = GetGameToPhysXSpaceMat().TransformVectorQuantity3D(forceDir);
			PxVec3 force = GetPxVec3(forceDirInPhysx * m_totalMass * acceleration);
			dynamicActor->addForce(force, PxForceMode::eFORCE, true);
			m_isDriving = true;
		}
		else
		{
			m_isDriving = false;
			if (m_crawlerCurrentFrameState != CrawlerAnimMode::IDLE)
			{
				m_crawlerCurrentFrameState = CrawlerAnimMode::ADJUSTING;
			}
		}

		// string debugStr = Stringf("disp: %.2f, %.2f, %.2f", forceDir.x, forceDir.y, forceDir.z);
		// DebugAddScreenText(debugStr, Vec2(), 20.f, Vec2(0.5f, 0.5f), -1.f, Rgba8::WHITE);

		// string debugStr = Stringf("angle: %.2f", m_playerAngleOffset);
		// DebugAddScreenText(debugStr, Vec2(), 20.f, Vec2(0.5f, 0.5f), -1.f, Rgba8::WHITE);
	}
}

void Crawler::RollRandomDesiredPositionInFrontOfPlayer()
{
	// change the timer period in case multiple crawlers are changing the target at the same time
	m_directionChangingTimer->m_period = g_rng->RollRandomFloatInFloatRange(FloatRange(m_directionChangingPeroidMin, m_directionChangingPeroidMax));

	m_playerAngleOffset = g_rng->RollRandomFloatInFloatRange(FloatRange(-m_playerAngleRange, m_playerAngleRange));
	m_closestDistToPlayer = g_rng->RollRandomFloatInFloatRange(FloatRange(m_distToPlayerMin, m_distToPlayerMax));
	m_directionChangingTimer->Restart();

	m_rememberedPlayerDir = g_theVRPlayer->m_orientation + EulerAngles(m_playerAngleOffset, 0.f, 0.f);
	m_rememberedPlayerPos = g_theVRPlayer->m_position;
}

void Crawler::UpdateActorInfoByRigidBody()
{
	// Retrieve the updated global pose of the capsule actor
	PxTransform capsuleGlobalPose = m_rigidActor->getGlobalPose();

	// Extract the position and orientation
	PxVec3 capsulePos = capsuleGlobalPose.p;
	PxQuat capsuleCollision = capsuleGlobalPose.q;

	// update position and orientation
	Mat44 quatMat = GetPhysXToGameSpaceMat().SimilarityTransformation(Mat44(Quat(capsuleCollision)));
	Vec3 actorZUp = quatMat.GetKBasis3D();
	Vec3 pos = GetPhysXToGameSpaceMat().TransformPosition3D(Vec3(capsulePos)) + (m_capsuleHalfHeight + m_capsuleRadius) * (actorZUp * -1.f);

	// std::string debugTargetPosition = Stringf("%.2f, %.2f, %.2f", Vec3(capsulePos).x, Vec3(capsulePos).y, Vec3(capsulePos).z);
	// DebugAddScreenText(debugTargetPosition, Vec2(), 20.f, Vec2(0.5f, 0.5f), -1.f);

	m_position = pos;
	m_orientation = quatMat.GetEulerAngles();

	// update actor velocity info
	PxRigidDynamic* dynamicActor = m_rigidActor->is<PxRigidDynamic>();
	PxVec3 velocity = dynamicActor->getLinearVelocity();
	m_velocity = GetPhysXToGameSpaceMat().TransformVectorQuantity3D(Vec3(velocity));
	
	m_speedLastFrame = m_speed;
	m_speed = m_velocity.GetLength();
	m_velocityDir = m_velocity.GetNormalized();


	UpdateActorDamping();
}

void Crawler::UpdateActorDamping()
{
	// if the crawler is knock off by the player by a great speed
	// set the damping to slow it down
	PxRigidDynamic* dynamicActor = m_rigidActor->is<PxRigidDynamic>();
	PxVec3 angularVelocity = dynamicActor->getAngularVelocity();
	Vec3 angularVel(angularVelocity);

	if (m_speed >= m_brakeTriggerSpeed)
	{
		dynamicActor->setLinearDamping(m_mechBrakeLinearDampingValue);
	}
	else if (m_crawlerCurrentFrameState == CrawlerAnimMode::ROTATING || m_crawlerCurrentFrameState == CrawlerAnimMode::ADJUSTING)
	{
		dynamicActor->setLinearDamping(m_mechBrakeLinearDampingValue);
	}
	else if (m_crawlerCurrentFrameState == CrawlerAnimMode::HIT)
	{
		dynamicActor->setLinearDamping(m_mechHitLinearDampingValue);
	}
	else
	{
		dynamicActor->setLinearDamping(0.f);
	}

	// limit rigid body's max angular velocity
	if (angularVel.GetLength() >= m_mechMaxRotationVelocity && m_crawlerCurrentFrameState != CrawlerAnimMode::HIT)
	{
		dynamicActor->setAngularDamping(m_mechRotatingAdjustingDampingValue);
	}	
	else if (angularVel.GetLength() >= m_mechMaxRotationVelocity && m_crawlerCurrentFrameState != CrawlerAnimMode::HIT)
	{
		dynamicActor->setAngularDamping(m_mechRotatingAdjustingDampingValue);
	}
	else
	{
		dynamicActor->setAngularDamping(0.f);
	}
}

void Crawler::Render() const
{
	if (m_poseAdjusted)
	{
	}
	if (m_skeleton)
	{
		m_skeleton->Render();
	}

	if (m_healthBar && m_currentHealth > 0.1f)
	{
		m_healthBar->Render();
	}
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

void Crawler::AdjustInitialPose()
{
	m_crawlerCurrentFrameState = CrawlerAnimMode::ROTATING;
	m_orientation = m_initialAngle;
	if (!CheckIfLegShouldRotate())
	{
		m_poseAdjusted = true;
	}
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

	// float fontSize = 24.f;

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
}

