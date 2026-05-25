#include "Game/VRPlayer.hpp"
#include "Game/Game.hpp"
#include "Game/WinPlayer.hpp"
#include "Game/openxr_program.h"
#include "Engine/Math/OpenXRMathUtils.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/DebugRender.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Physics/ThePhysX.hpp"
#include "Engine/Math/PhysXMathUtils.hpp"
#include "Engine/Physics/ThePhysX.hpp"
#include "Engine/core/Clock.hpp"
#include "Engine/core/Timer.hpp"
#include "ThirdParty/Physx/include/foundation/PxMath.h"
#include "ThirdParty/Physx/include/PxPhysicsAPI.h"

extern Renderer* g_theRenderer;
extern InputSystem* g_theInput;
extern Game* g_theGame;
extern WinPlayer* g_theWinPlayer;
extern ThePhysX* g_thePhysX;
extern OpenXrProgram* g_theApp;
extern Clock* g_theGameClock;

using namespace std;
using namespace physx;

VRPlayer::VRPlayer(Vec3 const& pos /*= Vec3()*/)
	: Actor(pos)
{
	m_hands[Side::LEFT].m_handIndex = Side::LEFT;
	m_hands[Side::RIGHT].m_handIndex = Side::RIGHT;
}

VRPlayer::~VRPlayer()
{
	if (m_cockpitModel) {
		delete m_cockpitModel;
		m_cockpitModel = nullptr;
	}
	// if (m_screenModel) {
	//     delete m_screenModel;
	//     m_screenModel = nullptr;
	// }

	if (m_shoulderModel_r) {
		delete m_shoulderModel_r;
		m_shoulderModel_r = nullptr;
	}
	if (m_shoulderModel_l) {
		delete m_shoulderModel_l;
		m_shoulderModel_l = nullptr;
	}

	if (m_upperArm_Connector_r) {
		delete m_upperArm_Connector_r;
		m_upperArm_Connector_r = nullptr;
	}
	if (m_upperArm_Gear_r) {
		delete m_upperArm_Gear_r;
		m_upperArm_Gear_r = nullptr;
	}
	if (m_lowerArm_Gear_r) {
		delete m_lowerArm_Gear_r;
		m_lowerArm_Gear_r = nullptr;
	}
	if (m_lowerArm_Connector_r) {
		delete m_lowerArm_Connector_r;
		m_lowerArm_Connector_r = nullptr;
	}

	if (m_maceModel) {
		delete m_maceModel;
		m_maceModel = nullptr;
	}

	if (m_upperArm_Connector_l) {
		delete m_upperArm_Connector_l;
		m_upperArm_Connector_l = nullptr;
	}
	if (m_upperArm_Gear_l) {
		delete m_upperArm_Gear_l;
		m_upperArm_Gear_l = nullptr;
	}
	if (m_lowerArm_Gear_l) {
		delete m_lowerArm_Gear_l;
		m_lowerArm_Gear_l = nullptr;
	}
	if (m_lowerArm_Connector_l) {
		delete m_lowerArm_Connector_l;
		m_lowerArm_Connector_l = nullptr;
	}

	if (m_skeleton)
	{
		delete m_skeleton;
	}
}

