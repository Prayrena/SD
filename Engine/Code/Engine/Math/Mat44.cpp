#include "Engine/Math/Mat44.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "ThirdParty/Physx/include/foundation/PxTransform.h"
#include "ThirdParty/Physx/include/foundation/PxVec3.h"
#include "ThirdParty/Physx/include/foundation/PxQuat.h"

using namespace DirectX;
using namespace physx;

Mat44::Mat44() // identity matrix
{
	m_values[Ix] = 1.f;
	m_values[Iy] = 0.f;
	m_values[Iz] = 0.f;
	m_values[Iw] = 0.f;

	m_values[Jx] = 0.f;
	m_values[Jy] = 1.f;
	m_values[Jz] = 0.f;
	m_values[Jw] = 0.f;

	m_values[Kx] = 0.f;
	m_values[Ky] = 0.f;
	m_values[Kz] = 1.f;
	m_values[Kw] = 0.f;

	m_values[Tx] = 0.f;
	m_values[Ty] = 0.f;
	m_values[Tz] = 0.f;
	m_values[Tw] = 1.f;
}

Mat44::Mat44(Vec2 const& iBasis2D, Vec2 const& jBasis2D, Vec2 const& translation2D)
{
	m_values[Ix] = iBasis2D.x;
	m_values[Iy] = iBasis2D.y;
	m_values[Iz] = 0.f;
	m_values[Iw] = 0.f;

	m_values[Jx] = jBasis2D.x;
	m_values[Jy] = jBasis2D.y;
	m_values[Jz] = 0.f;
	m_values[Jw] = 0.f;

	m_values[Kx] = 0.f;
	m_values[Ky] = 0.f;
	m_values[Kz] = 1.f;
	m_values[Kw] = 0.f;

	m_values[Tx] = translation2D.x;
	m_values[Ty] = translation2D.y;
	m_values[Tz] = 0.f;
	m_values[Tw] = 1.f;
}

Mat44::Mat44(Vec3 const& ibasis3D, Vec3 const& jBasis3D, Vec3 const& kBasis3D, Vec3 const& translation3D /*= Vec3()*/)
{
	m_values[Ix] = ibasis3D.x;
	m_values[Iy] = ibasis3D.y;
	m_values[Iz] = ibasis3D.z;
	m_values[Iw] = 0.f;

	m_values[Jx] = jBasis3D.x;
	m_values[Jy] = jBasis3D.y;
	m_values[Jz] = jBasis3D.z;
	m_values[Jw] = 0.f;

	m_values[Kx] = kBasis3D.x;
	m_values[Ky] = kBasis3D.y;
	m_values[Kz] = kBasis3D.z;
	m_values[Kw] = 0.f;

	m_values[Tx] = translation3D.x;
	m_values[Ty] = translation3D.y;
	m_values[Tz] = translation3D.z;
	m_values[Tw] = 1.f;
}

Mat44::Mat44(Vec3 const& ibasis3D, Vec3 const& jBasis3D, Vec3 const& kBasis3D, Vec3 const& translation3D, float UniformScale)
{
	m_values[Ix] = ibasis3D.x;
	m_values[Iy] = ibasis3D.y;
	m_values[Iz] = ibasis3D.z;
	m_values[Iw] = 0.f;

	m_values[Jx] = jBasis3D.x;
	m_values[Jy] = jBasis3D.y;
	m_values[Jz] = jBasis3D.z;
	m_values[Jw] = 0.f;

	m_values[Kx] = kBasis3D.x;
	m_values[Ky] = kBasis3D.y;
	m_values[Kz] = kBasis3D.z;
	m_values[Kw] = 0.f;

	m_values[Tx] = translation3D.x;
	m_values[Ty] = translation3D.y;
	m_values[Tz] = translation3D.z;
	m_values[Tw] = 1.f;

	AppendScaleUniform3D(UniformScale);
}

Mat44::Mat44(Vec4 const& ibasis4D, Vec4 const& jBasis4D, Vec4 const& kBasis4D, Vec4 const& translation4D)
{
	m_values[Ix] = ibasis4D.x;
	m_values[Iy] = ibasis4D.y;
	m_values[Iz] = ibasis4D.z;
	m_values[Iw] = ibasis4D.w;

	m_values[Jx] = jBasis4D.x;
	m_values[Jy] = jBasis4D.y;
	m_values[Jz] = jBasis4D.z;
	m_values[Jw] = jBasis4D.w;

	m_values[Kx] = kBasis4D.x;
	m_values[Ky] = kBasis4D.y;
	m_values[Kz] = kBasis4D.z;
	m_values[Kw] = kBasis4D.w;

	m_values[Tx] = translation4D.x;
	m_values[Ty] = translation4D.y;
	m_values[Tz] = translation4D.z;
	m_values[Tw] = translation4D.w;
}

Mat44::Mat44(float const* sixteenValuesBasisMajor)
{
	m_values[Ix] = sixteenValuesBasisMajor[0];
	m_values[Iy] = sixteenValuesBasisMajor[1];
	m_values[Iz] = sixteenValuesBasisMajor[2];
	m_values[Iw] = sixteenValuesBasisMajor[3];

	m_values[Jx] = sixteenValuesBasisMajor[4];
	m_values[Jy] = sixteenValuesBasisMajor[5];
	m_values[Jz] = sixteenValuesBasisMajor[6];
	m_values[Jw] = sixteenValuesBasisMajor[7];
										 
	m_values[Kx] = sixteenValuesBasisMajor[8];
	m_values[Ky] = sixteenValuesBasisMajor[9];
	m_values[Kz] = sixteenValuesBasisMajor[10];
	m_values[Kw] = sixteenValuesBasisMajor[11];
										
	m_values[Tx] = sixteenValuesBasisMajor[12];
	m_values[Ty] = sixteenValuesBasisMajor[13];
	m_values[Tz] = sixteenValuesBasisMajor[14];
	m_values[Tw] = sixteenValuesBasisMajor[15];
}

Mat44::Mat44(XrMatrix4x4f XrMat)
{
	for (int i = 0; i < 16; i++)
	{
		m_values[i] = XrMat.m[i];
	}
}

Mat44::Mat44(XrPosef XrPose)
{
	XrQuaternionf& quaternion = XrPose.orientation;
	XrVector3f& position = XrPose.position;

	Mat44 quatMat(quaternion);
	Mat44 posMat(position);

	*this = posMat.MatMultiply(quatMat);
}

Mat44::Mat44(XrQuaternionf quat)
{
	const float x2 = quat.x + quat.x;
	const float y2 = quat.y + quat.y;
	const float z2 = quat.z + quat.z;

	const float xx2 = quat.x * x2;
	const float yy2 = quat.y * y2;
	const float zz2 = quat.z * z2;

	const float yz2 = quat.y * z2;
	const float wx2 = quat.w * x2;
	const float xy2 = quat.x * y2;
	const float wz2 = quat.w * z2;
	const float xz2 = quat.x * z2;
	const float wy2 = quat.w * y2;

	m_values[Ix] = 1.0f - yy2 - zz2;
	m_values[Iy] = xy2 + wz2;
	m_values[Iz] = xz2 - wy2;
	m_values[Iw] = 0.0f;

	m_values[Jx] = xy2 - wz2;
	m_values[Jy] = 1.0f - xx2 - zz2;
	m_values[Jz] = yz2 + wx2;
	m_values[Jw] = 0.0f;

	m_values[Kx] = xz2 + wy2;
	m_values[Ky] = yz2 - wx2;
	m_values[Kz] = 1.0f - xx2 - yy2;
	m_values[Kw] = 0.0f;

	m_values[Tx] = 0.0f;
	m_values[Ty] = 0.0f;
	m_values[Tz] = 0.0f;
	m_values[Tw] = 1.0f;
}

