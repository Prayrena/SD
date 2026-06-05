#include "Engine/Animation/ProceduralAnimation/IKTwoBonesSolver.hpp"
#include "Engine/Renderer/DebugRender.hpp"
#include "ThirdParty/OpenXR/Application/logger.h"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/core/VertexUtils.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/core/EngineCommon.hpp"


TwoJointsIKSolver::TwoJointsIKSolver(Joint* root, Joint* mid, Joint* end)
		: m_root(root)
		, m_mid(mid)
		, m_end(end)
{
	m_joints.push_back(m_root);
	m_joints.push_back(m_mid);
	m_joints.push_back(m_end);
}

TwoJointsIKSolver::TwoJointsIKSolver(TwoJointsIKSolver* copiedSolver)
{
	m_root = new Joint(copiedSolver->m_root);
	m_mid = new Joint(copiedSolver->m_mid);
	m_mid->m_parent = m_root;
	m_end = new Joint(copiedSolver->m_end);
	m_end->m_parent = m_mid;
}

// make debug control that 
void TwoJointsIKSolver::SolveIK()
{
	// if (!m_usePoleVector)
	// {
	// 	if (GetDistanceSquared3D(m_targetLastFrame, m_target) <= 0.0001f)
	// 	{
	// 		return;
	// 	}
	// }
	// else
	// {
	// 	if (GetDistanceSquared3D(m_targetLastFrame, m_target) <= 0.0001f && GetDistanceSquared3D(m_poleVectorLastFrame, m_poleVecPos) <= 0.0001f)
	// 	{
	// 		return;
	// 	}
	// }

	// Step 1: Extend/Contract the Joint Chain
	// we will get a rotation for A and another rotation for B
	// if the extend or contract is almost done, do not do it again.
	Joint*& A = m_root;
	Joint*& B = m_mid;
	Joint*& C = m_end;
	Vec3 T = m_target;

	float eps = 0.01f;
	Vec3 A_worldPos = A->GetWorldPosition();
	Vec3 B_worldPos = B->GetWorldPosition();
	Vec3 C_worldPos = C->GetWorldPosition();

	float l_AB = GetDistance3D(B_worldPos, A_worldPos);
	float l_BC = GetDistance3D(B_worldPos, C_worldPos);
	float l_AT = GetClamped(GetDistance3D(T, A_worldPos), eps, (l_AB + l_BC - eps));

	// get the current interior angles of the hip and knee joints
	float a_AC_AB_current = acosf(GetClamped(DotProduct3D(
		(C_worldPos - A_worldPos).GetNormalized(),
		(B_worldPos - A_worldPos).GetNormalized()), -1.f, 1.f));

	float a_BA_BC_current = acosf(GetClamped(DotProduct3D(
		(A_worldPos - B_worldPos).GetNormalized(),
		(C_worldPos - B_worldPos).GetNormalized()), -1.f, 1.f));

	// Use The cosine rule to get the desired interior angles of these joints
	float a_AC_AB_desired = acosf(GetClamped((l_AB * l_AB + l_AT * l_AT - l_BC * l_BC) / ( 2.f * l_AB * l_AT)
								, -1.f, 1.f));
	float a_BA_BC_desired = acosf(GetClamped((l_AB * l_AB + l_BC * l_BC - l_AT * l_AT) / (2.f * l_AB * l_BC)
								, -1.f, 1.f));

	// the rotation axis for ABC
	Vec3 worldRotationAxis_CA_BA;
	if (CheckIfTwoVec3AreInTheSameDirection(C_worldPos - A_worldPos, B_worldPos - A_worldPos))
	{
		Mat44 A_transformMat = A->GetWorldTransformMat();
		Vec3 axis = A_transformMat.GetJBasis3D();
		worldRotationAxis_CA_BA = CrossProduct3D(C_worldPos - A_worldPos, axis).GetNormalized();
	}
	else
	{
		worldRotationAxis_CA_BA = CrossProduct3D(C_worldPos - A_worldPos, B_worldPos - A_worldPos).GetNormalized(); // todo: it will unstable when fully extended?
	}

	// Since we are working in the world space we have to first transform this axis into the local space of each joint (since the rotations will be applied locally)
	// this is achieved by applying the inverse of world rotation of the joint to the axis
	Quat A_worldQuat = A->GetWorldQuat();
	Quat B_worldQuat = B->GetWorldQuat();
	Quat C_worldQuat = C->GetWorldQuat();

	// Vec3 localRotationAxis_A = Quat::RotateVectorByQuat(B_worldQuat.GetInversed(), worldRotationAxis_CA_BA);
	// Vec3 localRotationAxis_B = Quat::RotateVectorByQuat(C_worldQuat.GetInversed(), worldRotationAxis_CA_BA);	
	
	Vec3 localRotationAxis_A = A_worldQuat.GetInversed().RotateVector(worldRotationAxis_CA_BA);
	Vec3 localRotationAxis_B = B_worldQuat.GetInversed().RotateVector(worldRotationAxis_CA_BA);

	// the final angles by which we have to rotate the hip and knee joints are just the difference between desired value and current value
	Quat localRotationDiff_A = Quat::CreateQuatByAngleAndAxis((a_AC_AB_desired - a_AC_AB_current), localRotationAxis_A);
	Quat localRotationDiff_B = Quat::CreateQuatByAngleAndAxis((a_BA_BC_desired - a_BA_BC_current), localRotationAxis_B);

	// float fontSize = 24.f;

	if (m_debugMode)
	{
		if (CheckIfInDebugModeAndShouldQuit())
		{
			return;
		}
		if (!m_step1_reached)
		{
			// Multiply this rotation to the right of the hip joint's local rotation(adding the rotation difference we calculated)
			A->m_localRotation = A->m_localRotation * localRotationDiff_A;
			B->m_localRotation = B->m_localRotation * localRotationDiff_B;
			UpdateDebugCounterInDebugMode();
			m_step1_reached = true;
		}
	}
	else
	{
		A->m_localRotation = A->m_localRotation * localRotationDiff_A;
		B->m_localRotation = B->m_localRotation * localRotationDiff_B;
		// A->ContrainJointLocalRotation();
		// B->ContrainJointLocalRotation();
	}

	// EulerAngles ea = ConvertQuatToEulerAngles(A->m_relativeRotationToParent);
	// Log::Write(Log::Level::Info, Stringf("joint rotation: %2.f, %.2f, %.2f", ea.m_yawDegrees, ea.m_pitchDegrees, ea.m_rollDegrees));

	// update ABC's transform information in order
	// A->ContrainJointLocalRotation();
	// B->ContrainJointLocalRotation();

	// update the value
	A_worldPos = A->GetWorldPosition();
	B_worldPos = B->GetWorldPosition();
	C_worldPos = C->GetWorldPosition();
	A_worldQuat = A->GetWorldQuat();
	B_worldQuat = B->GetWorldQuat();
	C_worldQuat = C->GetWorldQuat();

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// step 2: Rotate the Heel(C) into place, we will get the second rotation A need to make
	float a_AC_AT = acosf(DotProduct3D((C_worldPos - A_worldPos).GetNormalized(), (T - A_worldPos).GetNormalized()));	// the result is in radian

	// the rotation axis for ACT
	Vec3 worldRotationAxis_CA_TA = CrossProduct3D(C_worldPos - A_worldPos, T - A_worldPos).GetNormalized(); // todo: ??????? why not this order

	// Vec3 localRotationAxis_A_1 = Quat::RotateVectorByQuat(A_worldQuat.GetInversed(), worldRotationAxis_CA_TA);
	Vec3 localRotationAxis_A_1 = A_worldQuat.GetInversed().RotateVector(worldRotationAxis_CA_TA);
	Quat localRotationDiff_A_1 = Quat::CreateQuatByAngleAndAxis(a_AC_AT, localRotationAxis_A_1);

	if (m_debugMode)
	{
		if (CheckIfInDebugModeAndShouldQuit())
		{
			return;
		}

		if (!m_step2_reached)
		{
			A->m_localRotation = (A->m_localRotation * localRotationDiff_A_1).GetNormalized();
			UpdateDebugCounterInDebugMode();
			m_step2_reached = true;
		}
	}
	else
	{
		A->m_localRotation = (A->m_localRotation * localRotationDiff_A_1).GetNormalized();
		// A->ContrainJointLocalRotation();
	}

	C_worldPos = C->GetWorldPosition();
	m_targetLastFrame = m_target;

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// optional step: use pole vector to adjust the mid joint onto the plane
	if (m_usePoleVector)
	{
		if (m_target == Vec3() || m_poleVecPos == Vec3())
		{
			return;
		}

		// update the value
		A_worldPos = A->GetWorldPosition();
		B_worldPos = B->GetWorldPosition();
		C_worldPos = C->GetWorldPosition();
		A_worldQuat = A->GetWorldQuat();
		B_worldQuat = B->GetWorldQuat();
		C_worldQuat = C->GetWorldQuat();

		Vec3 A_Target = (m_target - A_worldPos).GetNormalized(); // K
		Vec3 A_Pole = (m_poleVecPos - A_worldPos).GetNormalized();
		Vec3 A_Pole_Target_Normal = CrossProduct3D(A_Pole, A_Target).GetNormalized(); // J

		// get direction which is perpendicular to A_Target, A_Pole_Target_Normal
		Vec3 poleDir = CrossProduct3D(A_Pole_Target_Normal, A_Target).GetNormalized(); // I

		Vec3& I_Basis = poleDir;
		// Vec3& J_Basis = A_Pole_Target_Normal;
		Vec3& K_Basis = A_Target;

		// get the angle based on current CAB position
		float l_AC = GetDistance3D(A_worldPos, C_worldPos);
		float cos_CAB = GetClamped((l_AC * l_AC + l_AB * l_AB - l_BC * l_BC) / (2 * l_AC * l_AB), -1.f, 1.f);
		float a_CAB = acosf(cos_CAB);

		// we want to rotate the CAB onto the plane that formed by A-pole-target, so the angle don't change
		// try to get the A-new_B 's projection on I and K
		Vec3 AB_I = I_Basis * (l_AB * sinf(a_CAB));
		Vec3 AB_K = K_Basis * (l_AB * cosf(a_CAB));
		Vec3 B_desiredPos = A_worldPos + AB_I + AB_K;

		//----------------------------------------------------------------------------------------------------------------------------------------------------
		Vec3 AB = B_worldPos - A_worldPos;
		Vec3 AB_desired = B_desiredPos - A_worldPos;
		Vec3 AC = C_worldPos - A_worldPos;
		
		// Project the pole onto the plane perpendicular to AC
		Vec3 AC_planeNormal = AC.GetNormalized();
		Vec3 pole_projected_AC = DotProduct3D(A_Pole, AC_planeNormal) * AC_planeNormal;
		Vec3 pole_projected_AC_plane = (A_Pole - pole_projected_AC).GetNormalized();
		
		// Project current AB vector onto the same plane
		Vec3 AB_projected_AC_plane = (AB - DotProduct3D(AB, AC_planeNormal) * AC_planeNormal).GetNormalized();
		Vec3 AB_desired_projected_AC_plane = (AB_desired - DotProduct3D(AB_desired, AC_planeNormal) * AC_planeNormal).GetNormalized();
		
		// get the rotation difference on plane
		Quat rotationAroundAC = Quat::GetQuatFromTwoVectors(AB_projected_AC_plane, AB_desired_projected_AC_plane); // this introduce unstable result, get negated result

		// Vec3 axis = CrossProduct3D(AB_projected_AC_plane, AB_desired_projected_AC_plane);
		// float angle = acosf(DotProduct3D(AB_projected_AC_plane, AB_desired_projected_AC_plane));
		// 
		// if (DotProduct3D(axis, AC_planeNormal) < 0.f)
		// {angle = -angle;}
		// Quat rotationAroundAC = Quat::CreateQuatByAngleAndAxis(angle, AC_planeNormal);

		// Convert world rotation to local (around joint A's local space)
		Quat localRotation = A_worldQuat.ConvertWorldRotationToLocal(rotationAroundAC);
		Quat A_newLocalRotation = (A->m_localRotation * localRotation).GetNormalized();
		
		if (m_debugMode)
		{
			if (CheckIfInDebugModeAndShouldQuit())
			{
				return;
			}
		
			if (!m_step3_reached)
			{
				// from world rotation, get local rotation of A
				A->m_localRotation = A_newLocalRotation;

				m_step1_reached = false;
				m_step2_reached = false;
				m_step3_reached = false;
				UpdateDebugCounterInDebugMode();
			}
		}
		else
		{
		
			// Apply rotation to A's local rotation
			A->m_localRotation = A_newLocalRotation;
		}
	}
}

