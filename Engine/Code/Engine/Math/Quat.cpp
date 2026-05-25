#include "Engine/Math/Quat.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/core/EngineCommon.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "ThirdParty/OpenXR/include/openxr/openxr.h"
#include "ThirdParty/Physx/include/PxPhysicsAPI.h"
#include "Engine/Math/Easing.hpp"
#include <cmath>

constexpr float epsilon = 0.001f;

Quat::Quat(XrQuaternionf const& xrQuat)
	: x(xrQuat.x)
	, y(xrQuat.y)
	, z(xrQuat.z)
	, w(xrQuat.w)
{
	// Normalize();
}

Quat::Quat(EulerAngles const& angle)
{
	float cy = cosf(ConvertDegreesToRadians(angle.m_yawDegrees) * 0.5f);
	float sy = sinf(ConvertDegreesToRadians(angle.m_yawDegrees) * 0.5f);
	float cp = cosf(ConvertDegreesToRadians(angle.m_pitchDegrees) * 0.5f);
	float sp = sinf(ConvertDegreesToRadians(angle.m_pitchDegrees) * 0.5f);
	float cr = cosf(ConvertDegreesToRadians(angle.m_rollDegrees) * 0.5f);
	float sr = sinf(ConvertDegreesToRadians(angle.m_rollDegrees) * 0.5f);

	Quat q;
	q.w = cr * cp * cy + sr * sp * sy;
	q.x = sr * cp * cy - cr * sp * sy;
	q.y = cr * sp * cy + sr * cp * sy;
	q.z = cr * cp * sy - sr * sp * cy;

	*this = q.GetNormalized();
}

Quat::Quat(physx::PxQuat const& pxQuat)
	: x(pxQuat.x)
	, y(pxQuat.y)
	, z(pxQuat.z)
	, w(pxQuat.w)
{
}

Quat::Quat(Vec3 axisNormal, float radians)
{
	float sinHalfAngle = sinf(radians * 0.5f);
	float cosHalfAngle = cosf(radians * 0.5f);

	x = axisNormal.x * sinHalfAngle;
	y = axisNormal.y * sinHalfAngle;
	z = axisNormal.z * sinHalfAngle;
	w = cosHalfAngle;
}

// the sign and unsign is caused by i*i = -1, j*j = -1, k*k = -1
// also, i*j = k, j*k = i, K*I = j, 
// j*i = -k, k*j = -i, i*k= -j,
Quat Quat::operator*(const Quat& q) const
{
	Quat result(
		w * q.x + x * q.w + y * q.z - z * q.y,
		w * q.y - x * q.z + y * q.w + z * q.x,
		w * q.z + x * q.y - y * q.x + z * q.w,
		w * q.w - x * q.x - y * q.y - z * q.z
	);
	return result;
}

Vec3 Quat::operator*(Vec3 const& v) const
{
	// Quaternion components
	float qx = x;
	float qy = y;
	float qz = z;
	float qw = w;

	// Calculate quat * vec (as quaternion multiplication)
	float ix = qw * v.x + qy * v.z - qz * v.y;
	float iy = qw * v.y + qz * v.x - qx * v.z;
	float iz = qw * v.z + qx * v.y - qy * v.x;
	float iw = -qx * v.x - qy * v.y - qz * v.z;

	// Calculate result * inverse quat
	return Vec3(
		ix * qw + iw * -qx + iy * -qz - iz * -qy,
		iy * qw + iw * -qy + iz * -qx - ix * -qz,
		iz * qw + iw * -qz + ix * -qy - iy * -qx
	);
}

// if all unit is normalized, no need to normalize
void Quat::operator*=(Quat const& q)
{
	*this = Quat(
		w * q.x + x * q.w + y * q.z - z * q.y,
		w * q.y - x * q.z + y * q.w + z * q.x,
		w * q.z + x * q.y - y * q.x + z * q.w,
		w * q.w - x * q.x - y * q.y - z * q.z
	);
}