Mat44::Mat44(Quat q)
{
	float xx = q.x * q.x;
	float yy = q.y * q.y;
	float zz = q.z * q.z;
	float xy = q.x * q.y;
	float xz = q.x * q.z;
	float yz = q.y * q.z;
	float wx = q.w * q.x;
	float wy = q.w * q.y;
	float wz = q.w * q.z;

	m_values[Ix] = 1.f - 2.f * (yy + zz);
	m_values[Iy] = 2.f * (xy + wz);
	m_values[Iz] = 2.f * (xz - wy);
	m_values[Iw] = 0.f;

	m_values[Jx] = 2.f * (xy - wz);
	m_values[Jy] = 1.f - 2.f * (xx + zz);
	m_values[Jz] = 2.f * (yz + wx);
	m_values[Jw] = 0.f;

	m_values[Kx] = 2.f * (xz + wy);
	m_values[Ky] = 2.f * (yz - wx);
	m_values[Kz] = 1.f - 2.f * (xx + yy);
	m_values[Kw] = 0.f;

	//  m_values[Ix] = 1.0f - 2.0f * (yy + zz);
	//  m_values[Iy] = 2.0f * (xy - wz);
	//  m_values[Iz] = 2.0f * (xz + wy);
	//  m_values[Iw] = 0.0f;
	//  
	//  m_values[Jx] = 2.0f * (xy + wz);
	//  m_values[Jy] = 1.0f - 2.0f * (xx + zz);
	//  m_values[Jz] = 2.0f * (yz - wx);
	//  m_values[Jw] = 0.0f;
	//  
	//  m_values[Kx] = 2.0f * (xz - wy);
	//  m_values[Ky] = 2.0f * (yz + wx);
	//  m_values[Kz] = 1.0f - 2.0f * (xx + yy);
	//  m_values[Kw] = 0.0f;

	m_values[Tx] = 0.f;
	m_values[Ty] = 0.f;
	m_values[Tz] = 0.f;
	m_values[Tw] = 1.f;
}

Mat44::Mat44(XrVector3f XrVec3)
{
	m_values[Ix] = 1.0f;
	m_values[Iy] = 0.0f;
	m_values[Iz] = 0.0f;
	m_values[Iw] = 0.0f;

	m_values[Jx] = 0.0f;
	m_values[Jy] = 1.0f;
	m_values[Jz] = 0.0f;
	m_values[Jw] = 0.0f;

	m_values[Kx] = 0.0f;
	m_values[Ky] = 0.0f;
	m_values[Kz] = 1.0f;
	m_values[Kw] = 0.0f;

	m_values[Tx] = XrVec3.x;
	m_values[Ty] = XrVec3.y;
	m_values[Tz] = XrVec3.z;
	m_values[Tw] = 1.0f;
}

Mat44::Mat44(Vec3 vec3)
{
	m_values[Ix] = 1.0f;
	m_values[Iy] = 0.0f;
	m_values[Iz] = 0.0f;
	m_values[Iw] = 0.0f;

	m_values[Jx] = 0.0f;
	m_values[Jy] = 1.0f;
	m_values[Jz] = 0.0f;
	m_values[Jw] = 0.0f;

	m_values[Kx] = 0.0f;
	m_values[Ky] = 0.0f;
	m_values[Kz] = 1.0f;
	m_values[Kw] = 0.0f;

	m_values[Tx] = vec3.x;
	m_values[Ty] = vec3.y;
	m_values[Tz] = vec3.z;
	m_values[Tw] = 1.0f;
}

Mat44::Mat44(DirectX::XMMATRIX const& directXMat)
{
	m_values[Ix] = directXMat.r[0].m128_f32[0];
	m_values[Iy] = directXMat.r[0].m128_f32[1];
	m_values[Iz] = directXMat.r[0].m128_f32[2];
	m_values[Iw] = directXMat.r[0].m128_f32[3];

	m_values[Jx] = directXMat.r[1].m128_f32[0];
	m_values[Jy] = directXMat.r[1].m128_f32[1];
	m_values[Jz] = directXMat.r[1].m128_f32[2];
	m_values[Jw] = directXMat.r[1].m128_f32[3];

	m_values[Kx] = directXMat.r[2].m128_f32[0];
	m_values[Ky] = directXMat.r[2].m128_f32[1];
	m_values[Kz] = directXMat.r[2].m128_f32[2];
	m_values[Kw] = directXMat.r[2].m128_f32[3];

	m_values[Tx] = directXMat.r[3].m128_f32[0];
	m_values[Ty] = directXMat.r[3].m128_f32[1];
	m_values[Tz] = directXMat.r[3].m128_f32[2];
	m_values[Tw] = directXMat.r[3].m128_f32[3];
}

Mat44::Mat44(physx::PxTransform const& physXTransform)
{
	PxVec3 physXPos = physXTransform.p;
	PxQuat physXQuat = physXTransform.q;
	Vec3 pos(physXPos);
	Quat quat(physXQuat);
	Mat44 posMat(pos);
	Mat44 quatMat(quat);
	*this = posMat.MatMultiply(quatMat);
}

XrMatrix4x4f Mat44::GetXrMatByMat()
{
	XrMatrix4x4f XrMat;
	for (int i = 0; i < 16; i++)
	{
		XrMat.m[i] = m_values[i];
	}
	return XrMat;
}

EulerAngles Mat44::GetEulerAngles() const
{
	EulerAngles euler;
	 
	// Pitch (Y-axis rotation)
	if (std::abs(m_values[Iz]) < 1.f)
	{
		euler.m_pitchDegrees = ConvertRadiansToDegrees(asinf(-m_values[Iz]));  // asin gives the pitch in radians
		// Roll (X-axis rotation)
		euler.m_rollDegrees = ConvertRadiansToDegrees(atan2f(m_values[Jz], m_values[Kz]));
		// Yaw (Z-axis rotation)
		euler.m_yawDegrees = ConvertRadiansToDegrees(atan2f(m_values[Iy], m_values[Ix]));
	}
	else
	{
		// Gimbal lock case
		euler.m_pitchDegrees = ConvertRadiansToDegrees(m_values[Iz] > 0.f ? -(PI * 0.5f) : (PI * 0.5f)); // ±90 degrees in radians
		euler.m_rollDegrees = 0.f;
		euler.m_yawDegrees = ConvertRadiansToDegrees(atan2f(-m_values[Jx], m_values[Jy]));
	}

	return euler;
}

