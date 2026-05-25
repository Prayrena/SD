#pragma once
#include "Engine/math/Vec3.hpp"

struct EulerAngles;
struct XrQuaternionf;

namespace physx {
	template <typename T>
	class PxQuatT; // Forward declare the template class
	typedef PxQuatT<float> PxQuat; // Declare the typedef
}

class Quat // unit Quat
{
public:
	float x, y, z, w;

	// must be normalized
	Quat(float x = 0.f, float y = 0.f, float z = 0.f, float w = 1.f)
		: x(x), y(y), z(z), w(w) {}
	explicit Quat(XrQuaternionf const& xrQuat);
	explicit Quat(physx::PxQuat const& pxQuat);
	explicit Quat(EulerAngles const& orientation);
	explicit Quat(Vec3 axisNormal, float radians);

	// Quaternion multiplication
	// Right-to-left multiplication order: In quaternion math, 
	// if you want to apply the rotation represented by B to the current orientation A
	// you multiply them as A * B.
	Quat operator*(Quat const& q) const;
	Vec3 operator*(Vec3 const& v) const;
	bool operator !=(Quat const& q) const;
	bool operator ==(Quat const& q) const;

	void operator*=(Quat const& q);
	void operator =(Quat const& q);

	// Quat creation
	static Quat CreateRotationAroundXAxis(float degrees);
	static Quat CreateRotationAroundYAxis(float degrees);
	static Quat CreateRotationAroundZAxis(float degrees);

	static Quat GetQuatFor180DegreesRotation(Vec3 const& v);

	// Quaternion inversion
	Quat GetInversed() const;
	Quat GetNegated() const;

	Quat GetNormalized() const;
	void Normalize();

	// converting a world-space rotation into local space relative to this joint
	Quat ConvertWorldRotationToLocal(Quat const& q) const;

	// creates a quaternion representing a rotation of a certain angle around a specified axis.
	static Quat CreateQuatByAngleAndAxis(float angle, Vec3 axis);

	// apply the rotation to the vector
	static Vec3 RotateVectorByQuat(Quat const& quat, Vec3 in_Vec); // this can be wrong
	Vec3 RotateVector(Vec3 const& v);

	// Get the quaternion between two vector
	static Quat GetQuatFromTwoVectors(Vec3 const& v1, Vec3 const& v2);

	Quat ClampQuaternion(EulerAngles const& min, EulerAngles const& max) const;

	void GetRotationAxisAndAngle(Vec3& axis, float& angle);

	// Slerp ensures smooth interpolation between rotations and handles quaternions correctly, including when they are not close together.
	static Quat Slerp(Quat const& q1, Quat const& q2, float t);
	static Quat Nlerp(Quat const& q1, Quat const& q2, float t);

	Quat ConvertDifferentHandSystem() const;
	bool IsMostEqual(Quat const& q, float acceptanceRange = 0.01f);
};
