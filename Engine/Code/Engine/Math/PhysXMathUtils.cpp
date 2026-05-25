#include "Engine/Math/PhysXMathUtils.hpp"
#include "ThirdParty/Physx/include/PxPhysicsAPI.h"

using namespace physx;

physx::PxVec3 GetPxVec3(Vec3 const& v)
{
	physx::PxVec3 result;
	result.x = v.x;
	result.y = v.y;
	result.z = v.z;
	return result;
}

physx::PxQuat GetPxQuat(Quat const& q)
{
	physx::PxQuat pxQ;
	pxQ.x = q.x;
	pxQ.y = q.y;
	pxQ.z = q.z;
	pxQ.w = q.w;
	return pxQ;
}

// physX is using the same axis direction as DirectX
// it is left-handed system and X-right, Y-up, Z-forward
Mat44 GetGameToPhysXSpaceMat()
{
	return Mat44(Mat44(Vec3(0.f, 0.f, 1.f), Vec3(-1.f, 0.f, 0.f), Vec3(0.f, 1.f, 0.f), Vec3()));
}

Mat44 GetPhysXToGameSpaceMat()
{
	// 	return Mat44(Vec3(0.f, -1.f, 0.f), Vec3(0.f, 0.f, 1.f), Vec3(1.f, 0.f, 0.f), Vec3());
	Mat44 result = GetGameToPhysXSpaceMat();
	result.Transpose();
	return result;
}

physx::PxTransform MakePhysxTransform(Vec3 p, Quat q)
{
	PxTransform pose;
	pose.p = GetPxVec3(p);
	pose.q = GetPxQuat(q);
	return pose;
}

Quat TransformQuatFromGameToPhysXAndGetInversed(Quat const& q)
{
	return Quat(q.y, -q.z, -q.x, q.w);
	// return Quat(-q.y, q.z, q.x, q.w);
}

Quat TransformQuatFromPhysXToGameAndGetInversed(Quat const& q)
{
	return Quat(-q.z, q.x, -q.y, q.w);
	// return Quat(q.z, -q.x, q.y, q.w);
}

Quat TransformQuatFromGameToPhysX(Quat const& q)
{
	return Quat(-q.y, q.z, q.x, q.w);
}

Quat TransformQuatFromPhysXToGame(Quat const& q)
{
	return Quat(q.z, -q.x, q.y, q.w);
}

Quat SetD6JointDriveAlignWithJointOrientation(Joint* skeletonJoint, Quat const& rotationAdjustMent /*= Quat()*/)
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
	Quat D6JointWorldRotation = (actor0_worldOrientation * D6Joint_localOrientation).GetNormalized();

	// calculate the local rotation the D6 joint needs to make
	desiredJointOrientation *= rotationAdjustMent; 
	Quat driveRotation = (D6JointWorldRotation.GetInversed() * desiredJointOrientation).GetNormalized();

	// do not change the D6 joint's location but update its orientation
	PxTransform t;
	t.q = GetPxQuat(TransformQuatFromGameToPhysXAndGetInversed(driveRotation));
	t.p = D6Joint->getDrivePosition().p;
	D6Joint->setDrivePosition(t);

	// get the joint's world rotation by getting the orientation of the dynamic actor's that it's controlling
	return TransformQuatFromPhysXToGameAndGetInversed(Quat(actor1->getGlobalPose().q)) * rotationAdjustMent.GetInversed();

}