Quat Mat44::GetQuaternion()
{
	Quat quat;

	// Extract the rotation part of the matrix (ignoring the translation)
	// const float& v_Ix = m_values[Ix]; // 1, 1
	// const float& v_Iy = m_values[Iy]; // 1, 2
	// const float& v_Iz = m_values[Iz]; // 1, 3
	// const float& v_Jx = m_values[Jx]; // 2, 1
	// const float& v_Jy = m_values[Jy]; // 2, 2
	// const float& v_Jz = m_values[Jz]; // 2, 3
	// const float& v_Kx = m_values[Kx]; // 3, 1
	// const float& v_Ky = m_values[Ky]; // 3, 2
	// const float& v_Kz = m_values[Kz]; // 3, 3

	// const float& m_11 = m_values[Ix]; // 1, 1
	// const float& m_12 = m_values[Iy]; // 1, 2
	// const float& m_13 = m_values[Iz]; // 1, 3
	// const float& m_21 = m_values[Jx]; // 2, 1
	// const float& m_22 = m_values[Jy]; // 2, 2
	// const float& m_23 = m_values[Jz]; // 2, 3
	// const float& m_31 = m_values[Kx]; // 3, 1
	// const float& m_32 = m_values[Ky]; // 3, 2
	// const float& m_33 = m_values[Kz]; // 3, 3

	const float& ix = m_values[Ix]; // 1, 1
	const float& jx = m_values[Jx]; // 1, 2
	const float& kx = m_values[Kx]; // 1, 3
	const float& iy = m_values[Iy]; // 2, 1
	const float& jy = m_values[Jy]; // 2, 2
	const float& ky = m_values[Ky]; // 2, 3
	const float& iz = m_values[Iz]; // 3, 1
	const float& jz = m_values[Jz]; // 3, 2
	const float& kz = m_values[Kz]; // 3, 3

	// Calculate the trace of the matrix
	// float trace = v_Ix + v_Jy + v_Kz;

	float w = sqrtf(1.0f + ix + jy + kz) * 0.5f;
	float x = sqrtf(1.0f + ix - jy - kz) * 0.5f;
	float y = sqrtf(1.0f - ix + jy - kz) * 0.5f;
	float z = sqrtf(1.0f - ix - jy + kz) * 0.5f;

	if (w >= x && w >= y && w >= z)
	{
		quat.w = w;
		float oneOfFourW = 1.f / (4.f * w);
		quat.x = (ky - jz) * oneOfFourW;
		quat.y = (iz - kx) * oneOfFourW;
		quat.z = (jx - iy) * oneOfFourW;
	}
	else if (x >= w && x >= y && x >= z)
	{
		quat.x = x;
		float oneOfFourX = 1.f / (4.f * x);
		quat.w = (ky - jz) * oneOfFourX;
		quat.y = (jx - iy) * oneOfFourX;
		quat.z = (iz - kx) * oneOfFourX;
	}
	else if (y >= w && y >= x && y >= z)
	{
		quat.y = y;
		float oneOfFourY = 1.f / (4.f * y);
		quat.w = (iz - kx) * oneOfFourY;
		quat.x = (jx - iy) * oneOfFourY;
		quat.z = (ky - jz) * oneOfFourY;
	}
	else if (z >= w && z >= x && z >= y)
	{
		quat.z = z;
		float oneOfFourZ = 1.f / (4.f * z);
		quat.w = (jx - iy) * oneOfFourZ;
		quat.x = (iz - kx) * oneOfFourZ;
		quat.y = (ky - jz) * oneOfFourZ;
	}

	//if (trace > 0.0f) {
	//	float S = 2.0f * sqrtf(trace + 1.0f);
	//	quat.w = 0.25f * S;
	//	quat.x = (v_Ky - v_Jz) / S;
	//	quat.y = (v_Iz - v_Kx) / S;
	//	quat.z = (v_Jx - v_Iy) / S;
	//}
	//else {
	//	if (v_Ix > v_Jy && v_Ix > v_Kz) {
	//		float S = 2.0f * sqrtf(1.0f + v_Ix - v_Jy - v_Kz);
	//		quat.w = (v_Ky - v_Jz) / S;
	//		quat.x = 0.25f * S;
	//		quat.y = (v_Iy + v_Jx) / S;
	//		quat.z = (v_Iz + v_Kx) / S;
	//	}
	//	else if (v_Jy > v_Kz) {
	//		float S = 2.0f * sqrtf(1.0f + v_Jy - v_Ix - v_Kz);
	//		quat.w = (v_Iz - v_Kx) / S;
	//		quat.x = (v_Iy + v_Jx) / S;
	//		quat.y = 0.25f * S;
	//		quat.z = (v_Jz + v_Ky) / S;
	//	}
	//	else {
	//		float S = 2.0f * sqrtf(1.0f + v_Kz - v_Ix - v_Jy);
	//		quat.w = (v_Jx - v_Iy) / S;
	//		quat.x = (v_Iz + v_Kx) / S;
	//		quat.y = (v_Jz + v_Ky) / S;
	//		quat.z = 0.25f * S;
	//	}
	//}

	quat = quat.GetNormalized();

	return quat;
}

Quat Mat44::TransformQuatBetweenSpaces(Quat q, Mat44 transformMat)
{
	Quat M = transformMat.GetQuaternion();
	Quat M_inverse = M.GetInversed();
	return M * q * M_inverse;
}

Mat44 const Mat44::CreateTranslation2D(Vec2 const& translationXY)
{
	Mat44 result;
	result.m_values[Ix] = 1.f;
	result.m_values[Iy] = 0.f;
	result.m_values[Iz] = 0.f;
	result.m_values[Iw] = 0.f;
	
	result.m_values[Jx] = 0.f;
	result.m_values[Jy] = 1.f;
	result.m_values[Jz] = 0.f;
	result.m_values[Jw] = 0.f;
	
	result.m_values[Kx] = 0.f;
	result.m_values[Ky] = 0.f;
	result.m_values[Kz] = 1.f;
	result.m_values[Kw] = 0.f;
	
	result.m_values[Tx] = translationXY.x;
	result.m_values[Ty] = translationXY.y;
	result.m_values[Tz] = 0.f;
	result.m_values[Tw] = 1.f;
	return result;
}

Mat44 const Mat44::CreateTranslation3D(Vec3 const& translationXYZ)
{
	Mat44 result;
	result.m_values[Ix] = 1.f;
	result.m_values[Iy] = 0.f;
	result.m_values[Iz] = 0.f;
	result.m_values[Iw] = 0.f;

	result.m_values[Jx] = 0.f;
	result.m_values[Jy] = 1.f;
	result.m_values[Jz] = 0.f;
	result.m_values[Jw] = 0.f;

	result.m_values[Kx] = 0.f;
	result.m_values[Ky] = 0.f;
	result.m_values[Kz] = 1.f;
	result.m_values[Kw] = 0.f;

	result.m_values[Tx] = translationXYZ.x;
	result.m_values[Ty] = translationXYZ.y;
	result.m_values[Tz] = translationXYZ.z;
	result.m_values[Tw] = 1.f;
	return result;
}