void TwoJointsIKSolver::SolveIK_oldVersion()
{
	// if (!m_usePoleVector)
	// {
	// 	if (GetDistanceSquared3D(m_targetLastFrame, m_target) <= 0.0001f)
	// 	{
	// 		return;
	// 	}
	// }
	// else
	// {
	// 	if (GetDistanceSquared3D(m_targetLastFrame, m_target) <= 0.0001f && GetDistanceSquared3D(m_poleVectorLastFrame, m_poleVecPos) <= 0.0001f)
	// 	{
	// 		return;
	// 	}
	// }

	// Step 1: Extend/Contract the Joint Chain
	// we will get a rotation for A and another rotation for B
	// if the extend or contract is almost done, do not do it again.
	Joint*& A = m_root;
	Joint*& B = m_mid;
	Joint*& C = m_end;
	Vec3 T = m_target;

	float eps = 0.01f;
	Vec3 A_worldPos = A->GetWorldPosition();
	Vec3 B_worldPos = B->GetWorldPosition();
	Vec3 C_worldPos = C->GetWorldPosition();

	float l_AB = GetDistance3D(B_worldPos, A_worldPos);
	float l_BC = GetDistance3D(B_worldPos, C_worldPos);
	float l_AT = GetClamped(GetDistance3D(T, A_worldPos), eps, (l_AB + l_BC - eps));

	// get the current interior angles of the hip and knee joints
	float a_AC_AB_current = acosf(GetClamped(DotProduct3D(
		(C_worldPos - A_worldPos).GetNormalized(),
		(B_worldPos - A_worldPos).GetNormalized()), -1.f, 1.f));

	float a_BA_BC_current = acosf(GetClamped(DotProduct3D(
		(A_worldPos - B_worldPos).GetNormalized(),
		(C_worldPos - B_worldPos).GetNormalized()), -1.f, 1.f));

	// Use The cosine rule to get the desired interior angles of these joints
	float a_AC_AB_desired = acosf(GetClamped((l_AB * l_AB + l_AT * l_AT - l_BC * l_BC) / (2.f * l_AB * l_AT)
		, -1.f, 1.f));
	float a_BA_BC_desired = acosf(GetClamped((l_AB * l_AB + l_BC * l_BC - l_AT * l_AT) / (2.f * l_AB * l_BC)
		, -1.f, 1.f));

	// the rotation axis for ABC
	Vec3 worldRotationAxis_CA_BA;
	if (CheckIfTwoVec3AreInTheSameDirection(C_worldPos - A_worldPos, B_worldPos - A_worldPos))
	{
		Mat44 A_transformMat = A->GetWorldTransformMat();
		Vec3 axis = A_transformMat.GetJBasis3D();
		worldRotationAxis_CA_BA = CrossProduct3D(C_worldPos - A_worldPos, axis).GetNormalized();
	}
	else
	{
		worldRotationAxis_CA_BA = CrossProduct3D(C_worldPos - A_worldPos, B_worldPos - A_worldPos).GetNormalized(); // todo: it will unstable when fully extended?
	}

	// Since we are working in the world space we have to first transform this axis into the local space of each joint (since the rotations will be applied locally)
	// this is achieved by applying the inverse of world rotation of the joint to the axis
	Quat A_worldQuat = A->GetWorldQuat();
	Quat B_worldQuat = B->GetWorldQuat();
	Quat C_worldQuat = C->GetWorldQuat();

	// Vec3 localRotationAxis_A = Quat::RotateVectorByQuat(B_worldQuat.GetInversed(), worldRotationAxis_CA_BA);
	// Vec3 localRotationAxis_B = Quat::RotateVectorByQuat(C_worldQuat.GetInversed(), worldRotationAxis_CA_BA);	

	Vec3 localRotationAxis_A = A_worldQuat.GetInversed().RotateVector(worldRotationAxis_CA_BA);
	Vec3 localRotationAxis_B = B_worldQuat.GetInversed().RotateVector(worldRotationAxis_CA_BA);

	// the final angles by which we have to rotate the hip and knee joints are just the difference between desired value and current value
	Quat localRotationDiff_A = Quat::CreateQuatByAngleAndAxis((a_AC_AB_desired - a_AC_AB_current), localRotationAxis_A);
	Quat localRotationDiff_B = Quat::CreateQuatByAngleAndAxis((a_BA_BC_desired - a_BA_BC_current), localRotationAxis_B);

	// float fontSize = 24.f;

	if (m_debugMode)
	{
		if (CheckIfInDebugModeAndShouldQuit())
		{
			return;
		}
		if (!m_step1_reached)
		{
			// Multiply this rotation to the right of the hip joint's local rotation(adding the rotation difference we calculated)
			A->m_localRotation = A->m_localRotation * localRotationDiff_A;
			B->m_localRotation = B->m_localRotation * localRotationDiff_B;
			UpdateDebugCounterInDebugMode();
			m_step1_reached = true;
		}
	}
	else
	{
		A->m_localRotation = A->m_localRotation * localRotationDiff_A;
		B->m_localRotation = B->m_localRotation * localRotationDiff_B;
		// A->ContrainJointLocalRotation();
		// B->ContrainJointLocalRotation();
	}

	// EulerAngles ea = ConvertQuatToEulerAngles(A->m_relativeRotationToParent);
	// Log::Write(Log::Level::Info, Stringf("joint rotation: %2.f, %.2f, %.2f", ea.m_yawDegrees, ea.m_pitchDegrees, ea.m_rollDegrees));

	// update ABC's transform information in order
	// A->ContrainJointLocalRotation();
	// B->ContrainJointLocalRotation();

	// update the value
	A_worldPos = A->GetWorldPosition();
	B_worldPos = B->GetWorldPosition();
	C_worldPos = C->GetWorldPosition();
	A_worldQuat = A->GetWorldQuat();
	B_worldQuat = B->GetWorldQuat();
	C_worldQuat = C->GetWorldQuat();

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// step 2: Rotate the Heel(C) into place, we will get the second rotation A need to make
	float a_AC_AT = acosf(DotProduct3D((C_worldPos - A_worldPos).GetNormalized(), (T - A_worldPos).GetNormalized()));	// the result is in radian

	// the rotation axis for ACT
	Vec3 worldRotationAxis_CA_TA = CrossProduct3D(C_worldPos - A_worldPos, T - A_worldPos).GetNormalized(); // todo: ??????? why not this order

	// Vec3 localRotationAxis_A_1 = Quat::RotateVectorByQuat(A_worldQuat.GetInversed(), worldRotationAxis_CA_TA);
	Vec3 localRotationAxis_A_1 = A_worldQuat.GetInversed().RotateVector(worldRotationAxis_CA_TA);
	Quat localRotationDiff_A_1 = Quat::CreateQuatByAngleAndAxis(a_AC_AT, localRotationAxis_A_1);

	if (m_debugMode)
	{
		if (CheckIfInDebugModeAndShouldQuit())
		{
			return;
		}

		if (!m_step2_reached)
		{
			A->m_localRotation = (A->m_localRotation * localRotationDiff_A_1).GetNormalized();
			UpdateDebugCounterInDebugMode();
			m_step2_reached = true;
		}
	}
	else
	{
		A->m_localRotation = (A->m_localRotation * localRotationDiff_A_1).GetNormalized();
		// A->ContrainJointLocalRotation();
	}

	C_worldPos = C->GetWorldPosition();
	m_targetLastFrame = m_target;

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// optional step: use pole vector to adjust the mid joint onto the plane
	if (m_usePoleVector)
	{
		if (m_target == Vec3() || m_poleVecPos == Vec3())
		{
			return;
		}

		// update the value
		A_worldPos = A->GetWorldPosition();
		B_worldPos = B->GetWorldPosition();
		C_worldPos = C->GetWorldPosition();
		A_worldQuat = A->GetWorldQuat();
		B_worldQuat = B->GetWorldQuat();
		C_worldQuat = C->GetWorldQuat();

		Vec3 A_Target = (m_target - A_worldPos).GetNormalized(); // K
		Vec3 A_Pole = (m_poleVecPos - A_worldPos).GetNormalized();
		Vec3 A_Pole_Target_Normal = CrossProduct3D(A_Pole, A_Target).GetNormalized(); // J

		// get direction which is perpendicular to A_Target, A_Pole_Target_Normal
		Vec3 poleDir = CrossProduct3D(A_Pole_Target_Normal, A_Target).GetNormalized(); // I

		Vec3& I_Basis = poleDir;
		// Vec3& J_Basis = A_Pole_Target_Normal;
		Vec3& K_Basis = A_Target;

		// get the angle based on current CAB position
		float l_AC = GetDistance3D(A_worldPos, C_worldPos);
		float cos_CAB = GetClamped((l_AC * l_AC + l_AB * l_AB - l_BC * l_BC) / (2 * l_AC * l_AB), -1.f, 1.f);
		float a_CAB = acosf(cos_CAB);

		// we want to rotate the CAB onto the plane that formed by A-pole-target, so the angle don't change
		// try to get the A-new_B 's projection on I and K
		Vec3 AB_I = I_Basis * (l_AB * sinf(a_CAB));
		Vec3 AB_K = K_Basis * (l_AB * cosf(a_CAB));
		Vec3 B_desiredPos = A_worldPos + AB_I + AB_K;

		// // we need to update A's local rotation difference to rotate B on the plane
		Vec3 AB_currentDir = (B_worldPos - A_worldPos).GetNormalized();
		Vec3 BC_currentDir = (C_worldPos - B_worldPos).GetNormalized();
		Vec3 AB_desiredDir = (B_desiredPos - A_worldPos).GetNormalized();

		// calculate the quat world rotation to align AB to A-desiredB
		Quat A_worldRotationDiff = Quat::GetQuatFromTwoVectors(AB_currentDir, AB_desiredDir);
		Quat A_worldRotation = A_worldRotationDiff * A_worldQuat; // because of the convention in post-multiplication order. transformations applied on the right are the ones that happen first.

		//----------------------------------------------------------------------------------------------------------------------------------------------------
		if(m_debugMode)
		{
			if (CheckIfInDebugModeAndShouldQuit())
			{
				return;
			}
		
			if (!m_step3_reached)
			{
				// from world rotation, get local rotation of A
				A->m_localRotation = A->m_parent->GetWorldQuat().GetInversed() * A_worldRotation;
				m_step3_reached = true;
				// A->ContrainJointLocalRotation();
				UpdateDebugCounterInDebugMode();
			}
		}
		else
		{
			A->m_localRotation = A->m_parent->GetWorldQuat().GetInversed() * A_worldRotation;
			// A->ContrainJointLocalRotation();
		}
		
		//----------------------------------------------------------------------------------------------------------------------------------------------------
		// A is settled
		
		// update the value
		A_worldPos = A->GetWorldPosition();
		B_worldPos = B->GetWorldPosition();
		C_worldPos = C->GetWorldPosition();
		A_worldQuat = A->GetWorldQuat();
		B_worldQuat = B->GetWorldQuat();
		C_worldQuat = C->GetWorldQuat();
		
		// we also need to update B's local rotation to aim B's bone at the target 
		Vec3 BC_desiredDirInWorld = (m_target - B_worldPos).GetNormalized();
		
		// convert the direction into jointB's local space
		// B's local space rotation is determinated by joint A
		Quat invJointAWorldRotation = A_worldQuat.GetInversed();
		Vec3 BC_desiredDirInBLocal = Quat::RotateVectorByQuat(invJointAWorldRotation, BC_desiredDirInWorld);
		
		if (m_debugMode)
		{
			if (CheckIfInDebugModeAndShouldQuit())
			{
				return;
			}
		
			// get B's current local rotation direction
			// Vec3 BC_currentDirInBLocal = Quat::RotateVectorByQuat(B->m_localRotation, Vec3(1.f, 0.f, 0.f));
			B->m_localRotation = Quat::GetQuatFromTwoVectors(Vec3(1.f, 0.f, 0.f), BC_desiredDirInBLocal);
			// B->ContrainJointLocalRotation();
			m_step1_reached = false;
			m_step2_reached = false;
			m_step3_reached = false;
			UpdateDebugCounterInDebugMode();
		}
		else
		{
			B->m_localRotation = Quat::GetQuatFromTwoVectors(Vec3(1.f, 0.f, 0.f), BC_desiredDirInBLocal);
		}
		
		EulerAngles angle = ConvertQuatToEulerAngles(A->m_localRotation); 
		// Log::Write(Log::Level::Info, Stringf("joint rotation: %2.f, %.2f, %.2f", angle.m_yawDegrees, angle.m_pitchDegrees, angle.m_rollDegrees));
		
		A->ContrainJointLocalRotation();
		B->ContrainJointLocalRotation();
		
		C_worldPos = C->GetWorldPosition();
		
		m_targetLastFrame = m_target;
		m_poleVectorLastFrame = m_poleVecPos;
	}
}