void VRPlayer::Startup()
{
	// create skeleton
	Actor* actor = static_cast<Actor*>(this);
	m_skeleton = new Skeleton("PlayerMechSkeleton", actor);

	// m_skeleton->m_worldPos = Vec3();

	// create all joints for player mech
	Quat quat_0;
	Quat quat_15 = Quat::CreateRotationAroundZAxis(15.f);
	Quat quat_30 = Quat::CreateRotationAroundZAxis(30.f);
	Quat quat_45 = Quat::CreateRotationAroundZAxis(45.f);
	Quat quat_90 = Quat::CreateRotationAroundZAxis(90.f);
	Quat quat_N90 = Quat::CreateRotationAroundZAxis(-90.f);
	Quat quat_Y_N90 = Quat::CreateRotationAroundYAxis(-90.f);

	Joint* rootJoint = m_skeleton->AddJoint("root", Vec3(), quat_Y_N90);
	Joint* pelvisJoint = m_skeleton->AddJoint("pelvis", Vec3(1.5f, 0.f, 0.f), Quat(), rootJoint);
	Joint* clavicle_r = m_skeleton->AddJoint("clavicle_r", Vec3(0.75f, -0.45f, 0.f), ConvertEulerAnglesToQuat(EulerAngles(-90.f, 0.f, -90.f)), pelvisJoint);
	Joint* clavicle_l = m_skeleton->AddJoint("clavicle_l", Vec3(0.75f, 0.45f, 0.f), ConvertEulerAnglesToQuat(EulerAngles(90.f, 0.f, 90.f)), pelvisJoint);

	Joint* upperArm_r = m_skeleton->AddJoint("upperArm_r", Vec3(m_shoulderLen, 0.f, 0.f), quat_0, clavicle_r);
	Joint* lowerArm_r = m_skeleton->AddJoint("lowerArm_r", Vec3(m_upperArmLen, 0.f, 0.f), quat_0, upperArm_r);
	Joint* hand_r = m_skeleton->AddJoint("hand_r", Vec3(m_foreArmLen, 0.f, 0.f), quat_0, lowerArm_r);

	Joint* upperArm_l = m_skeleton->AddJoint("upperArm_l", Vec3(m_shoulderLen, 0.f, 0.f), quat_0, clavicle_l);
	Joint* lowerArm_l = m_skeleton->AddJoint("lowerArm_l", Vec3(m_upperArmLen, 0.f, 0.f), quat_0, upperArm_l);
	Joint* hand_l = m_skeleton->AddJoint("hand_l", Vec3(m_foreArmLen, 0.f, 0.f), quat_0, lowerArm_l);

	// for now removing the constraint have much better final visual result, we'll deal with this in the final version
	// upperArm_r->SetLocalRotationConstraint(EulerAngles(-30.f, -60.f, -60.f), EulerAngles(150.f, 90.f, 60.f));			// both min and max equal to 0 means not able to move
	// upperArm_l->SetLocalRotationConstraint(EulerAngles(-150.f, -60.f, -60.f), EulerAngles(30.f, 90.f, 60.f));			// both min and max equal to 0 means not able to move
	
	// lowerArm_r->SetLocalRotationConstraint(EulerAngles(-180.f, -180.f, -90.f), EulerAngles(180.f, 180.f, 90.f));			// min is -180, max is 180 means it is free to move all degrees
	// lowerArm_r->SetLocalRotationConstraint(EulerAngles(-90.f, -80.f, 0.f), EulerAngles(90.f, 0.f, 0.f));				// min is -180, max is 180 means it is free to move all degrees

	m_skeleton->Startup();

	// create two chains
	TwoJointsIKSolver* m_ikSolver_arm_r = new TwoJointsIKSolver(upperArm_r, lowerArm_r, hand_r);
	m_ikSolver_arm_r->m_usePoleVector = true;
	m_arm_r = new Chain(m_ikSolver_arm_r);

	TwoJointsIKSolver* m_ikSolver_arm_l = new TwoJointsIKSolver(upperArm_l, lowerArm_l, hand_l);
	m_ikSolver_arm_l->m_usePoleVector = true;
	m_arm_l = new Chain(m_ikSolver_arm_l);


	//----------------------------------------------------------------------------------------------------------------------------------------------------

	CreateDynamicRigidBodyForMech();
	CreateJointToConstrainMechBodyMovement();

	m_rotationResetTimer = new Timer(m_rotationResetDuration);

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// generate D6 joint for arms
	// right arm
	PxRigidDynamic* upperArmDynamicActor_r = CreateDynamicActorForJoint("lowerArm_r");
	upperArm_r->m_rigidActor = upperArmDynamicActor_r;
	PxRigidDynamic* lowerArmDynamicActor_r = CreateDynamicActorForJoint("hand_r");
	lowerArm_r->m_rigidActor = lowerArmDynamicActor_r;
	
	float lowerArmBoneLength = lowerArm_r->GetBoneLength();
	float handBoneLength = hand_r->GetBoneLength();
	
	PxTransform pxTransformLocal_0(GetPxVec3(Vec3(lowerArmBoneLength * 0.5f, 0.f, 0.f)), GetPxQuat(Quat()));
	PxTransform pxTransformLocal_1(GetPxVec3(Vec3(-handBoneLength * 0.5f, 0.f, 0.f)), GetPxQuat(Quat()));
	
	// connect upper arm and lower arm with a D6 joint
	PxD6Joint* lowerArm_r_D6_joint = PxD6JointCreate(*g_thePhysX->m_physics, upperArmDynamicActor_r, pxTransformLocal_0, lowerArmDynamicActor_r, pxTransformLocal_1);
	lowerArm_r->m_D6Joint = lowerArm_r_D6_joint;
	
	lowerArm_r_D6_joint->setMotion(PxD6Axis::eSWING1, PxD6Motion::eFREE);
	lowerArm_r_D6_joint->setMotion(PxD6Axis::eSWING2, PxD6Motion::eFREE);
	lowerArm_r_D6_joint->setMotion(PxD6Axis::eTWIST, PxD6Motion::eFREE);
	lowerArm_r_D6_joint->setDrive(PxD6Drive::eSLERP, PxD6JointDrive(5100.0f, 750.0f, FLT_MAX, true));
	
	PxRigidDynamic* bodyDynamicActor = m_rigidActor->is<PxRigidDynamic>();
	Vec3 bodyActorCenterPos = m_position + Vec3(0.f, 0.f, m_capsuleHalfHeight + m_capsuleRadius);

	// connect upper arm and body with a D6 joint
	Vec3 upperArm_r_disp = upperArm_r->GetWorldPosition() - bodyActorCenterPos;
	PxTransform pxTransformLocal_2(GetPxVec3(Vec3(-upperArm_r_disp.y, upperArm_r_disp.z, 0.f)), GetPxQuat(Quat()));
	PxTransform pxTransformLocal_3(GetPxVec3(Vec3(-lowerArmBoneLength * 0.5f, 0.f, 0.f)), GetPxQuat(Quat()));
	
	PxD6Joint* upperArm_r_D6_joint = PxD6JointCreate(*g_thePhysX->m_physics, bodyDynamicActor, pxTransformLocal_2, upperArmDynamicActor_r, pxTransformLocal_3);
	upperArm_r->m_D6Joint = upperArm_r_D6_joint;
	
	upperArm_r_D6_joint->setMotion(PxD6Axis::eSWING1, PxD6Motion::eFREE);
	upperArm_r_D6_joint->setMotion(PxD6Axis::eSWING2, PxD6Motion::eFREE);
	upperArm_r_D6_joint->setMotion(PxD6Axis::eTWIST, PxD6Motion::eFREE);

	// high damping controls whether its "shaking" when it stops, stiffness controls the speed of arm following
	upperArm_r_D6_joint->setDrive(PxD6Drive::eSLERP, PxD6JointDrive(4500.0f, 600.0f, FLT_MAX, true));	

	lowerArmDynamicActor_r->setActorFlag(PxActorFlag::eVISUALIZATION, true);
	upperArmDynamicActor_r->setActorFlag(PxActorFlag::eVISUALIZATION, true);

	// Disable collisions between the two actors connected by this joint
	// clavicle_D6_joint->setConstraintFlag(PxConstraintFlag::eDISABLE_COLLISION, true);

	// left arm
	PxRigidDynamic* upperArmDynamicActor_l = CreateDynamicActorForJoint("lowerArm_l");
	upperArm_l->m_rigidActor = upperArmDynamicActor_l;
	PxRigidDynamic* lowerArmDynamicActor_l = CreateDynamicActorForJoint("hand_l");
	lowerArm_l->m_rigidActor = lowerArmDynamicActor_l;

	PxTransform pxTransformLocal_4(GetPxVec3(Vec3(-lowerArmBoneLength * 0.5f, 0.f, 0.f)), GetPxQuat(Quat()));
	PxTransform pxTransformLocal_5(GetPxVec3(Vec3(handBoneLength * 0.5f, 0.f, 0.f)), GetPxQuat(Quat()));

	// connect upper arm with lower arm
	PxD6Joint* lowerArm_l_D6_joint = PxD6JointCreate(*g_thePhysX->m_physics, upperArmDynamicActor_l, pxTransformLocal_4, lowerArmDynamicActor_l, pxTransformLocal_5);
	lowerArm_l->m_D6Joint = lowerArm_l_D6_joint;

	lowerArm_l_D6_joint->setMotion(PxD6Axis::eSWING1, PxD6Motion::eFREE);
	lowerArm_l_D6_joint->setMotion(PxD6Axis::eSWING2, PxD6Motion::eFREE);
	lowerArm_l_D6_joint->setMotion(PxD6Axis::eTWIST, PxD6Motion::eFREE);
	lowerArm_l_D6_joint->setDrive(PxD6Drive::eSLERP, PxD6JointDrive(5100.0f, 750.0f, FLT_MAX, true));

	// connect upper arm and body with a D6 joint
	Vec3 upperArm_l_disp = upperArm_l->GetWorldPosition() - bodyActorCenterPos;
	PxTransform pxTransformLocal_6(GetPxVec3(Vec3(-upperArm_l_disp.y, upperArm_r_disp.z, 0.f)), GetPxQuat(Quat()));
	PxTransform pxTransformLocal_7(GetPxVec3(Vec3(lowerArmBoneLength * 0.5f, 0.f, 0.f)), GetPxQuat(Quat()));

	PxD6Joint* upperArm_l_D6_joint = PxD6JointCreate(*g_thePhysX->m_physics, bodyDynamicActor, pxTransformLocal_6, upperArmDynamicActor_l, pxTransformLocal_7);
	upperArm_l->m_D6Joint = upperArm_l_D6_joint;

	upperArm_l_D6_joint->setMotion(PxD6Axis::eSWING1, PxD6Motion::eFREE);
	upperArm_l_D6_joint->setMotion(PxD6Axis::eSWING2, PxD6Motion::eFREE);
	upperArm_l_D6_joint->setMotion(PxD6Axis::eTWIST, PxD6Motion::eFREE);
	upperArm_l_D6_joint->setDrive(PxD6Drive::eSLERP, PxD6JointDrive(4500.0f, 600.0f, FLT_MAX, true));	// high damping controls whether its "shaking" when it stops, stiffness constrols the speed of arm following

	lowerArmDynamicActor_l->setActorFlag(PxActorFlag::eVISUALIZATION, true);
	upperArmDynamicActor_l->setActorFlag(PxActorFlag::eVISUALIZATION, true);

	// add hand collision for the lower arm dynamic actor
	// create dynamic rigid actor and bind it with sphere collision
	PxSphereGeometry sphereGeom(m_handSphereCollisionRadius);
	m_handMaterial = g_thePhysX->CreatePhysXMaterial("hand", m_handStaticFriction, m_handDynamicFriction, m_handRestitution);

	PxShape* capsuleShape_r = PxRigidActorExt::createExclusiveShape(*lowerArmDynamicActor_r, sphereGeom, *m_handMaterial);
	PxShape* capsuleShape_l = PxRigidActorExt::createExclusiveShape(*lowerArmDynamicActor_l, sphereGeom, *m_handMaterial);
	PxTransform sphereOffset_r(PxVec3(m_foreArmLen * 0.5f, 0.f, 0.f));
	PxTransform sphereOffset_l(PxVec3(-m_foreArmLen * 0.5f, 0.f, 0.f));
	capsuleShape_r->setLocalPose(sphereOffset_r);
	capsuleShape_l->setLocalPose(sphereOffset_l);

	PxFilterData filterData;
	filterData.word0 = 1; // Custom collision group the shape belongs to. Prevent collisions with parts of the VR player (same group)
	filterData.word1 = 2; // Collide with crawler parts
	filterData.word2 = 0; // more advanced filtering
	filterData.word3 = 0; // more advanced filtering

	capsuleShape_r->setSimulationFilterData(filterData);
	capsuleShape_r->setQueryFilterData(filterData);
	capsuleShape_r->setFlag(PxShapeFlag::eSIMULATION_SHAPE, true);
	
	capsuleShape_l->setSimulationFilterData(filterData);
	capsuleShape_l->setQueryFilterData(filterData);
	capsuleShape_l->setFlag(PxShapeFlag::eSIMULATION_SHAPE, true);

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// load models
	m_cockpitModel = new ObjModel();
	m_cockpitModel->LoadXml("Data/Models/Player_Cockpit.xml");
	m_cockpitModel->m_color = Rgba8::WHITE;
	pelvisJoint->m_objModels.push_back(m_cockpitModel);

	// m_screenModel = new ObjModel();
	// m_screenModel->LoadXml("Data/Models/Player_Screen.xml");
	// m_screenModel->m_color = Rgba8::WHITE;
	// pelvisJoint->m_objModels.push_back(m_screenModel);

	m_shoulderModel_r = new ObjModel();
	m_shoulderModel_r->LoadXml("Data/Models/Player_Shoulder_r.xml");
	m_shoulderModel_r->m_color = Rgba8::WHITE;
	clavicle_r->m_objModels.push_back(m_shoulderModel_r);

	m_shoulderModel_l = new ObjModel();
	m_shoulderModel_l->LoadXml("Data/Models/Player_Shoulder_l.xml");
	m_shoulderModel_l->m_color = Rgba8::WHITE;
	clavicle_l->m_objModels.push_back(m_shoulderModel_l);

	// Right Arm
	m_upperArm_Connector_r = new ObjModel();
	m_upperArm_Connector_r->LoadXml("Data/Models/Player_UpperArm_Connector_r.xml");
	m_upperArm_Connector_r->m_color = Rgba8::WHITE;
	upperArm_r->m_objModels.push_back(m_upperArm_Connector_r);

	m_upperArm_Gear_r = new ObjModel();
	m_upperArm_Gear_r->LoadXml("Data/Models/Player_UpperArm_Gears_r.xml");
	m_upperArm_Gear_r->m_color = Rgba8::WHITE;
	upperArm_r->m_objModels.push_back(m_upperArm_Gear_r);

	m_lowerArm_Gear_r = new ObjModel();
	m_lowerArm_Gear_r->LoadXml("Data/Models/Player_LowerArm_Gears_r.xml");
	m_lowerArm_Gear_r->m_color = Rgba8::WHITE;
	lowerArm_r->m_objModels.push_back(m_lowerArm_Gear_r);

	m_lowerArm_Connector_r = new ObjModel();
	m_lowerArm_Connector_r->LoadXml("Data/Models/Player_LowerArm_Connector_r.xml");
	m_lowerArm_Connector_r->m_color = Rgba8::WHITE;
	lowerArm_r->m_objModels.push_back(m_lowerArm_Connector_r);

	// Mace
	m_maceModel = new ObjModel();
	m_maceModel->LoadXml("Data/Models/Mace.xml");
	m_maceModel->m_color = Rgba8::WHITE;
	hand_l->m_objModels.push_back(m_maceModel);
	hand_r->m_objModels.push_back(m_maceModel);

	// Left Arm
	m_upperArm_Connector_l = new ObjModel();
	m_upperArm_Connector_l->LoadXml("Data/Models/Player_UpperArm_Connector_l.xml");
	m_upperArm_Connector_l->m_color = Rgba8::WHITE;
	upperArm_l->m_objModels.push_back(m_upperArm_Connector_l);

	m_upperArm_Gear_l = new ObjModel();
	m_upperArm_Gear_l->LoadXml("Data/Models/Player_UpperArm_Gears_r.xml"); // **Are you sure it's "_r" here? Might be typo.**
	m_upperArm_Gear_l->m_color = Rgba8::WHITE;
	upperArm_l->m_objModels.push_back(m_upperArm_Gear_l);

	m_lowerArm_Gear_l = new ObjModel();
	m_lowerArm_Gear_l->LoadXml("Data/Models/Player_LowerArm_Gears_l.xml");
	m_lowerArm_Gear_l->m_color = Rgba8::WHITE;
	lowerArm_l->m_objModels.push_back(m_lowerArm_Gear_l);

	m_lowerArm_Connector_l = new ObjModel();
	m_lowerArm_Connector_l->LoadXml("Data/Models/Player_LowerArm_Connector_l.xml");
	m_lowerArm_Connector_l->m_color = Rgba8::WHITE;
	lowerArm_l->m_objModels.push_back(m_lowerArm_Connector_l);

	AssignModelsToHands();
}