bool Quat::operator!=(Quat const& compare) const
{
	return(x <= (compare.x - epsilon) || x >= (compare.x + epsilon) ||
			y <= (compare.y - epsilon) || y >= (compare.y + epsilon) ||
			z <= (compare.z - epsilon) || z >= (compare.z + epsilon) ||
			w <= (compare.x - epsilon) || w >= (compare.w + epsilon));
}

bool Quat::IsMostEqual(Quat const& compare, float acceptanceRange /*= 0.01f*/)
{
	bool directMatch =
		fabsf(x - compare.x) <= acceptanceRange &&
		fabsf(y - compare.y) <= acceptanceRange &&
		fabsf(z - compare.z) <= acceptanceRange &&
		fabsf(w - compare.w) <= acceptanceRange;

	bool negatedMatch =
		fabsf(x + compare.x) <= acceptanceRange &&
		fabsf(y + compare.y) <= acceptanceRange &&
		fabsf(z + compare.z) <= acceptanceRange &&
		fabsf(w + compare.w) <= acceptanceRange;

	return directMatch || negatedMatch;
}

void Quat::operator=(Quat const& q)
{
	x = q.x;
	y = q.y;
	z = q.z;
	w = q.w;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
// because sin(theta/2) ^ 2 + cos(theta/2) ^ 2 = 1,
// here is doing (theta/2) in 3D, because (1,0,0,0) is 180 around x, (0,1,0,0) is 180 around y, (0,0,1,0) is 180 around z
// but in 2D, (0,1) is 90 counter clockwise, (-1,0) is -180 counter clockwise, (0,-1) is 90 clockwise
// these three function do not need to be normalized
Quat Quat::CreateRotationAroundXAxis(float degrees)
{
	float radians = degrees * (PI / 180.0f);  // Convert degrees to radians
	float halfAngle = radians / 2.0f;
	float sinHalfAngle = sin(halfAngle);

	return Quat(sinHalfAngle, 0.0f, 0.0f, cos(halfAngle));
}

Quat Quat::CreateRotationAroundYAxis(float degrees)
{
	float radians = degrees * (PI / 180.0f);  // Convert degrees to radians
	float halfAngle = radians / 2.0f;
	float sinHalfAngle = sin(halfAngle);

	return Quat(0.0f, sinHalfAngle, 0.0f, cos(halfAngle));
}

Quat Quat::CreateRotationAroundZAxis(float degrees)
{
	float radians = degrees * (PI / 180.0f);  // Convert degrees to radians
	float halfAngle = radians / 2.0f;
	float sinHalfAngle = sin(halfAngle);

	return Quat(0.0f, 0.0f, sinHalfAngle, cos(halfAngle));
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
Quat Quat::GetQuatFor180DegreesRotation(Vec3 const& v)
{
	Vec3 rotationAxis = abs(v.x) > 0.1f ? Vec3(0.0f, 1.0f, 0.0f) : Vec3(1.0f, 0.0f, 0.0f);
	rotationAxis = CrossProduct3D(rotationAxis, v).GetNormalized();

	return Quat(rotationAxis.x, rotationAxis.y, rotationAxis.z, 0.0f); // because sin(theta/2) = 1.f
}

// the quat must be normalized already
// inverse the real number
Quat Quat::GetInversed() const
{
	return Quat(-x, -y, -z, w);
}

// inverse the real number w and the complex x,y,z
Quat Quat::GetNegated() const
{
	return Quat(-x, -y, -z, -w);
}

Quat Quat::GetNormalized() const
{
	float magnitude = sqrt(x * x + y * y + z * z + w * w);

	if (magnitude > 0.f)
	{
		float invMag = 1.0f / magnitude;
		return Quat(x * invMag, y * invMag, z * invMag, w * invMag);
	}
	else
	{
		return Quat(0.f, 0.f, 0.f, 1.0f);
	}
}

// the pass in axis must be normalized
Quat Quat::CreateQuatByAngleAndAxis(float angle, Vec3 axisNormal)
{
	float half_angle = angle * 0.5f;
	float s = sinf(half_angle);
	Quat result(axisNormal.x * s, axisNormal.y * s, axisNormal.z * s, cos(half_angle));
	return result.GetNormalized();
}

// the pass in quat must be normalized, Vec3 does not need to be
Vec3 Quat::RotateVectorByQuat(Quat const& unitQuat, Vec3 in_Vec)
{
	Vec3 unitVector = in_Vec.GetNormalized();

	// Convert the vector into a pure quaternion
	Quat vecQuat(unitVector.x, unitVector.y, unitVector.z, 0.f);	// make sure this is a unit quat

	// Perform the rotation: q * vec_q * q_inverse
	Quat quat_inv = unitQuat.GetInversed();
	Quat rotated_q = unitQuat * vecQuat * quat_inv;		// the sandwich 

	// Extract the rotated vector from the quat
	return Vec3(rotated_q.x,
		rotated_q.y,
		rotated_q.z) * in_Vec.GetLength();
}

// no need to pass in normalized value, works for disp and position, velocity
Vec3 Quat::RotateVector(Vec3 const& v)
{
	// Convert vector to a pure quaternion (w = 0)
	Quat vectorQuat(v.x, v.y, v.z, 0.f);

	// Perform the rotation: q * v * q^-1
	Quat result = (*this) * vectorQuat * this->GetInversed();

	// The rotated vector is stored in the x, y, z components of the result
	return Vec3(result.x, result.y, result.z);
}

// the pass in vector no need to be normalized
Quat Quat::GetQuatFromTwoVectors(Vec3 const& v1, Vec3 const& v2)
{
	Vec3 from = v1.GetNormalized();
	Vec3 to = v2.GetNormalized();

	float cosTheta = DotProduct3D(from, to);
	Vec3 rotationAxis;

	// If the vectors are nearly identical
	if (cosTheta > 0.999999f)
	{
		return Quat(0.0f, 0.0f, 0.0f, 1.0f);  // No rotation needed
	}

	// If the vectors are opposite
	// choose an orthogonal axis and return a 180-degree rotation around that axis.
	else if (cosTheta < -0.999999f)
	{
		return Quat::GetQuatFor180DegreesRotation(from);
	}

	// Otherwise, calculate the quaternion
	else
	{
		rotationAxis = CrossProduct3D(from, to);
		float s = sqrt((1.0f + cosTheta) * 2.0f);
		float invS = 1.f / s;

		Quat q(
			rotationAxis.x * invS,
			rotationAxis.y * invS,
			rotationAxis.z * invS,
			s * 0.5f
		);
		return q.GetNormalized();
	}
}

Quat Quat::ClampQuaternion(EulerAngles const& min, EulerAngles const& max) const
{
	// convert quaternion to euler angles
	EulerAngles euler = ConvertQuatToEulerAngles(*this);

	// clamp on yaw, pitch, roll
	if (min.m_yawDegrees != -180.f || max.m_yawDegrees != 180.f)
	{
		euler.m_yawDegrees = GetClamped(euler.m_yawDegrees, min.m_yawDegrees, max.m_yawDegrees);
	}
	if (min.m_pitchDegrees != -180.f || max.m_pitchDegrees != 180.f)
	{
		euler.m_pitchDegrees = GetClamped(euler.m_pitchDegrees, min.m_pitchDegrees, max.m_pitchDegrees);
	}
	if (min.m_rollDegrees != -180.f || max.m_rollDegrees != 180.f)
	{
		euler.m_rollDegrees = GetClamped(euler.m_rollDegrees, min.m_rollDegrees, max.m_rollDegrees);
	}

	// convert back to quat
	Quat quat = ConvertEulerAnglesToQuat(euler);

	return quat;
}

// both quat should be normalized before input
Quat Quat::Slerp(Quat const& q1, Quat const& q2, float t)
{
	// Compute the dot product
	float dot = DotProductQuat(q1, q2);

	// If the dot product is negative, reverse one quaternion to take the shortest path
	Quat q2_shortest = q2;
	if (dot < 0.f) {
		q2_shortest = q2.GetNegated();
		dot = -dot;
	}

	// Compute the angle between the quaternions
	float theta = acos(dot);
	float sinTheta = sin(theta);

	// Compute interpolation factors
	float w1 = sin((1.f - t) * theta) / sinTheta;
	float w2 = sin(t * theta) / sinTheta;

	// Compute the SLERP result
	return Quat(
		w1 * q1.x + w2 * q2_shortest.x,
		w1 * q1.y + w2 * q2_shortest.y,
		w1 * q1.z + w2 * q2_shortest.z,
		w1 * q1.w + w2 * q2_shortest.w
	);
}

// the pass in values must be normalized
Quat Quat::Nlerp(Quat const& q1, Quat const& q2, float t)
{
	// Compute the dot product
	float dot = DotProductQuat(q1, q2);
	if (dot < -0.99f)	// if this two quat is nearly the opposite, use  
	{
		return Slerp(q1, q2, t);
	}

	// If the dot product is negative, reverse one quaternion to take the shortest path
	Quat q2_shortest = q2;
	if (dot < 0.f) {
		q2_shortest = q2_shortest.GetNegated();
	}

	// because the interpolation is linear
	// we need to compensate the angular speed by changing t
	// Jonathan Blow solution: http://number-none.com/product/Hacking%20Quaternions/
	// float k = (t > 0.5f) ? 0.5855064f : 0.5069269f;
	// float newT = (2.f * k * t * t) - (3.f * k * t) + k + 1.f;
	// 
	// float t_hesitate = Hesitate3(0.f, 1.f, t);	 
	// float newT = Interpolate(t, t_hesitate, 0.2f);

	// check the graph on the https://www.desmos.com/calculator
	float t_hesitate = (t - 0.5f) * (t - 0.5f) * (t - 0.5f) + 0.5f;
	float weight = 0.f;
	float diff = abs(t - 0.5f);
	weight = RangeMapClamped(diff, 0.f, 0.5f, 0.2f, 0.f);
	float newT = Interpolate(t, t_hesitate, weight);

	Quat result = Quat(
		q1.x + newT * (q2_shortest.x - q1.x),
		q1.y + newT * (q2_shortest.y - q1.y),
		q1.z + newT * (q2_shortest.z - q1.z),
		q1.w + newT * (q2_shortest.w - q1.w)
	);
	return result.GetNormalized();
}

void Quat::Normalize()
{
	*this = GetNormalized();
}

void Quat::GetRotationAxisAndAngle(Vec3& axis, float& angle)
{
	// Given any quaternion you can extract the rotation axis and rotation angle for the shortest path as follows
	// because for q(x, y, z, w), (x, y, z) = axis * sin(Θ / 2), w = cos(Θ / 2)
	angle = 2.f * acos(w);		// 
	float sinHalfAngle = sqrt(1.f - w * w);

	if (sinHalfAngle != 0.f)
	{
		if (angle > PI)
		{
			angle = PI * 2.f - angle;
			float oneDivideBySinHalfAngle = 1.f / sinHalfAngle;
			axis = Vec3(-x * oneDivideBySinHalfAngle,
						-y * oneDivideBySinHalfAngle,
						-z * oneDivideBySinHalfAngle);
		}
		else
		{
			float oneDivideBySinHalfAngle = 1.f / sinHalfAngle;
			axis = Vec3(x * oneDivideBySinHalfAngle,
						y * oneDivideBySinHalfAngle,
						z * oneDivideBySinHalfAngle);	// after math proving, this is normalized
		}
	}
	else
	{
		axis = Vec3(1.f, 0.f, 0.f);
	}
}

bool Quat::operator==(Quat const& q) const
{
	return(x == q.x && y == q.y && z == q.z && w == q.w);
}

Quat Quat::ConvertDifferentHandSystem() const
{
	return Quat(x, -y, -z, w); // Negate Z-axis
}

Quat Quat::ConvertWorldRotationToLocal(Quat const& q) const
{
	Quat thisQuat(*this);
	return  ((thisQuat.GetInversed() * q).GetNormalized() * thisQuat).GetNormalized();
}