Mat44 const Mat44::CreateUniformScale2D(float uniformScaleXY)
{
	Mat44 result;
	result.m_values[Ix] = uniformScaleXY;
	result.m_values[Iy] = 0.f;
	result.m_values[Iz] = 0.f;
	result.m_values[Iw] = 0.f;

	result.m_values[Jx] = 0.f;
	result.m_values[Jy] = uniformScaleXY;
	result.m_values[Jz] = 0.f;
	result.m_values[Jw] = 0.f;

	result.m_values[Kx] = 0.f;
	result.m_values[Ky] = 0.f;
	result.m_values[Kz] = 1.f;
	result.m_values[Kw] = 0.f;

	result.m_values[Tx] = 0.f;
	result.m_values[Ty] = 0.f;
	result.m_values[Tz] = 0.f;
	result.m_values[Tw] = 1.f;
	return result;
}

Mat44 const Mat44::CreateUniformScale3D(float uniformScaleXYZ)
{
	Mat44 result;
	result.m_values[Ix] = uniformScaleXYZ;
	result.m_values[Iy] = 0.f;
	result.m_values[Iz] = 0.f;
	result.m_values[Iw] = 0.f;

	result.m_values[Jx] = 0.f;
	result.m_values[Jy] = uniformScaleXYZ;
	result.m_values[Jz] = 0.f;
	result.m_values[Jw] = 0.f;

	result.m_values[Kx] = 0.f;
	result.m_values[Ky] = 0.f;
	result.m_values[Kz] = uniformScaleXYZ;
	result.m_values[Kw] = 0.f;

	result.m_values[Tx] = 0.f;
	result.m_values[Ty] = 0.f;
	result.m_values[Tz] = 0.f;
	result.m_values[Tw] = 1.f;
	return result;
}

Mat44 const Mat44::CreateNonUniformScale2D(Vec2 nonUniformScaleXY)
{
	Mat44 result;
	result.m_values[Ix] = nonUniformScaleXY.x;
	result.m_values[Iy] = 0.f;
	result.m_values[Iz] = 0.f;
	result.m_values[Iw] = 0.f;

	result.m_values[Jx] = 0.f;
	result.m_values[Jy] = nonUniformScaleXY.y;
	result.m_values[Jz] = 0.f;
	result.m_values[Jw] = 0.f;

	result.m_values[Kx] = 0.f;
	result.m_values[Ky] = 0.f;
	result.m_values[Kz] = 1.f;
	result.m_values[Kw] = 0.f;

	result.m_values[Tx] = 0.f;
	result.m_values[Ty] = 0.f;
	result.m_values[Tz] = 0.f;
	result.m_values[Tw] = 1.f;
	return result;
}

Mat44 const Mat44::CreateNonUniformScale3D(Vec3 nonUniformScaleXYZ)
{
	Mat44 result;
	result.m_values[Ix] = nonUniformScaleXYZ.x;
	result.m_values[Iy] = 0.f;
	result.m_values[Iz] = 0.f;
	result.m_values[Iw] = 0.f;

	result.m_values[Jx] = 0.f;
	result.m_values[Jy] = nonUniformScaleXYZ.y;
	result.m_values[Jz] = 0.f;
	result.m_values[Jw] = 0.f;

	result.m_values[Kx] = 0.f;
	result.m_values[Ky] = 0.f;
	result.m_values[Kz] = nonUniformScaleXYZ.z;
	result.m_values[Kw] = 0.f;

	result.m_values[Tx] = 0.f;
	result.m_values[Ty] = 0.f;
	result.m_values[Tz] = 0.f;
	result.m_values[Tw] = 1.f;
	return result;
}

// kBasis stays unchanged
Mat44 const Mat44::CreateZRotationDegrees(float rotationDegreesAboutZ)
{
	Mat44 result;
	float rotationDegrees = ConvertDegreesToRadians(rotationDegreesAboutZ);
	result.m_values[Ix] = cosf(rotationDegrees);
	result.m_values[Iy] = sinf(rotationDegrees);
	result.m_values[Iz] = 0.f;
	result.m_values[Iw] = 0.f;

	result.m_values[Jx] =-sinf(rotationDegrees);
	result.m_values[Jy] = cosf(rotationDegrees);
	result.m_values[Jz] = 0.f;
	result.m_values[Jw] = 0.f;

	result.m_values[Kx] = 0.f;	
	result.m_values[Ky] = 0.f;
	result.m_values[Kz] = 1.f;
	result.m_values[Kw] = 0.f;

	result.m_values[Tx] = 0.f;
	result.m_values[Ty] = 0.f;
	result.m_values[Tz] = 0.f;
	result.m_values[Tw] = 1.f;
	return result;
}

// jBasis stays unchanged
Mat44 const Mat44::CreateYRotationDegrees(float rotationDegreesAboutY)
{
	Mat44 result;
	float rotationDegrees = ConvertDegreesToRadians(rotationDegreesAboutY);
	result.m_values[Ix] = cosf(rotationDegrees);
	result.m_values[Iy] = 0.f;
	result.m_values[Iz] = -sinf(rotationDegrees);
	result.m_values[Iw] = 0.f;

	result.m_values[Jx] = 0.f;
	result.m_values[Jy] = 1.f;
	result.m_values[Jz] = 0.f;
	result.m_values[Jw] = 0.f;

	result.m_values[Kx] = sinf(rotationDegrees);
	result.m_values[Ky] = 0.f;
	result.m_values[Kz] = cosf(rotationDegrees);
	result.m_values[Kw] = 0.f;

	result.m_values[Tx] = 0.f;
	result.m_values[Ty] = 0.f;
	result.m_values[Tz] = 0.f;
	result.m_values[Tw] = 1.f;
	return result;
}

// iBasis stays unchanged
Mat44 const Mat44::CreateXRotationDegrees(float rotationDegreesAboutX)
{
	Mat44 result;
	float rotationDegrees = ConvertDegreesToRadians(rotationDegreesAboutX);
	result.m_values[Ix] = 1.f;
	result.m_values[Iy] = 0.f;
	result.m_values[Iz] = 0.f;
	result.m_values[Iw] = 0.f;

	result.m_values[Jx] = 0.f;
	result.m_values[Jy] = cosf(rotationDegrees);
	result.m_values[Jz] = sinf(rotationDegrees);
	result.m_values[Jw] = 0.f;

	result.m_values[Kx] = 0.f;
	result.m_values[Ky] =-sinf(rotationDegrees);
	result.m_values[Kz] = cosf(rotationDegrees);
	result.m_values[Kw] = 0.f;

	result.m_values[Tx] = 0.f;
	result.m_values[Ty] = 0.f;
	result.m_values[Tz] = 0.f;
	result.m_values[Tw] = 1.f;
	return result;
}