physx::PxRigidDynamic* VRPlayer::CreateDynamicActorForJoint(std::string jointName)
{
	// get the location to generate the rigid dynamic actor
	Joint* j = m_skeleton->GetJointByName(jointName);
	float boneLength = j->GetBoneLength();
	Vec3 jointWorldPos = j->GetWorldPosition();
	Vec3 parentJointWorldPos = j->m_parent->GetWorldPosition();
	Vec3 jointMiddlePos = (jointWorldPos + parentJointWorldPos) * 0.5f;

	Mat44 posMat(jointMiddlePos);
	posMat = GetGameToPhysXSpaceMat().MatMultiply(posMat);
	PxVec3 pxPos = GetPxVec3(posMat.GetTranslation3D());
	Quat quat(m_orientation);
	PxQuat pxQuat = GetPxQuat(quat);
	PxTransform pxTransform(pxPos, pxQuat);

	PxRigidDynamic* rigidActor = g_thePhysX->m_physics->createRigidDynamic(pxTransform);

	DebuggerPrintf("VRPlayer joint dynamic actor userData set: %p ", this);

	rigidActor->setActorFlag(PxActorFlag::eVISUALIZATION, true);

	m_physXMaterial = g_thePhysX->GetPhysXMaterialByStinrg("player");
	// using createExclusiveShape, the shape is automatically attached to the actor it is created for
	PxShape* aCapsuleShape = PxRigidActorExt::createExclusiveShape(*rigidActor,
		PxCapsuleGeometry(m_joint_capsuleRadius, boneLength * 0.5f), *m_physXMaterial);

	PxFilterData filterData;
	filterData.word0 = 1; // Custom collision group the shape belongs to. Prevent collisions with parts of the VR player (same group)
	filterData.word1 = 2; // Collide with crawler parts
	filterData.word2 = 0; // more advanced filtering
	filterData.word3 = 0; // more advanced filtering

	aCapsuleShape->setSimulationFilterData(filterData);
	aCapsuleShape->setQueryFilterData(filterData);
	aCapsuleShape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, true);	// actually physx do it by default

	// Store a reference to the Actor in PhysX's userData field
	rigidActor->userData = this; // Store the VRPlayer pointer

	// set up mass and inertia and add to the physX scene
	PxRigidDynamic* dynamicActor = rigidActor->is<PxRigidDynamic>();
	if (dynamicActor)
	{
		PxRigidBodyExt::updateMassAndInertia(*dynamicActor, m_totalMass * 0.05f);
		g_thePhysX->m_scene->addActor(*rigidActor);
	}
	else
	{
		ERROR_AND_DIE("Failed to create dynamic rigid actor");
	}

	return rigidActor;
}

void VRPlayer::AssignModelsToHands()
{
	ObjModel* controllerModel_l = new ObjModel();
	controllerModel_l->LoadXml("Data/Models/Controller_l.xml");
	controllerModel_l->m_color = Rgba8::WHITE;

	m_hands[Side::LEFT].m_model = controllerModel_l;

	ObjModel* controllerModel_r = new ObjModel();
	controllerModel_r->LoadXml("Data/Models/Controller_r.xml");
	controllerModel_r->m_color = Rgba8::WHITE;

	m_hands[Side::RIGHT].m_model = controllerModel_r;
}

