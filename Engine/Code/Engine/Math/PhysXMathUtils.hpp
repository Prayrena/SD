#pragma once
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Animation/Skeleton.hpp"

//----------------------------------------------------------------------------------------------------------------------------------------------------
// physX stuff
namespace physx {
	template <typename T>
	class PxVec3T;					// Forward declaration of template
	typedef PxVec3T<float> PxVec3;	// Forward declaration of PxVec3

	template <typename T> class PxTransformT; // Forward declaration of template
	using PxTransform = PxTransformT<float>;   // Define PxTransform as PxTransformT<float>
}

namespace physx {
	template <typename T>
	class PxQuatT; // Forward declare the template class
	typedef PxQuatT<float> PxQuat; // Declare the typedef
}

physx::PxVec3 GetPxVec3(Vec3 const& v);
physx::PxQuat GetPxQuat(Quat const& q);

Mat44 GetGameToPhysXSpaceMat();
Mat44 GetPhysXToGameSpaceMat();

physx::PxTransform MakePhysxTransform(Vec3 p, Quat q);

Quat TransformQuatFromGameToPhysXAndGetInversed(Quat const& q);
Quat TransformQuatFromPhysXToGameAndGetInversed(Quat const& q); 

Quat TransformQuatFromGameToPhysX(Quat const& q);
Quat TransformQuatFromPhysXToGame(Quat const& q);

Quat SetD6JointDriveAlignWithJointOrientation(Joint* skeletonJoint, Quat const& rotationAdjustMent = Quat());