Mat44 const Mat44::CreateRotationMatrixFromQuat(Quat const& q)
{
	Mat44 mat;

	float xx = q.x * q.x;
	float yy = q.y * q.y;
	float zz = q.z * q.z;
	float xy = q.x * q.y;
	float xz = q.x * q.z;
	float yz = q.y * q.z;
	float wx = q.w * q.x;
	float wy = q.w * q.y;
	float wz = q.w * q.z;

	mat.m_values[Ix] = 1.0f - 2.0f * (yy + zz);
	mat.m_values[Iy] = 2.0f * (xy + wz);
	mat.m_values[Iz] = 2.0f * (xz - wy);
	mat.m_values[Iw] = 0.0f;

	mat.m_values[Jx] = 2.0f * (xy - wz);
	mat.m_values[Jy] = 1.0f - 2.0f * (xx + zz);
	mat.m_values[Jz] = 2.0f * (yz + wx);
	mat.m_values[Jw] = 0.0f;

	mat.m_values[Kx] = 2.0f * (xz + wy);
	mat.m_values[Ky] = 2.0f * (yz - wx);
	mat.m_values[Kz] = 1.0f - 2.0f * (xx + yy);
	mat.m_values[Kw] = 0.0f;

	mat.m_values[Tx] = 0.0f;
	mat.m_values[Ty] = 0.0f;
	mat.m_values[Tz] = 0.0f;
	mat.m_values[Tw] = 1.0f;

	return mat;
}

Vec2 const Mat44::TransformVectorQuantity2D(Vec2 const& vectorQuantityXY) const
{
	Mat44 copyOfThis = *this;
	float const* left = &copyOfThis.m_values[0]; 
	Vec4 right(vectorQuantityXY.x, vectorQuantityXY.y, 0.f, 0.f);
	Vec4 result;

	result.x = DotProduct4D(Vec4(left[Ix], left[Jx], left[Kx], left[Tx]), right);
	result.y = DotProduct4D(Vec4(left[Iy], left[Jy], left[Ky], left[Ty]), right);

	return Vec2(result.x, result.y);
}

Vec3 const Mat44::TransformVectorQuantity3D(Vec3 const& vectorQuantityXYZ) const
{
	Mat44 copyOfThis = *this;
	float const* left = &copyOfThis.m_values[0];
	Vec4 right(vectorQuantityXYZ.x, vectorQuantityXYZ.y, vectorQuantityXYZ.z, 0.f);	// ignore the translation
	Vec4 result;

	result.x = DotProduct4D(Vec4(left[Ix], left[Jx], left[Kx], left[Tx]), right);
	result.y = DotProduct4D(Vec4(left[Iy], left[Jy], left[Ky], left[Ty]), right);
	result.z = DotProduct4D(Vec4(left[Iz], left[Jz], left[Kz], left[Tz]), right);
	result.w = 0.f;		

	return Vec3(result.x, result.y, result.z);
}

Vec2 const Mat44::TransformPosition2D(Vec2 const& positionXY) const
{
	Mat44 copyOfThis = *this;
	float const* left = &copyOfThis.m_values[0];
	Vec4 right(positionXY.x, positionXY.y, 0.f, 1.f);
	Vec4 result;

	result.x = DotProduct4D(Vec4(left[Ix], left[Jx], left[Kx], left[Tx]), right);
	result.y = DotProduct4D(Vec4(left[Iy], left[Jy], left[Ky], left[Ty]), right);

	return Vec2(result.x, result.y);
}

Vec3 const Mat44::TransformPosition3D(Vec3 const& position3D) const
{
	Mat44 copyOfThis = *this;
	float const* left = &copyOfThis.m_values[0];
	Vec4 right(position3D.x, position3D.y, position3D.z, 1.f);		// calculate the translation
	Vec4 result;

	result.x = DotProduct4D(Vec4(left[Ix], left[Jx], left[Kx], left[Tx]), right);
	result.y = DotProduct4D(Vec4(left[Iy], left[Jy], left[Ky], left[Ty]), right);
	result.z = DotProduct4D(Vec4(left[Iz], left[Jz], left[Kz], left[Tz]), right);
	result.w = 0.f;

	return Vec3(result.x, result.y, result.z);
}

Vec4 const Mat44::TransformHomogeneous3D(Vec4 const& homogeneousPoint3D) const
{
	Mat44 copyOfThis = *this;
	float const* left = &copyOfThis.m_values[0];
	Vec4 result;

	result.x = DotProduct4D(Vec4(left[Ix], left[Jx], left[Kx], left[Tx]), homogeneousPoint3D);
	result.y = DotProduct4D(Vec4(left[Iy], left[Jy], left[Ky], left[Ty]), homogeneousPoint3D);
	result.z = DotProduct4D(Vec4(left[Iz], left[Jz], left[Kz], left[Tz]), homogeneousPoint3D);
	result.w = DotProduct4D(Vec4(left[Iw], left[Jw], left[Kw], left[Tw]), homogeneousPoint3D);

	return result;
}

// this have problem when converting between different handedness spaces
Quat Mat44::TransformQuaternion(Quat const& q)
{
	Mat44 copyOfThis = *this;
	float const* left = &copyOfThis.m_values[0];
	Vec4 right(q.x, q.y, q.z, q.w);
	Quat result;

	result.x = DotProduct4D(Vec4(left[Ix], left[Jx], left[Kx], left[Tx]), right);
	result.y = DotProduct4D(Vec4(left[Iy], left[Jy], left[Ky], left[Ty]), right);
	result.z = DotProduct4D(Vec4(left[Iz], left[Jz], left[Kz], left[Tz]), right);
	result.w = DotProduct4D(Vec4(left[Iw], left[Jw], left[Kw], left[Tw]), right);

	return result.GetNormalized();
}

// If you have a rotation matrix R expressed in one coordinate system
// and you need to convert it to another coordinate system using a transformation matrix 𝑇
// the transformed rotation matrix is computed using similarity transformation
// the result is [T][R][T-1], is not [T][R]!!! the calculation order is [T]([R][T-1])
// the [R] cannot contain translation information!!!
// if we are transforming a translation matrix, we just need [Transform][Translation]
// if we are transforming a transformation matrix(affine matrix), which means containing translation and rotation info
// this method also works [Transform][Translation + Rotation][Transform-1]
Mat44 Mat44::SimilarityTransformation(Mat44 const& rotationMat)
{
	Mat44 transformMat = *this;
	Mat44 rotMat = rotationMat;

	Mat44 transformMatInversed = transformMat; // get inversed of the transform matrix
	transformMatInversed.Transpose(); // for transform matrix, pure rotation matrices: The inverse is the same as the transpose

	Mat44 stepOneMat = rotMat.MatMultiply(transformMatInversed);
	Mat44 stepTwoMat = transformMat.MatMultiply(stepOneMat);

	return stepTwoMat;
}

float* Mat44::GetAsFloatArray()
{
	return &m_values[0];
}

float const* Mat44::GetAsFloatArray() const
{
	float const* valuePtr = &m_values[0];
	return valuePtr;
}

Vec2 const Mat44::GetIBasis2D() const
{
	Vec2 result;
	result.x = m_values[Ix];
	result.y = m_values[Iy];
	return result;
}

Vec2 const Mat44::GetJBasis2D() const
{
	Vec2 result;
	result.x = m_values[Jx];
	result.y = m_values[Jy];
	return result;
}