void VRPlayer::CreateDynamicRigidBodyForMech()
{
	// get physX transform info
	Mat44 posMat(m_position + Vec3(0.f, 0.f, m_capsuleHalfHeight + m_capsuleRadius));
	posMat = GetGameToPhysXSpaceMat().MatMultiply(posMat);
	PxVec3 pxPos = GetPxVec3(posMat.GetTranslation3D());
	Quat quat(m_orientation);
	PxQuat pxQuat = GetPxQuat(quat);
	PxTransform pxTransform(pxPos, pxQuat);

	// create capsule actor and rotate to along the +Y direction
	m_rigidActor = g_thePhysX->m_physics->createRigidDynamic(pxTransform);
	m_rigidActor->setActorFlag(PxActorFlag::eVISUALIZATION, true);

	// Store a reference to the Actor in PhysX's userData field
	m_rigidActor->userData = this; // Store the VRPlayer pointer
	DebuggerPrintf("VRPlayer userData set: %p", this);

	PxTransform relativePose(PxQuat(PxHalfPi, PxVec3(0.f, 0.f, 1.f)));
	m_physXMaterial = g_thePhysX->CreatePhysXMaterial("player", m_dynamicFriction, m_staticFriction, m_restitution);
	// using createExclusiveShape, the shape is automatically attached to the actor it is created for
	PxShape* aCapsuleShape = PxRigidActorExt::createExclusiveShape(*m_rigidActor,
																	PxCapsuleGeometry(m_capsuleRadius, m_capsuleHalfHeight), *m_physXMaterial);
	aCapsuleShape->setLocalPose(relativePose);

	PxFilterData filterData;
	filterData.word0 = 1; // Custom collision group
	filterData.word1 = 2; // Collides with the crawlers part
	filterData.word2 = 0;
	filterData.word3 = 0;

	aCapsuleShape->setSimulationFilterData(filterData);
	aCapsuleShape->setQueryFilterData(filterData);
	aCapsuleShape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, true);

	// set up mass and inertia and add to the physX scene
	PxRigidDynamic* dynamicActor = m_rigidActor->is<PxRigidDynamic>();
	if (dynamicActor)
	{
		PxRigidBodyExt::updateMassAndInertia(*dynamicActor, m_totalMass);
		g_thePhysX->m_scene->addActor(*m_rigidActor);
	}
	else
	{
		ERROR_AND_DIE("Failed to create dynamic rigid actor");
	}

	dynamicActor->wakeUp();
	dynamicActor->setActorFlag(PxActorFlag::eVISUALIZATION, true);
	// dynamicActor->setAngularDamping(0.0);	// larger the value is, the quick the angular velocity is going to decease
	// AddVertsForCollisionDebug();
}

void VRPlayer::CreateJointToConstrainMechBodyMovement()
{
	// Create a D6 joint with rotation restricted
	PxRigidDynamic* dynamicActor = m_rigidActor->is<PxRigidDynamic>();
	if (dynamicActor)
	{		
		PxD6Joint* joint = PxD6JointCreate(*g_thePhysX->m_physics, nullptr, PxTransform(PxIdentity), dynamicActor, dynamicActor->getGlobalPose());
		m_skeleton->GetJointByName("root")->m_D6Joint = joint;

		// set free motion for sliding in different directions
		joint->setMotion(PxD6Axis::eX, PxD6Motion::eFREE); // Free motion along X
		joint->setMotion(PxD6Axis::eY, PxD6Motion::eFREE); // Free motion along Y
		joint->setMotion(PxD6Axis::eZ, PxD6Motion::eFREE); // Free motion along Z	
	
		// // Allow free rotation around the Z axis (eSWING1)
		joint->setMotion(PxD6Axis::eTWIST, PxD6Motion::eLOCKED); // Allow spinning
		joint->setMotion(PxD6Axis::eSWING1, PxD6Motion::eFREE); // Allow spinning
		joint->setMotion(PxD6Axis::eSWING2, PxD6Motion::eLOCKED); // Allow spinning
		// have a swing limit will cause fall over while moving and rotating
		// Set swing limits (±15 degrees)
		// PxJointLimitCone swingLimit(PxPi / 180.f, PxPi / 180.f);
		// joint->setSwingLimit(swingLimit);
	
		// PxD6JointDrive drive(1000.0f, 100.0f, PX_MAX_F32); // Stiffness, damping, force limit
		// joint->setDrive(PxD6Drive::eSWING, drive);    // Stabilize swing motion
	
		// todo: should I set it every frame, or set it once and update its drive desired position every frame?
	
		PxD6JointDrive twistDrive(900.f, 100.f, PX_MAX_F32); // Stiffness, damping, force limit
		joint->setDrive(PxD6Drive::eTWIST, twistDrive);     // Stabilize twist motion

		PxD6JointDrive swingDrive(900.f, 100.0f, PX_MAX_F32); // Stiffness, damping, force limit
		joint->setDrive(PxD6Drive::eSWING, swingDrive);       // Stabilize swing motion
	}
	else
	{
		ERROR_AND_DIE("Failed to cast to dynamic rigid actor");
	}
}

void VRPlayer::Update()
{	
	UpdateRigidBodyTransform();
	UpdateHeadsetCameraPose();
	UpdateHands();
	UpdatePlayerShoulderDisp();
	UpdateMechArms();
	ControlMechToRotate();
	UpdateDebugInput();
}

void VRPlayer::UpdateRigidBodyTransform()
{
	// for testing we first try apply force to the rigid body and see if the skeleton can move with rigid body

	// Retrieve the updated global pose of the capsule actor
	PxTransform capsuleGlobalPose = m_rigidActor->getGlobalPose();

	// Extract the position and orientation
	PxVec3 capsulePos = capsuleGlobalPose.p;
	PxQuat capsuleCollision = capsuleGlobalPose.q;

	Mat44 quatMat = GetPhysXToGameSpaceMat().SimilarityTransformation(Mat44(Quat(capsuleCollision)));
	Vec3 actorZUp = quatMat.GetKBasis3D();
	m_position = GetPhysXToGameSpaceMat().TransformPosition3D(Vec3(capsulePos)) + (m_capsuleHalfHeight + m_capsuleRadius) * (actorZUp * -1.f);
	m_orientation = quatMat.GetEulerAngles();
}

void VRPlayer::UpdateHeadsetCameraPose()
{
	Mat44 actorMat = GetModelMatrix();
	Mat44 actorMatInOpenXR = GetGameToOpenXRMat().SimilarityTransformation(actorMat);
	// Quat quat = actorMatInOpenXR.GetQuaternion();
	// Quat quat = Mat44::TransformQuatBetweenSpaces(Quat(m_orientation), GetGameToPhysXSpaceMat());
	Quat quat = ConvertEulerAnglesToQuat(m_orientation);
	Vec3 pos = actorMatInOpenXR.GetTranslation3D();
	g_theRenderer->m_headOffsetPos = m_position;
	// g_theRenderer->m_headOffsetQuat = Quat::CreateRotationAroundYAxis(m_orientation.m_yawDegrees).GetNormalized();
	g_theRenderer->m_headOffsetOrientation = m_orientation;
}

void VRPlayer::Render() const
{
	RenderHands();
	RenderSkeleton();
}

void VRPlayer::RenderHands() const
{
	for (int i = 0; i < 2; ++i)
	{
		m_hands[i].Render();
	}
}

void VRPlayer::RenderSkeleton() const
{
	m_skeleton->Render();
}