void TwoJointsIKSolver::SolveIKWithWeight(float weight)
{
	Quat rootRotation = m_root->m_localRotation;
	Quat midRotation = m_mid->m_localRotation;
	Quat endRotation = m_end->m_localRotation;

	SolveIK();
	m_root->m_localRotation = Quat::Nlerp(rootRotation, m_root->m_localRotation, weight);
	m_mid->m_localRotation = Quat::Nlerp(midRotation, m_mid->m_localRotation, weight);
	m_end->m_localRotation = Quat::Nlerp(endRotation, m_end->m_localRotation, weight);
}

void TwoJointsIKSolver::SolveFK()
{
	// m_root->m_worldRotation = m_root->m_skeleton->m_worldRotation * m_root->m_localRotation;
	// m_root->m_worldPos = m_root->m_skeleton->m_worldPos;  // Root joint's position is the same as skeleton's actor's position
	

	// Update all child joints
	// for (Joint* child : m_root->m_children)
	// {
	// 	child->Update();
	// }
}

void TwoJointsIKSolver::SetSolverTarget(Vec3 const& pos)
{
	m_target = pos;
}

void TwoJointsIKSolver::SetPoleVector(Vec3 const& pos)
{
	m_poleVecPos = pos;
}

bool TwoJointsIKSolver::CheckIfInDebugModeAndShouldQuit()
{
	// debug settings
	if (m_debugMode)
	{
		if (m_debugCounter == 0)			// only go forward if user adding a command to go forward
		{
			return true;				// if there is no command allowance, end the solver
		}
	}

	return false;
}

void TwoJointsIKSolver::UpdateDebugCounterInDebugMode()
{
	if (m_debugMode)		// setting the local rotation will cost a debug allowance
	{
		m_debugCounter -= 1;
	}
}