Vec2 const Mat44::GetTranslation2D() const
{
	Vec2 result;
	result.x = m_values[Tx];
	result.y = m_values[Ty];
	return result;
}

Vec3 const Mat44::GetIBasis3D() const
{
	Vec3 result;
	result.x = m_values[Ix];
	result.y = m_values[Iy];
	result.z = m_values[Iz];
	return result;
}

Vec3 const Mat44::GetJBasis3D() const
{
	Vec3 result;
	result.x = m_values[Jx];
	result.y = m_values[Jy];
	result.z = m_values[Jz];
	return result;
}

Vec3 const Mat44::GetKBasis3D() const
{
	Vec3 result;
	result.x = m_values[Kx];
	result.y = m_values[Ky];
	result.z = m_values[Kz];
	return result;
}

Vec3 const Mat44::GetTranslation3D() const
{
	Vec3 result;
	result.x = m_values[Tx];
	result.y = m_values[Ty];
	result.z = m_values[Tz];
	return result;
}

Vec4 const Mat44::GetIBasis4D() const
{
	Vec4 result;
	result.x = m_values[Ix];
	result.y = m_values[Iy];
	result.z = m_values[Iz];
	result.w = m_values[Iw];
	return result;
}

Vec4 const Mat44::GetJBasis4D() const
{
	Vec4 result;
	result.x = m_values[Jx];
	result.y = m_values[Jy];
	result.z = m_values[Jz];
	result.w = m_values[Jw];
	return result;
}

Vec4 const Mat44::GetKBasis4D() const
{
	Vec4 result;
	result.x = m_values[Kx];
	result.y = m_values[Ky];
	result.z = m_values[Kz];
	result.w = m_values[Kw];
	return result;
}

Vec4 const Mat44::GetTranslation4D() const
{
	Vec4 result;
	result.x = m_values[Tx];
	result.y = m_values[Ty];
	result.z = m_values[Tz];
	result.w = m_values[Tw];
	return result;
}

void Mat44::SetTranslation2D(Vec2 const& translationXY)
{
	m_values[Tx] = translationXY.x;
	m_values[Ty] = translationXY.y;
	m_values[Tz] = 0.f;
	m_values[Tw] = 1.f;
}

void Mat44::SetTranslation3D(Vec3 const& translationXYZ)
{
	m_values[Tx] = translationXYZ.x;
	m_values[Ty] = translationXYZ.y;
	m_values[Tz] = translationXYZ.z;
	m_values[Tw] = 1.f;
}

void Mat44::SetIJ2D(Vec2 const& iBasis2D, Vec2 const& jBasis2D)
{
	m_values[Ix] = iBasis2D.x;
	m_values[Iy] = iBasis2D.y;
	m_values[Iz] = 0.f;
	m_values[Iw] = 0.f;

	m_values[Jx] = jBasis2D.x;
	m_values[Jy] = jBasis2D.y;
	m_values[Jz] = 0.f;
	m_values[Jw] = 0.f;
}

void Mat44::SetIJT2D(Vec2 const& iBasis2D, Vec2 const& jBasis2D, Vec2 const& translationXY)
{
	m_values[Ix] = iBasis2D.x;
	m_values[Iy] = iBasis2D.y;
	m_values[Iz] = 0.f;
	m_values[Iw] = 0.f;

	m_values[Jx] = jBasis2D.x;
	m_values[Jy] = jBasis2D.y;
	m_values[Jz] = 0.f;
	m_values[Jw] = 0.f;

	m_values[Tx] = translationXY.x;
	m_values[Ty] = translationXY.y;
	m_values[Tz] = 0.f;
	m_values[Tw] = 1.f;
}

void Mat44::SetIJK3D(Vec3 const& iBasis3D, Vec3 const& jBasis3D, Vec3 const& kBasis3D)
{
	m_values[Ix] = iBasis3D.x;
	m_values[Iy] = iBasis3D.y;
	m_values[Iz] = iBasis3D.z;
	m_values[Iw] = 0.f;

	m_values[Jx] = jBasis3D.x;
	m_values[Jy] = jBasis3D.y;
	m_values[Jz] = jBasis3D.z;
	m_values[Jw] = 0.f;

	m_values[Kx] = kBasis3D.x;
	m_values[Ky] = kBasis3D.y;
	m_values[Kz] = kBasis3D.z;
	m_values[Kw] = 0.f;
}

void Mat44::SetIJKT3D(Vec3 const& iBasis3D, Vec3 const& jBasis3D, Vec3 const& kBasis3D, Vec3 const& translationXYZ)
{
	m_values[Ix] = iBasis3D.x;
	m_values[Iy] = iBasis3D.y;
	m_values[Iz] = iBasis3D.z;
	m_values[Iw] = 0.f;

	m_values[Jx] = jBasis3D.x;
	m_values[Jy] = jBasis3D.y;
	m_values[Jz] = jBasis3D.z;
	m_values[Jw] = 0.f;

	m_values[Kx] = kBasis3D.x;
	m_values[Ky] = kBasis3D.y;
	m_values[Kz] = kBasis3D.z;
	m_values[Kw] = 0.f;

	m_values[Tx] = translationXYZ.x;
	m_values[Ty] = translationXYZ.y;
	m_values[Tz] = translationXYZ.z;
	m_values[Tw] = 1.f;
}

void Mat44::SetIJKT4D(Vec4 const& iBasis3D, Vec4 const& jBasis3D, Vec4 const& kBasis3D, Vec4 const& translation4D)
{
	m_values[Ix] = iBasis3D.x;
	m_values[Iy] = iBasis3D.y;
	m_values[Iz] = iBasis3D.z;
	m_values[Iw] = iBasis3D.w;

	m_values[Jx] = jBasis3D.x;
	m_values[Jy] = jBasis3D.y;
	m_values[Jz] = jBasis3D.z;
	m_values[Jw] = jBasis3D.w;

	m_values[Kx] = kBasis3D.x;
	m_values[Ky] = kBasis3D.y;
	m_values[Kz] = kBasis3D.z;
	m_values[Kw] = kBasis3D.w;

	m_values[Tx] = translation4D.x;
	m_values[Ty] = translation4D.y;
	m_values[Tz] = translation4D.z;
	m_values[Tw] = translation4D.w;
}