void VRPlayer::UpdateHands()
{
	// if (g_theInput->WasKeyJustPressed(' '))
	// {
	// 	// Cast to PxRigidDynamic
	// 	PxRigidDynamic* dynamicActor = m_rigidActor->is<PxRigidDynamic>();
	// 	if (dynamicActor) 
	// 	{
	// 		// Apply force only if it's a dynamic actor
	// 		// dynamicActor->setAngularVelocity(PxVec3(0.f, 5.f, 0.f)); // Give it a spin
	// 		// dynamicActor->addForce(PxVec3(0, 0, 100), PxForceMode::eFORCE, true);			// eForce is adding force of n Newtons in the direction
	// 		// dynamicActor->addForce(PxVec3(0, 0, 10), PxForceMode::eIMPULSE, true);		// an instantaneous momentum of n kg·m/s in the +X direction
	// 
	// 		// dynamicActor->setAngularDamping(1.0);	// larger the value is, the quick the angular velocity is going to decease
	// 
	// 		// float rotationDegrees = 30.f;
	// 		// 
	// 		// Quat addingRotation = Quat::CreateRotationAroundYAxis(rotationDegrees);
	// 		// 
	// 		// PxTransform poseInPhysX = dynamicActor->getGlobalPose();
	// 		// 
	// 		// Quat crtQuat = Quat(poseInPhysX.q);
	// 		// crtQuat *= addingRotation;
	// 		// poseInPhysX.q = GetPxQuat(crtQuat);
	// 		// 
	// 		// PxD6Joint* joint = m_skeleton->GetJointByName("root")->m_D6Joint;
	// 		// joint->setDrivePosition(poseInPhysX);
	// 		// dynamicActor->setGlobalPose(poseInPhysX);
	// 
	// 		// based on the two update position, let apply torque for the two arm capsule and let them follow the procedural movement
	// 		Joint* upperArm_r = m_skeleton->GetJointByName("upperArm_r");
	// 		Joint* lowerArm_r = m_skeleton->GetJointByName("lowerArm_r");
	// 		PxRigidDynamic* upperDynamicActor = upperArm_r->m_rigidActor;
	// 		PxRigidDynamic* lowerDynamicActor = lowerArm_r->m_rigidActor;
	// 
	// 		Quat desiredUpperArmOrientation = upperArm_r->GetWorldQuat();
	// 		// Quat desiredUpperArmOrientation = Quat();
	// 		Quat desiredlowerArmOrientation = lowerArm_r->GetWorldQuat();
	// 
	// 		PxTransform upperDynamicActorPose = upperDynamicActor->getGlobalPose();
	// 		PxVec3 upperDynamicActorPos_px = upperDynamicActorPose.p;
	// 		PxQuat upperDynamicActorOrientation_px = upperDynamicActorPose.q;
	// 		Vec3 upperDynamicActorPos = GetPhysXToGameSpaceMat().TransformPosition3D(Vec3(upperDynamicActorPos_px));
	// 		Quat upperDynamicActorOrientation = GetPhysXToGameSpaceMat().TransformQuaternion(Quat(upperDynamicActorOrientation_px));
	// 
	// 		PxD6Joint* upperArm_r_D6_joint = upperArm_r->m_D6Joint;
	// 		Quat jointWorldRotation = (GetPhysXToGameSpaceMat().TransformQuaternion(Quat(m_rigidActor->getGlobalPose().q)) *
	// 			GetPhysXToGameSpaceMat().TransformQuaternion(Quat(upperArm_r_D6_joint->getLocalPose(PxJointActorIndex::eACTOR0).q))).GetNormalized();
	// 		Quat driveRotation = (jointWorldRotation.GetInversed() * desiredUpperArmOrientation).GetNormalized();
	// 		Quat test = jointWorldRotation * driveRotation;
	// 		Quat test2 = Quat(upperArm_r_D6_joint->getLocalPose(PxJointActorIndex::eACTOR0).q);
	// 		// Quat jointLocalRotation = GetPxQuat(GetGameToPhysXSpaceMat().TransformQuaternion(desiredUpperArmOrientation));
	// 		PxTransform t;
	// 		// t.q = GetPxQuat(GetGameToPhysXSpaceMat().TransformQuaternion(driveRotation));
	// 		Quat test3 = ConvertEulerAnglesToQuat(EulerAngles(0.f, 0.f, 30.f));
	// 		Quat test5 = test3.ConvertDifferentHandSystem();
	// 		Quat test4 = GetGameToPhysXSpaceMat().TransformQuaternion(test5);
	// 
	// 		t.q = GetPxQuat(TransformQuatFromGameToPhysX(driveRotation));
	// 		t.p = upperArm_r_D6_joint->getDrivePosition().p;
	// 		upperArm_r_D6_joint->setDrivePosition(t);
	// 	}
	// }
	
	for (int i = 0; i < 2; ++i)
	{
		m_hands[i].Update();
	}
}

void VRPlayer::UpdateMechArms()
{
	g_theWinPlayer->m_debugChain = m_arm_r;
	if (g_theWinPlayer->m_solveTarget)
	{
		DebugChainByWinPlayer(m_arm_r);
	}
	else
	{
		UpdateArmChainByHand(m_arm_l, &m_hands[Side::LEFT]);
		UpdateArmChainByHand(m_arm_r, &m_hands[Side::RIGHT]);
	}

	ControlMechMovement();
}

void VRPlayer::ControlMechMovement()
{
	// if one grip button is pressed, player drag to move
	int numGripButton = 0;
	for (int handIndex = 0; handIndex < 2; ++handIndex)
	{
		if (m_hands[handIndex].m_grabValue > m_grabToMoveValue)
		{
			++numGripButton; 
		}
	}

	if (numGripButton == 1)
	{
		for (int handIndex = 0; handIndex < 2; ++handIndex)
		{
			if (m_hands[handIndex].m_grabValue > m_grabToMoveValue)
			{
				// if the mech hand is holding in place for a period of time, stop the actor moving
				float dispLen = m_mechHandDisp[handIndex].GetLength();
				// Log::Write(Log::Level::Info, Stringf("DispLen: %.2f", dispLen));

				PxRigidDynamic* dynamicActor = m_rigidActor->is<PxRigidDynamic>();
				Vec3 linearVelocity(dynamicActor->getLinearVelocity());
				float speed = linearVelocity.GetLength();

				if (dispLen < m_mechBrakeDisp) // && speed >= m_mechBrakeMinSpeed)
				{
					dynamicActor->setLinearDamping(m_mechBrakeLinearDampingValue);
					return;
				}
				else
				{
					dynamicActor->setLinearDamping(0.f);
				}

				// the force is related to the length of the disp
				// do not add force when the mech speed is already over max speed
				// if player is gripping with both hands, add only one force instead of two, calculate the average pos
				if (dynamicActor && speed <= m_maxMovingSpeed)
				{
					PxVec3 forceDir = GetPxVec3(GetGameToPhysXSpaceMat().TransformVectorQuantity3D(m_mechHandDisp[handIndex]));
					// Log::Write(Log::Level::Info, Stringf("Speed: %.2f", speed));
					dynamicActor->addForce(forceDir * m_mechSlideForceMultiplier, PxForceMode::eFORCE, true);
				}			
			}
		}
	}
	else if (numGripButton == 2)
	{
		// if two grip buttons are pressed, if both of them are moving in the same direction
		// we slide to move
		float dotProductResult = DotProduct3D(m_mechHandDisp[0], m_mechHandDisp[1]);
		if (dotProductResult > 0.f)
		{
			for (int handIndex = 0; handIndex < 2; ++handIndex)
			{
				if (m_hands[handIndex].m_grabValue > m_grabToMoveValue)
				{
					// if the mech hand is holding in place for a period of time, stop the actor moving
					float dispLen = m_mechHandDisp[handIndex].GetLength();
					// Log::Write(Log::Level::Info, Stringf("DispLen: %.2f", dispLen));

					PxRigidDynamic* dynamicActor = m_rigidActor->is<PxRigidDynamic>();
					Vec3 linearVelocity(dynamicActor->getLinearVelocity());
					float speed = linearVelocity.GetLength();

					if (dispLen < m_mechBrakeDisp && speed >= m_mechBrakeMinSpeed)
					{
						dynamicActor->setLinearDamping(m_mechBrakeLinearDampingValue);
						return;
					}
					else
					{
						dynamicActor->setLinearDamping(0.f);
					}

					// the force is related to the length of the disp
					// do not add force when the mech speed is already over max speed
					// if player is gripping with both hands, add only one force instead of two, calculate the average pos
					if (dynamicActor && speed <= m_maxMovingSpeed)
					{
						PxVec3 forceDir = GetPxVec3(GetGameToPhysXSpaceMat().TransformVectorQuantity3D(m_mechHandDisp[handIndex]));
						// Log::Write(Log::Level::Info, Stringf("Speed: %.2f", speed));
						dynamicActor->addForce(forceDir * m_mechSlideForceMultiplier, PxForceMode::eFORCE, true);
					}
				}
			}
		}
		// if the disp are in opposite directions, we let the mech rotate
		else if (dotProductResult < 0.f)
		{
			float rightHandDispLen =  m_mechHandDisp[Side::RIGHT].GetLength();
			float leftHandDispLen =  m_mechHandDisp[Side::LEFT].GetLength();
			float rotationDegrees = m_motionRotatingMultiplier * (rightHandDispLen + leftHandDispLen);

			// if the right hand is moving forward or left hand is moving backward
			// we rotate the mech clockwise
			// if the right hand is moving forward or left hand is moving backward
			// we rotate the mech clockwise
			float rightDispForwardProductResult = DotProduct3D(m_orientation.GetForwardIBasis(), m_mechHandDisp[Side::RIGHT]);
			float leftDispForwardProductResult = DotProduct3D(m_orientation.GetForwardIBasis(), m_mechHandDisp[Side::LEFT]);
			if (rightDispForwardProductResult < 0.f || leftDispForwardProductResult > 0.f)
			{
				rotationDegrees *= -1.f;
			}

			Quat addingRotation = Quat::CreateRotationAroundYAxis(rotationDegrees);

			PxRigidDynamic* dynamicActor = m_rigidActor->is<PxRigidDynamic>();
			PxTransform poseInPhysX = dynamicActor->getGlobalPose();

			Quat crtQuat = Quat(poseInPhysX.q);
			crtQuat *= addingRotation;
			poseInPhysX.q = GetPxQuat(crtQuat);
			dynamicActor->setGlobalPose(poseInPhysX);

			PxD6Joint* joint = m_skeleton->GetJointByName("root")->m_D6Joint;
			joint->setDrivePosition(dynamicActor->getGlobalPose());
		}
	}
}