// we use our class matrix to transform the argument matrix
// In post-multiplication, the order of matrix multiplication follows a right-to-left sequence
// [T][R][Identity]
Mat44 Mat44::MatMultiply(Mat44 const& addingTransformInfo)
{
	float const* left = &m_values[0]; // make a copy of current value, in case the original values are polluted
	float const* right = &addingTransformInfo.m_values[0];
	Mat44 result;

	result.m_values[Ix] = DotProduct4D(Vec4(left[Ix], left[Jx], left[Kx], left[Tx]), Vec4(right[Ix], right[Iy], right[Iz], right[Iw]));
	result.m_values[Iy] = DotProduct4D(Vec4(left[Iy], left[Jy], left[Ky], left[Ty]), Vec4(right[Ix], right[Iy], right[Iz], right[Iw]));
	result.m_values[Iz] = DotProduct4D(Vec4(left[Iz], left[Jz], left[Kz], left[Tz]), Vec4(right[Ix], right[Iy], right[Iz], right[Iw]));
	result.m_values[Iw] = DotProduct4D(Vec4(left[Iw], left[Jw], left[Kw], left[Tw]), Vec4(right[Ix], right[Iy], right[Iz], right[Iw]));

	result.m_values[Jx] = DotProduct4D(Vec4(left[Ix], left[Jx], left[Kx], left[Tx]), Vec4(right[Jx], right[Jy], right[Jz], right[Jw]));
	result.m_values[Jy] = DotProduct4D(Vec4(left[Iy], left[Jy], left[Ky], left[Ty]), Vec4(right[Jx], right[Jy], right[Jz], right[Jw]));
	result.m_values[Jz] = DotProduct4D(Vec4(left[Iz], left[Jz], left[Kz], left[Tz]), Vec4(right[Jx], right[Jy], right[Jz], right[Jw]));
	result.m_values[Jw] = DotProduct4D(Vec4(left[Iw], left[Jw], left[Kw], left[Tw]), Vec4(right[Jx], right[Jy], right[Jz], right[Jw]));

	result.m_values[Kx] = DotProduct4D(Vec4(left[Ix], left[Jx], left[Kx], left[Tx]), Vec4(right[Kx], right[Ky], right[Kz], right[Kw]));
	result.m_values[Ky] = DotProduct4D(Vec4(left[Iy], left[Jy], left[Ky], left[Ty]), Vec4(right[Kx], right[Ky], right[Kz], right[Kw]));
	result.m_values[Kz] = DotProduct4D(Vec4(left[Iz], left[Jz], left[Kz], left[Tz]), Vec4(right[Kx], right[Ky], right[Kz], right[Kw]));
	result.m_values[Kw] = DotProduct4D(Vec4(left[Iw], left[Jw], left[Kw], left[Tw]), Vec4(right[Kx], right[Ky], right[Kz], right[Kw]));

	result.m_values[Tx] = DotProduct4D(Vec4(left[Ix], left[Jx], left[Kx], left[Tx]), Vec4(right[Tx], right[Ty], right[Tz], right[Tw]));
	result.m_values[Ty] = DotProduct4D(Vec4(left[Iy], left[Jy], left[Ky], left[Ty]), Vec4(right[Tx], right[Ty], right[Tz], right[Tw]));
	result.m_values[Tz] = DotProduct4D(Vec4(left[Iz], left[Jz], left[Kz], left[Tz]), Vec4(right[Tx], right[Ty], right[Tz], right[Tw]));
	result.m_values[Tw] = DotProduct4D(Vec4(left[Iw], left[Jw], left[Kw], left[Tw]), Vec4(right[Tx], right[Ty], right[Tz], right[Tw]));
	
	return result;
}

// multiplies by the "appendThis" matrix on the :
//  - right hand side ( column- major notation) or (equivalently):
//	- left hand side(row-major notation)
//
// Essentially, this does "push and flatten" of another transform onto the existing matrix, which
// will be applied first to the points when transformed, e.g.:
//
//  [this] = [this][append] // in column notation
//	[this] = [append][this]	// in row notation
//
// so subsequent point transformations will be transformed by the last appended matrix first,
// then prior matrix, etc:
//
//  this( append(p) )	// in function notation
//	[this][append][p]	// in column notation
//	[p][append][this]	// in row notation
//
void Mat44::Append(Mat44 const& appendThis)
{
	Mat44 copyOfThis = *this;
	float const* left = &copyOfThis.m_values[0]; // make a copy of current value, in case the original values are polluted
	float const* right = &appendThis.m_values[0];

	m_values[Ix] = DotProduct4D(Vec4(left[Ix], left[Jx], left[Kx], left[Tx]), Vec4(right[Ix], right[Iy], right[Iz], right[Iw]));
	m_values[Iy] = DotProduct4D(Vec4(left[Iy], left[Jy], left[Ky], left[Ty]), Vec4(right[Ix], right[Iy], right[Iz], right[Iw]));
	m_values[Iz] = DotProduct4D(Vec4(left[Iz], left[Jz], left[Kz], left[Tz]), Vec4(right[Ix], right[Iy], right[Iz], right[Iw]));
	m_values[Iw] = DotProduct4D(Vec4(left[Iw], left[Jw], left[Kw], left[Tw]), Vec4(right[Ix], right[Iy], right[Iz], right[Iw]));

	m_values[Jx] = DotProduct4D(Vec4(left[Ix], left[Jx], left[Kx], left[Tx]), Vec4(right[Jx], right[Jy], right[Jz], right[Jw]));
	m_values[Jy] = DotProduct4D(Vec4(left[Iy], left[Jy], left[Ky], left[Ty]), Vec4(right[Jx], right[Jy], right[Jz], right[Jw]));
	m_values[Jz] = DotProduct4D(Vec4(left[Iz], left[Jz], left[Kz], left[Tz]), Vec4(right[Jx], right[Jy], right[Jz], right[Jw]));
	m_values[Jw] = DotProduct4D(Vec4(left[Iw], left[Jw], left[Kw], left[Tw]), Vec4(right[Jx], right[Jy], right[Jz], right[Jw]));

	m_values[Kx] = DotProduct4D(Vec4(left[Ix], left[Jx], left[Kx], left[Tx]), Vec4(right[Kx], right[Ky], right[Kz], right[Kw]));
	m_values[Ky] = DotProduct4D(Vec4(left[Iy], left[Jy], left[Ky], left[Ty]), Vec4(right[Kx], right[Ky], right[Kz], right[Kw]));
	m_values[Kz] = DotProduct4D(Vec4(left[Iz], left[Jz], left[Kz], left[Tz]), Vec4(right[Kx], right[Ky], right[Kz], right[Kw]));
	m_values[Kw] = DotProduct4D(Vec4(left[Iw], left[Jw], left[Kw], left[Tw]), Vec4(right[Kx], right[Ky], right[Kz], right[Kw]));

	m_values[Tx] = DotProduct4D(Vec4(left[Ix], left[Jx], left[Kx], left[Tx]), Vec4(right[Tx], right[Ty], right[Tz], right[Tw]));
	m_values[Ty] = DotProduct4D(Vec4(left[Iy], left[Jy], left[Ky], left[Ty]), Vec4(right[Tx], right[Ty], right[Tz], right[Tw]));
	m_values[Tz] = DotProduct4D(Vec4(left[Iz], left[Jz], left[Kz], left[Tz]), Vec4(right[Tx], right[Ty], right[Tz], right[Tw]));
	m_values[Tw] = DotProduct4D(Vec4(left[Iw], left[Jw], left[Kw], left[Tw]), Vec4(right[Tx], right[Ty], right[Tz], right[Tw]));
}

void Mat44::AppendZRotation(float degreesRotationAboutZ)
{
	Mat44 zRotationMatrix = CreateZRotationDegrees(degreesRotationAboutZ);
	Append(zRotationMatrix);
}

void Mat44::AppendYRotation(float degreesRotationAboutY)
{
	Mat44 yRotationMatrix = CreateYRotationDegrees(degreesRotationAboutY);
	Append(yRotationMatrix);
}

void Mat44::AppendXRotation(float degreesRotationAboutX)
{
	Mat44 xRotationMatrix = CreateXRotationDegrees(degreesRotationAboutX);
	Append(xRotationMatrix);
}

void Mat44::AppendTranslation2D(Vec2 const& translationXY)
{
	Mat44 translationMatrix = CreateTranslation2D(translationXY);
	Append(translationMatrix);
}

void Mat44::AppendTranslation3D(Vec3 const& translationXYZ)
{
	Mat44 translationMatrix = CreateTranslation3D(translationXYZ);
	Append(translationMatrix);
}

void Mat44::AppendScaleUniform2D(float uniformScaleXY)
{
	Mat44 scaleMatrix = CreateUniformScale2D(uniformScaleXY);
	Append(scaleMatrix);
}

void Mat44::AppendScaleUniform3D(float uniformScaleXYZ)
{
	Mat44 scaleMatrix = CreateUniformScale3D(uniformScaleXYZ);
	Append(scaleMatrix);
}

void Mat44::AppendScaleNonUniform2D(Vec2 const& nonUniformScaleXY)
{
	Mat44 scaleMatrix = CreateNonUniformScale2D(nonUniformScaleXY);
	Append(scaleMatrix);
}

void Mat44::AppendScaleNonUniform3D(Vec3 const& nonUniformScaleXYZ)
{
	Mat44 scaleMatrix = CreateNonUniformScale3D(nonUniformScaleXYZ);
	Append(scaleMatrix);
}

void Mat44::AppendRotation(Quat const& q)
{
	Mat44 quatMat = CreateRotationMatrixFromQuat(q);
	return Append(quatMat);
}

void Mat44::Transpose()
{
	// Mat44 old = *this;
	// Mat44 old = Mat44(*this);
	Mat44 oldMat = Mat44(m_values);

	m_values[Ix] = oldMat.m_values[Ix];
	m_values[Iy] = oldMat.m_values[Jx];
	m_values[Iz] = oldMat.m_values[Kx];
	m_values[Iw] = oldMat.m_values[Tx];

	m_values[Jx] = oldMat.m_values[Iy];
	m_values[Jy] = oldMat.m_values[Jy];
	m_values[Jz] = oldMat.m_values[Ky];
	m_values[Jw] = oldMat.m_values[Ty];

	m_values[Kx] = oldMat.m_values[Iz];
	m_values[Ky] = oldMat.m_values[Jz];
	m_values[Kz] = oldMat.m_values[Kz];
	m_values[Kw] = oldMat.m_values[Tz];

	m_values[Tx] = oldMat.m_values[Iw];
	m_values[Ty] = oldMat.m_values[Jw];
	m_values[Tz] = oldMat.m_values[Kw];
	m_values[Tw] = oldMat.m_values[Tw];
}

// [rotate]-1[translate]-1
Mat44 const Mat44::GetOrthonormalInverse()
{
	// build rotation-only matrix and invert (transpose)
	Mat44 antiRotation;
	antiRotation.SetIJK3D(GetIBasis3D(), GetJBasis3D(), GetKBasis3D());
	antiRotation.Transpose();

	// build translation only matrix and inverse it(NOT TRANSPOSE IT!!!)
	Mat44 antiTranslation;
	Vec3 inverseTranslation = GetTranslation3D() * (-1.f);
	antiTranslation.SetTranslation3D(inverseTranslation);

	// append two matrix
	antiRotation.Append(antiTranslation);
	return antiRotation;
}

void Mat44::Orthonormalize_IFwd_JLeft_KUp()
{
	// Get I basis
	Vec3 fwdI = GetIBasis3D();
	fwdI = fwdI.GetNormalized();

	// calculate the current J basis's projection on I
	// delete this wrong part and normalize it to get Orthogonal and normalized J
	Vec3 brokenJ = GetJBasis3D();
	Vec3 brokenJOnI = DotProduct3D(brokenJ, fwdI) * fwdI;
	Vec3 lftJ = (brokenJ - brokenJOnI).GetNormalized();

	// same way to get corrected K basis
	Vec3 brokenK = GetKBasis3D();
	Vec3 brokenKOnI = DotProduct3D(brokenK, fwdI) * fwdI;
	Vec3 brokenKOnJ = DotProduct3D(brokenK, lftJ) * lftJ;
	Vec3 UpK = (brokenK - brokenKOnI - brokenKOnJ).GetNormalized();

	// write into this Matrix
	SetIJK3D(fwdI, lftJ, UpK);
}

Mat44 const Mat44::CreateOrthoProjection(float left, float right, float bottom, float top, float Znear, float Zfar)
{
	Mat44 projectionMatrix;//get a identity matrix first

	// then modify the matrix to get the projection matrix from the view space
	// todo: how this matrix is able to to clip the matrix
	projectionMatrix.m_values[Ix] = 2.f / (right - left);
	projectionMatrix.m_values[Jy] = 2.f / (top - bottom);
	projectionMatrix.m_values[Kz] = 1.f / (Zfar - Znear);

	projectionMatrix.m_values[Tx] = (right + left) / (left - right);
	projectionMatrix.m_values[Ty] = (top + bottom) / (bottom - top);
	projectionMatrix.m_values[Tz] = Znear / (Znear - Zfar);

	return projectionMatrix;
}

Mat44 const Mat44::CreateOrthoProjection(Vec3 LBN, Vec3 RTF)
{
	float left = LBN.x;
	float bottom = LBN.y;
	float Znear = LBN.z;

	float right = RTF.x;
	float top = RTF.y;
	float Zfar = RTF.z;

	Mat44 projectionMatrix = CreateOrthoProjection(left, right, bottom, top, Znear, Zfar);
	return projectionMatrix;
}

Mat44 const Mat44::CreatePerspectiveProjection(float fovYDegrees, float aspectRatio, float zNear, float zFar)
{
	Mat44 projectionMatrix;//get a identity matrix first

	float scaleY = CosDegrees(fovYDegrees * 0.5f) / SinDegrees(fovYDegrees * 0.5f);  // equals 1 if vertical Field of View is 90
	float scaleX = scaleY / aspectRatio; // equals scaleY if screen is square (aspect=1.0); equals 1 if square screen and FOV 90
	float scaleZ = zFar / (zFar - zNear);

	float translateZ = (zNear * zFar) / (zNear - zFar);

	projectionMatrix.m_values[Ix] = scaleX;
	projectionMatrix.m_values[Jy] = scaleY;
	projectionMatrix.m_values[Kz] = scaleZ;
	
	projectionMatrix.m_values[Kw] = 1.0f; // this puts Z into the W component (in preparation for the hardware w-divide)

	projectionMatrix.m_values[Tz] = translateZ;
	projectionMatrix.m_values[Tw] = 0.0f;  // Otherwise we would be putting Z+1 (instead of Z) into the W component

	return projectionMatrix;
}

void Mat44::operator=(Mat44 const& copyFrom)
{
	for (int i = 0; i < 16; i++)
	{
		m_values[i] = copyFrom.m_values[i];
	}
}