void VRPlayer::UpdatePlayerShoulderDisp()
{
	m_disp_actor_shoulder_r = Vec3(-0.1f, -0.16f, 1.7f);
	m_disp_actor_shoulder_l = Vec3(-0.1f, 0.16f, 1.7f);

	// Vec3 shoulderOffset = m_headTrackingPos - m_skeleton->m_actor->m_position;
	Vec3 shoulderOffset = m_headTrackingPos;
	m_disp_actor_shoulder_l += shoulderOffset;
	m_disp_actor_shoulder_r += shoulderOffset;
}

Vec3 VRPlayer::GetMechHandPosFromPlayerHandAndVirtualShoulder(int sideIndex)
{
	Chain* chain = nullptr;
	Vec3 disp_actor_shoulder;

	// assign the joints
	string JointStr_hand;
	string JointStr_lowerArm;
	string JointStr_upperArm;
	if (sideIndex == Side::LEFT)
	{
		chain = m_arm_l;
		JointStr_hand = "hand_l";
		JointStr_lowerArm = "lowerArm_l";
		JointStr_upperArm = "upperArm_l";
		disp_actor_shoulder = m_disp_actor_shoulder_l;
	}
	else
	{
		chain = m_arm_r;
		JointStr_hand = "hand_r";
		JointStr_lowerArm = "lowerArm_r";
		JointStr_upperArm = "upperArm_r";
		disp_actor_shoulder = m_disp_actor_shoulder_r;
	}

	// the disp from head -> shoulder are moved according to forward direction
	// Vec3 forwardDir = Vec3::GetDirectionForYawPitch(m_orientation.m_yawDegrees, m_orientation.m_pitchDegrees);
	disp_actor_shoulder = m_orientation.GetAsMatrix_XFwd_YLeft_ZUp().TransformVectorQuantity3D(disp_actor_shoulder);

	// player's shoulder position
	Vec3 shoulderPosition = m_skeleton->m_actor->m_position + disp_actor_shoulder;
	Vec3 disp_hand_shoulder_player = m_hands[sideIndex].m_pos - shoulderPosition;
	float length_hand_shoulder = disp_hand_shoulder_player.GetLength();


	// based on the hand-shoulder disp calculate the scale of the mech-hand-shoulder
	float upperArmLength = m_skeleton->GetJointByName(JointStr_lowerArm)->GetBoneLength();
	float handLength = m_skeleton->GetJointByName(JointStr_hand)->GetBoneLength();
	float mechArmLength = upperArmLength + handLength;
	float mechTargetDisp = RangeMapClamped(length_hand_shoulder, 0.f, m_playerMaxArmLength, 0.6f * mechArmLength, mechArmLength);

	// string debugStr = Stringf("disp: %.2f, %.2f, %.2f", disp_hand_shoulder_player.x, disp_hand_shoulder_player.y, disp_hand_shoulder_player.z);
	// DebugAddScreenText(debugStr, Vec2(), 20.f, Vec2(0.5f, 0.5f), -1.f, Rgba8::WHITE);

	Vec3 disp_hand_shoulder_mech = disp_hand_shoulder_player.GetNormalized() * mechTargetDisp;
	Vec3 targetPos = m_skeleton->GetJointByName(JointStr_upperArm)->GetWorldPosition() + disp_hand_shoulder_mech;
	return targetPos;
}

void VRPlayer::SetArmChainPoleVectorByVirtualElbowPos(int sideIndex)
{
	// assign the chain
	Chain* chain = nullptr;
	if (sideIndex == Side::LEFT)
	{
		chain = m_arm_l;
	}
	else
	{
		chain = m_arm_r;
	}

	// the pole vector is relatively above the hand
	Mat44 handOrientationMat = Mat44(m_hands[sideIndex].m_orientation);
	Vec3 kBasis = handOrientationMat.GetKBasis3D();
	kBasis = kBasis.GetNormalized();
	m_mechArmPoleVectorPos[sideIndex] = m_mechArm_current_TargetPos[sideIndex] + kBasis;

	chain->m_solver->SetPoleVector(m_mechArmPoleVectorPos[sideIndex]);
	DebugAddWorldWireSphere(m_mechArmPoleVectorPos[sideIndex], 0.1f, 0.2f, Rgba8::RED, Rgba8::RED);
}

void VRPlayer::UpdateArmChainByHand(Chain* chain, PlayerHand* hand)
{	
	UNUSED(chain)
	// update last frame arm anchored status
	int handIndex = hand->m_handIndex;
	m_isArmAnchored_lastFrame[handIndex] = m_isArmAnchored_current[handIndex];

	if (hand->m_isActive)
	{
		// update current frame
		if (hand->m_grabValue >= m_grabToMoveValue)
		{
			m_isArmAnchored_current[handIndex] = true;
			ControlArmToMove(handIndex);
			UpdateAnchorPos(handIndex);
		}
		else
		{
			m_isArmAnchored_current[handIndex] = false;
			m_mechHandDisp[handIndex] = Vec3::ZERO;
			ControlArmForCombat(handIndex);
		}
	}
	else
	{
		m_isArmAnchored_current[handIndex] = false;
	}
}

void VRPlayer::ControlArmForCombat(int sideIndex)
{
	// define which arm chain to control by side index
	Chain* chain = nullptr;
	string upperArmString;
	string lowerArmString;
	Quat adjustment;

	if (sideIndex == Side::LEFT)
	{
		chain = m_arm_l;
		upperArmString = "upperArm_l";
		lowerArmString = "lowerArm_l";
		adjustment = ConvertEulerAnglesToQuat(EulerAngles(-90.f, 0.f, 0.f));
	}
	else
	{
		chain = m_arm_r;
		upperArmString = "upperArm_r";
		lowerArmString = "lowerArm_r";
		adjustment = ConvertEulerAnglesToQuat(EulerAngles(90.f, 0.f, 0.f));
	}

	m_mechArm_current_TargetPos[sideIndex] = GetMechHandPosFromPlayerHandAndVirtualShoulder(sideIndex);
	chain->m_solver->SetSolverTarget(m_mechArm_current_TargetPos[sideIndex]);
	
	DebugAddWorldWireSphere(m_mechArm_current_TargetPos[sideIndex], 0.1f, 0.2f, Rgba8::BLUE, Rgba8::BLUE);

	SetArmChainPoleVectorByVirtualElbowPos(sideIndex);

	// use procedural animation data to update physical animation data
	// if arm did not move, do not align again, otherwise the arm is going to keep rotating because of the arm cannot fully align with arm due to gravity
	// GetDistanceSquared3D(m_mechArm_lastFrame_TargetPos[sideIndex], m_mechArm_current_TargetPos[sideIndex]) >= 0.000001f
	if (GetDistance3D(m_hands[sideIndex].m_posLastFrame, m_hands[sideIndex].m_pos) >= 0.0002f)
	{
		chain->m_solver->SolveIK();

		// based on the two update position, let apply torque for the two arm capsule and let them follow the procedural movement
		Joint* upperArmJoint = m_skeleton->GetJointByName(upperArmString);
		Joint* lowerArmJoint = m_skeleton->GetJointByName(lowerArmString);

		Quat upperArmRotation = SetD6JointDriveAlignWithJointOrientation(upperArmJoint, adjustment);
		Quat lowerArmRotation = SetD6JointDriveAlignWithJointOrientation(lowerArmJoint, adjustment);

		if (sideIndex == 1)
		{
			Quat q = upperArmJoint->GetWorldQuat();
			// Log::Write(Log::Level::Info, Stringf("RIGHT: %.2f, %.2f, %.2f, %.2f", q.x, q.y, q.z, q.w));
		}

		// use physical animation data to present the blending result
		upperArmJoint->m_localRotation = upperArmJoint->m_parent->GetWorldQuat().GetInversed() * upperArmRotation;
		lowerArmJoint->m_localRotation = upperArmJoint->GetWorldQuat().GetInversed() * lowerArmRotation;
	}

	// record the target pos
	m_mechArm_lastFrame_TargetPos[sideIndex] = m_mechArm_current_TargetPos[sideIndex];

	// the way of applying torque is unstable
	// Quat upperArmRotationDiff = (desiredUpperArmOrientation * upperDynamicActorOrientation.GetInversed()).GetNormalized();
	// Vec3 axis;
	// float angle;
	// upperArmRotationDiff.GetRotationAxisAndAngle(axis, angle);
	// Vec3 angularVel = axis * angle * 0.005f; // 10.f is gain
	// // Vec3 movedAngularVel = CrossProduct3D((upperArm_r->GetWorldPosition() - upperDynamicActorPos), angularVel);
	// 
	// PxVec3 torqueDir = GetPxVec3(GetGameToPhysXSpaceMat().TransformVectorQuantity3D(angularVel));
	// 
	// // Apply torque to the actor
	// upperDynamicActor->addTorque(torqueDir, PxForceMode::eFORCE);
	// //Log::Write(Log::Level::Info, Fmt("Right Hand Orientation: X = %.02f,Y = %.02f, Z = %.02f", rotation.x, rotation.y, rotation.z));
	// Log::Write(Log::Level::Info, Fmt("Angle = %.02f", angle * 180.f / PI));
	// // string distStr = Stringf("%.2f", GetDistance3D(chain->GetEndJoint()->GetWorldPosition(), m_mechArmTargetPos[sideIndex]));
	// // DebugAddScreenText(distStr, Vec2(0.5f, 0.5f), 30.f, Vec2(0.5f, 0.5f), -1.f);
}

// return the D6 joint's world rotation for skeleton joint to follow
Quat VRPlayer::SetD6JointDriveAlignWithJointOrientation(Joint* skeletonJoint, Quat const& rotationAdjustment /*= Quat()*/)
{
	PxD6Joint* D6Joint = skeletonJoint->m_D6Joint;
	PxRigidActor* actor0 = nullptr;		// the rigid actor that the D6 joint is on
	PxRigidActor* actor1 = nullptr;		// the rigid actor that the D6 joint drive controls
	D6Joint->getActors(actor0, actor1);
	if (!D6Joint)
	{
		ERROR_AND_DIE("This skeleton joint does not have a D6 joint!");
	}

	// get two joints world rotation
	Quat desiredJointOrientation = skeletonJoint->GetWorldQuat();
	Quat actor0_worldOrientation = TransformQuatFromPhysXToGameAndGetInversed(Quat(actor0->getGlobalPose().q));
	Quat D6Joint_localOrientation = TransformQuatFromPhysXToGameAndGetInversed(Quat(D6Joint->getLocalPose(PxJointActorIndex::eACTOR0).q));
	Quat D6JointWorldRotation = ( actor0_worldOrientation * D6Joint_localOrientation).GetNormalized();

	// calculate the local rotation the D6 joint needs to make
	desiredJointOrientation *= rotationAdjustment; // fucking why?
	Quat driveRotation = (D6JointWorldRotation.GetInversed() * desiredJointOrientation).GetNormalized();

	// do not change the D6 joint's location but update its orientation
	PxTransform t;
	t.q = GetPxQuat(TransformQuatFromGameToPhysXAndGetInversed(driveRotation));
	t.p = D6Joint->getDrivePosition().p;
	D6Joint->setDrivePosition(t);

	// get the joint's world rotation by getting the orientation of the dynamic actor's that it's controlling
	return TransformQuatFromPhysXToGameAndGetInversed(Quat(actor1->getGlobalPose().q)) * rotationAdjustment.GetInversed();
}

void VRPlayer::ControlArmToMove(int sideIndex)
{
	// based on the side index, assign the hand and chain to control
	Chain* chain = nullptr;
	PlayerHand* hand = nullptr;
	if (sideIndex == Side::LEFT)
	{
		chain = m_arm_l;
		hand = &m_hands[Side::LEFT];
	}
	else
	{
		chain = m_arm_r;
		hand = &m_hands[Side::RIGHT];
	}

	Vec3 targetPos = GetMechHandPosFromPlayerHandAndVirtualShoulder(sideIndex);

	// todo: in future version, we use physX to raycast downwards to get ground position
	// for now we overwrite the mech arm position height to be 0
	// play the arm animation of anchoring to the ground
	targetPos.z = 0.f;
	chain->m_solver->SetSolverTarget(targetPos);
	SetArmChainPoleVectorByVirtualElbowPos(sideIndex);
	chain->m_solver->SolveIK();

	// targetPos.z = 0.f;
	// chain->m_solver->SetSolverTarget(targetPos);
	// SetArmChainPoleVectorByVirtualElbowPos(sideIndex);
	// 
	// chain->m_solver->SolveIK();
}

void VRPlayer::UpdateAnchorPos(int sideIndex)
{
	// based on the side index, assign the hand and chain to control
	Chain* chain = nullptr;
	PlayerHand* hand = nullptr;
	if (sideIndex == Side::LEFT)
	{
		chain = m_arm_l;
		hand = &m_hands[Side::LEFT];
	}
	else
	{
		chain = m_arm_r;
		hand = &m_hands[Side::RIGHT];
	}

	// if current hand is released, then we set the mech anchor pos and hand anchor pos to be invalid
	// if (!m_isArmAnchored_current[sideIndex]) 
	// {
	// 	m_mechHandAnchorPos[sideIndex] = Vec3::INVALID;
	// 	m_playerHandAnchorPos[sideIndex] = Vec3::INVALID;
	// }

	// if the anchor pos in record is invalid and now the arm is anchored
	// we going to record the player hand and mech hand's pos when mech's hand touches the ground
	if (!m_isArmAnchored_lastFrame[sideIndex] && m_isArmAnchored_current[sideIndex])
	{
		// record the disp from the actor pos
		m_playerHandAnchorPos[sideIndex] = hand->GetWorldPosInGame() - m_position; 
		m_mechHandAnchorPos[sideIndex] = chain->m_solver->m_end->GetWorldPosition() - m_position;
	}
	// if we have previous mech anchor pos and we are still anchored, we try to move the rigid actor
	else if (m_isArmAnchored_lastFrame[sideIndex] && m_isArmAnchored_current[sideIndex])
	{
		// Get current moving info
		Vec3 currentMechHandPos =  chain->m_solver->m_end->GetWorldPosition() - m_position;
		Vec3 disp = currentMechHandPos - m_mechHandAnchorPos[sideIndex];
		disp *= -1.f;
		disp.z = 0.f;

		m_mechHandDisp[sideIndex] = disp;

		// update recorded value
		m_playerHandAnchorPos[sideIndex] = hand->GetWorldPosInGame() - m_position;
		m_mechHandAnchorPos[sideIndex] = currentMechHandPos;
	}
	// if player release the hand, let add impulse to the actor
	else if (m_isArmAnchored_lastFrame[sideIndex] && !m_isArmAnchored_current[sideIndex])
	{
		// add force for the dynamic actor
		PxRigidDynamic* kinematicActor = m_rigidActor->is<PxRigidDynamic>();
		m_rigidActor = g_thePhysX->ConvertActorFromKinematicToDynamic(kinematicActor);
		// the direction is the disp from last frame hand pos to current frame hand pos
		Vec3 disp = hand->GetWorldPosInGame() - m_playerHandAnchorPos[sideIndex];
		
		PxVec3 forceDir = GetPxVec3(GetGameToPhysXSpaceMat().TransformVectorQuantity3D(disp));
		// the force is related to the length of the disp
		// dynamicActor->addForce(forceDir * m_mechSlideForceMultiplier, PxForceMode::eFORCE, true);

		// reset record
		m_mechHandAnchorPos[sideIndex] = Vec3::INVALID;
		m_playerHandAnchorPos[sideIndex] = Vec3::INVALID;
	}
}

void VRPlayer::DebugChainByWinPlayer(Chain* chain)
{
	g_theWinPlayer->m_debugChain = chain;
	if (g_theWinPlayer->m_solveTarget)
	{
		chain->m_solver->SetSolverTarget(g_theWinPlayer->m_movingTarget_endEffector);
		chain->m_solver->m_poleVecPos = g_theWinPlayer->m_movingTarget_poleVector;
		chain->m_solver->m_usePoleVector = true;
		chain->m_solver->SolveIK();
	}
}

void VRPlayer::ControlMechToRotate()
{
	if (m_snapTurnMode)
	{
		if (!m_hasRotated)
		{
			float x = m_hands[Side::RIGHT].m_joyStickPos.x;
			if (abs(x) >= m_joystickMinToSnapRotate)
			{
				PxD6Joint* joint = m_skeleton->GetJointByName("root")->m_D6Joint;

				// based on whether input is left or right
				// decide the rotation to be clockwise or counter clockwise
				float rotationDegrees = m_rotationDegrees;
				if (x <= -m_joystickMinToSnapRotate)
				{
					rotationDegrees *= -1.f;
				}

				Quat addingRotation = Quat::CreateRotationAroundYAxis(rotationDegrees);

				PxRigidDynamic* dynamicActor = m_rigidActor->is<PxRigidDynamic>();
				PxTransform poseInPhysX = dynamicActor->getGlobalPose();

				// Apply torque around the Y-axis
				// dynamicActor->addTorque(PxVec3(0.0f, -torqueStrength, 0.0f), PxForceMode::eIMPULSE);
				// float torqueStrength = .01f; // Adjust based on desired rotation speed

				Quat crtQuat = Quat(poseInPhysX.q);
				crtQuat *= addingRotation;
				poseInPhysX.q = GetPxQuat(crtQuat);

				dynamicActor->setGlobalPose(poseInPhysX);
				joint->setDrivePosition(dynamicActor->getGlobalPose());

				// rotation timer start to cooling
				m_rotationResetTimer->Start();
				m_hasRotated = true;
			}
		}
		else
		{
			// if player has already rotated
			// we need to check the angular speed to make sure the mech is settled
			// or maybe just have a cool down to double make sure of that
			// it requires player's to reset the joystick to rotate again
			if (m_rotationResetTimer->HasPeroidElapsed())
			{
				m_rotationResetTimer->Start();
				m_hasRotated = false;
			}
		}
	}
	else
	{
		float x = m_hands[Side::RIGHT].m_joyStickPos.x;
		if (abs(x) >= m_joystickMinToSmoothRotate)	// prevent player touch turning by mistake
		{
			float rotationDegrees = m_smoothTurningRate * g_theGameClock->GetDeltaSeconds();
			if (x <= -m_joystickMinToSnapRotate)
			{
				rotationDegrees *= -1.f;
			}

			Quat addingRotation = Quat::CreateRotationAroundYAxis(rotationDegrees);

			PxRigidDynamic* dynamicActor = m_rigidActor->is<PxRigidDynamic>();
			PxTransform poseInPhysX = dynamicActor->getGlobalPose();

			Quat crtQuat = Quat(poseInPhysX.q);
			crtQuat *= addingRotation;
			poseInPhysX.q = GetPxQuat(crtQuat);

			dynamicActor->setGlobalPose(poseInPhysX);

			PxD6Joint* joint = m_skeleton->GetJointByName("root")->m_D6Joint;
			joint->setDrivePosition(dynamicActor->getGlobalPose());
		}
	}

	// float x = m_hands[Side::RIGHT].m_joyStickPos.x;
	// if (abs(x) >= m_joystickMinToRotate)
	// {
	// 	PxD6Joint* joint = m_skeleton->GetJointByName("root")->m_D6Joint;
	// 
	// 	// based on whether input is left or right
	// 	// decide the rotation to be clockwise or counter clockwise
	// 	float rotationDegrees = m_rotationDegrees;
	// 	if (x <= -m_joystickMinToRotate)
	// 	{
	// 		rotationDegrees *= -1.f;
	// 	}
	// 
	// 	// calculate the transform after rotation
	// 	// EulerAngles angleInGame(m_orientation.m_yawDegrees + rotationDegrees, m_orientation.m_pitchDegrees, m_orientation.m_rollDegrees);
	// 	// Quat quatInGame = ConvertEulerAnglesToQuat(angleInGame);
	// 	// Quat quatInPhysX = Mat44::TransformQuatBetweenSpaces(quatInGame, GetGameToPhysXSpaceMat());
	// 	Quat addingRotation = Quat::CreateRotationAroundYAxis(rotationDegrees);
	// 
	// 	PxRigidDynamic* dynamicActor = m_rigidActor->is<PxRigidDynamic>();
	// 	PxTransform poseInPhysX = dynamicActor->getGlobalPose();
	// 	// Vec3 posInGameWorld = GetPhysXToGameSpaceMat().TransformPosition3D(Vec3(poseInPhysX.p));
	// 	// Vec3 posInPhysx = GetGameToPhysXSpaceMat().TransformPosition3D(m_position + Vec3(0.f, 0.f, m_capsuleHalfHeight + m_capsuleRadius));
	// 
	// 	// PxTransform newPoseInPhysX = MakePhysxTransform(posInPhysx, quatInPhysX);
	// 
	// 	float torqueStrength = 100.0f; // Adjust based on desired rotation speed
	// 
	// 	// Apply torque around the Y-axis
	// 	// dynamicActor->addTorque(PxVec3(0.0f, torqueStrength, 0.0f), PxForceMode::eFORCE);
	// 
	// 	Quat crtQuat = Quat(poseInPhysX.q);
	// 	crtQuat *= addingRotation;
	// 	poseInPhysX.q = GetPxQuat(crtQuat);
	// 	// joint->setDrivePosition(poseInPhysX);
	// 	dynamicActor->setGlobalPose(poseInPhysX);
	// 
	// 	// rotation timer start to cooling
	// 	m_rotationResetTimer->Start();
	// 	m_hasRotated = true;
	// }
}

void VRPlayer::UpdateDebugInput()
{
	// if (g_theInput->WasKeyJustPressed(' '))
	// {
	// 	m_arm_r->m_solver->m_debugMode = true;
	// 	m_arm_r->m_solver->m_debugCounter += 1;
	// }

	// if (g_theInput->WasKeyJustPressed(KEYCODE_ENTER))
	// {
	// 	m_arm_r->m_solver->m_debugMode = false;
	// }
}

void VRPlayer::UpdateHandPoseTracking(XrPosef handPose, int handIndex)
{
	if (handIndex == Side::LEFT)
	{
		m_hands[0].m_actionSpacePose = handPose;
	}
	else if (handIndex == Side::RIGHT)
	{
		m_hands[1].m_actionSpacePose = handPose;
	}
}

void VRPlayer::UpdateHeadPoseTracking(Mat44 headTransform)
{
	m_headTrackingPos = headTransform.GetTranslation3D();
	m_headTrackingOrientation = headTransform.GetQuaternion();
	m_headMat = headTransform;

	if (m_hands[Side::LEFT].m_grabValue > 0.5f && m_hands[Side::RIGHT].m_grabValue > 0.5f)
	{
		Vec3 disp_R = m_hands[Side::RIGHT].m_pos - m_headTrackingPos;
		Vec3 disp_L = m_hands[Side::LEFT].m_pos - m_headTrackingPos;
		// Log::Write(Log::Level::Info, Stringf("RIGHT: %.2f, %.2f, %.2f", disp_R.x, disp_R.y, disp_R.z));
		// Log::Write(Log::Level::Info, Stringf("LEFT: %.2f, %.2f, %.2f", disp_L.x, disp_L.y, disp_L.z));
	}
}