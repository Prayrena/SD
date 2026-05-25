#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Renderer/DebugRender.hpp"
#include <math.h>

extern RandomNumberGenerator* g_rng;

int GetMax(int a, int b)
{
	return (a > b) ? a : b;
}

float GetClamped(float value, float minValue, float maxValue)
{
	if (value <= minValue)
	{
		return minValue;
	}

	if (value >= maxValue)
	{
		return maxValue;
	}

	else
	{
		return value;
	}
}

template <typename T>
T GetClamped(T value, T minValue, T maxValue)
{
	if (value < minValue)
	{
		return minValue;
	}

	else if (value > maxValue)
	{
		return maxValue;
	}

	else
	{
		return value;
	}
}

template <>
Vec3 GetClamped<Vec3>(Vec3 value, Vec3 minValue, Vec3 maxValue) 
{
	return Vec3
	(
		GetClamped(value.x, minValue.x, maxValue.x),
		GetClamped(value.y, minValue.y, maxValue.y),
		GetClamped(value.z, minValue.z, maxValue.z)
	);
}

int GetClamped(int value, int minValue, int maxValue)
{
	if (value <= minValue)
	{
		return minValue;
	}

	if (value >= maxValue)
	{
		return maxValue;
	}

	else
	{
		return value;
	}
}

float GetClampedByFloatRange(float value, FloatRange range)
{
	return GetClamped(value, range.m_min, range.m_max);
}

float GetClampedZeroToOne(float value)
{
	if (value > 1.f)
	{
		return 1.f;
	}

	else if (value < 0.f)
	{
		return 0.f;
	}

	else
	{
		return value;
	}
}

float Interpolate(float start, float end, float fractionTowardEnd)
{
	float disp = end - start;
	float distWithinRange = disp * fractionTowardEnd;
	float interpolatedPosition = start + distWithinRange;
	return interpolatedPosition;
}

Vec2 Interpolate(Vec2 start, Vec2 end, float fractionTowardEnd)
{
	Vec2 result;
	result.x = Interpolate(start.x, end.x, fractionTowardEnd);
	result.y = Interpolate(start.y, end.y, fractionTowardEnd);
	return result;
}

Vec3 Interpolate(Vec3 start, Vec3 end, float fractionTowardEnd)
{
	Vec3 result;
	result.x = Interpolate(start.x, end.x, fractionTowardEnd);
	result.y = Interpolate(start.y, end.y, fractionTowardEnd);
	result.z = Interpolate(start.z, end.z, fractionTowardEnd);
	return result;
}

Rgba8 InterpolateRGB(Rgba8 start, Rgba8 end, float fractionOfEnd)
{
	float r = Interpolate(NormalizeByte(start.r), NormalizeByte(end.r), fractionOfEnd);
	float g = Interpolate(NormalizeByte(start.g), NormalizeByte(end.g), fractionOfEnd);
	float b = Interpolate(NormalizeByte(start.b), NormalizeByte(end.b), fractionOfEnd);
	return Rgba8(DenormalizeByte(r), DenormalizeByte(g), DenormalizeByte(b), 255);
}

Rgba8 InterpolateRGBA(Rgba8 start, Rgba8 end, float fractionOfEnd)
{
	float r = Interpolate(NormalizeByte(start.r), NormalizeByte(end.r), fractionOfEnd);
	float g = Interpolate(NormalizeByte(start.g), NormalizeByte(end.g), fractionOfEnd);
	float b = Interpolate(NormalizeByte(start.b), NormalizeByte(end.b), fractionOfEnd);
	float a = Interpolate(NormalizeByte(start.a), NormalizeByte(end.a), fractionOfEnd);
	return Rgba8(DenormalizeByte(r), DenormalizeByte(g), DenormalizeByte(b), DenormalizeByte(a));
}

float GetFractionWithinRange(float value, float rangeStart, float rangeEnd)
{
	float disp = rangeEnd - rangeStart;
	if ( rangeStart != rangeEnd )
	{
		float proportion = (value - rangeStart) / disp;
		return proportion;
	}
	else
	{
		return .5f;
	}
}

float RangeMap(float inValue, float inStart, float inEnd, float outStart, float outEnd)
{
	float proportion = GetFractionWithinRange(inValue, inStart, inEnd);

	float outValue = Interpolate( outStart, outEnd, proportion );
	return outValue;
}

float RangeMapClamped(float inValue, float inStart, float inEnd, float outStart, float outEnd)
{
	float proportion = GetFractionWithinRange(inValue, inStart, inEnd);
	proportion = GetClamped(proportion, 0.f, 1.f);

	float outValue = Interpolate(outStart, outEnd, proportion);
	return outValue;
}

int RoundDownToInt(float value)
{
	if ( value >= 0 )
	{
		return static_cast<int>( floorf(value));
	}
	else
	{
		return static_cast<int>( -(floorf(-value) + 1));
	}
}

Quat ConvertEulerAnglesToQuat(EulerAngles e)
{
	float cy = cosf(ConvertDegreesToRadians(e.m_yawDegrees) * 0.5f);
	float sy = sinf(ConvertDegreesToRadians(e.m_yawDegrees ) * 0.5f);
	float cp = cosf(ConvertDegreesToRadians(e.m_pitchDegrees) * 0.5f);
	float sp = sinf(ConvertDegreesToRadians(e.m_pitchDegrees) * 0.5f);
	float cr = cosf(ConvertDegreesToRadians(e.m_rollDegrees ) * 0.5f);
	float sr = sinf(ConvertDegreesToRadians(e.m_rollDegrees ) * 0.5f);

	Quat q;
	q.w = cr * cp * cy + sr * sp * sy;
	q.x = sr * cp * cy - cr * sp * sy;
	q.y = cr * sp * cy + sr * cp * sy;
	q.z = cr * cp * sy - sr * sp * cy;

	q = q.GetNormalized();

	return q;
}

EulerAngles ConvertQuatToEulerAngles(Quat const& quat)
{
	EulerAngles euler;

	Quat q = quat.GetNormalized();

	// Roll (X-axis rotation)
	float sinr_cosp = 2.f * (q.w * q.x + q.y * q.z);
	float cosr_cosp = 1.f - 2.f * (q.x * q.x + q.y * q.y);
	euler.m_rollDegrees = ConvertRadiansToDegrees(atan2f(sinr_cosp, cosr_cosp));

	// Pitch (Y-axis rotation)
	float sinp = 2.f * (q.w * q.y - q.z * q.x);
	float cosp = 1.f - 2.f * (q.y * q.y + q.x * q.x);
	euler.m_pitchDegrees = ConvertRadiansToDegrees(atan2f(sinp, cosp));

	// Yaw (Z-axis rotation)
	float siny_cosp = 2.f * (q.w * q.z + q.x * q.y);
	float cosy_cosp = 1.f - 2.f * (q.y * q.y + q.z * q.z);
	euler.m_yawDegrees = ConvertRadiansToDegrees(atan2f(siny_cosp, cosy_cosp));

	return euler;
}

EulerAngles ConvertQuatToEulerAngles(float x, float y, float z, float w)
{
	EulerAngles euler;

	// Roll (X-axis rotation)
	float sinr_cosp = 2.f * (w * x + y * z);
	float cosr_cosp = 1.f - 2.f * (x * x + y * y);
	euler.m_rollDegrees = ConvertRadiansToDegrees(atan2f(sinr_cosp, cosr_cosp));

	// Pitch (Y-axis rotation)
	float sinp = 2.f * (w * y - z * x);
	if (abs(sinp) >= 1.f)
		euler.m_pitchDegrees = ConvertRadiansToDegrees(copysignf(PI / 2.f, sinp)); // Use 90 degrees if out of range
	else
		euler.m_pitchDegrees = ConvertRadiansToDegrees(asinf(sinp));

	// Yaw (Z-axis rotation)
	float siny_cosp = 2.f * (w * z + x * y);
	float cosy_cosp = 1.f - 2.f * (y * y + z * z);
	euler.m_yawDegrees = ConvertRadiansToDegrees(atan2f(siny_cosp, cosy_cosp));

	return euler;
}

float DotProduct2D(Vec2 const& a, Vec2 const& b)
{
	return (a.x * b.x) + (a.y * b.y);
}

float DotProductQuat(Quat const& q1, Quat const& q2)
{
	float dot = q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w;
	return dot;
}

float DotProduct3D(Vec3 const& a, Vec3 const& b)
{
	return ((a.x * b.x) + (a.y * b.y) + (a.z * b.z));
}

float DotProduct4D(Vec4 const& copyOfMatrix, Vec4 const& appendMatrix)
{
	return ((copyOfMatrix.x * appendMatrix.x) + (copyOfMatrix.y * appendMatrix.y) + (copyOfMatrix.z * appendMatrix.z) + (copyOfMatrix.w * appendMatrix.w));
}

float DotProduct4D(Quat const& q1, Quat const& q2)
{
	return ((q1.x * q2.x) + (q1.y * q2.y) + (q1.z * q2.z) + (q1.w * q2.w));
}

float CrossProduct2D(Vec2 a, Vec2 b)
{
	return ((a.x * b.y) - (a.y * b.x));
}

Vec3 CrossProduct3D(Vec3 const& a, Vec3 const& b)
{
	return Vec3((a.y * b.z - a.z * b.y), (a.z * b.x - a.x * b.z), (a.x * b.y - a.y * b.x));
}

bool CheckIfTwoVec3AreInTheSameDirection(Vec3 const& a, Vec3 const& b)
{
	float dotProduct3D = DotProduct3D(a, b);
	return (dotProduct3D * dotProduct3D) == (GetDistanceSquared3D(a, Vec3()) * GetDistanceSquared3D(b, Vec3()));
}

float ConvertDegreesToRadians(float degrees)
{
	return degrees * (PI / 180.f);
}

float ConvertRadiansToDegrees(float Radians)
{
	return Radians * (180.f / PI);
}

// both cosf and sinf takes radian
float CosDegrees(float degrees)
{
	return cosf(degrees * (PI / 180));
}
// sinf is calculating floats, sin is for double
float SinDegrees(float degrees)
{
	return sinf(degrees * (PI / 180));
}

float Atan2Degrees(float y, float x)
{
	float theta_Radian = atan2f(y, x);
	return ConvertRadiansToDegrees(theta_Radian);
}

float GetShortestAngularDispDegrees(float startDegrees, float endDegrees)
{
	float disp = endDegrees - startDegrees;

	// keep the disp with 180 degrees
	while (disp > 180.f)
	{
		disp -= 360.f;
	}
	while (disp < -180.f)
	{
		disp += 360.f;
	}
	return disp;
}

float GetTurnedTowardDegrees(float currentDegrees, float goalDegrees, float maxDeltaDegrees)
{
	// keep the current degrees under 360 for final result
	while (currentDegrees > 360.f)
	{
		currentDegrees -= 360.f;
	}
	while (currentDegrees < -360.f)
	{
		currentDegrees += 360.f;
	}

	float disp = GetShortestAngularDispDegrees(currentDegrees, goalDegrees);
	if (disp > 0)
	{
		if (disp > maxDeltaDegrees)
		{
			return(currentDegrees + maxDeltaDegrees);
		}
		else return (currentDegrees + disp);
	}
	else if (disp < 0)
	{
		if (-disp > maxDeltaDegrees)
		{
			return ( currentDegrees - maxDeltaDegrees );
		}
		else return  (currentDegrees + disp);
	}
	// if disp == 0
	else return goalDegrees;
}

// get an absolute value for the degrees in between
float GetAngleDegreesBetweenVectors2D(Vec2 const& a, Vec2 const& b)
{
	// this method is much more slower and the result mey below zero
	// float result = b.GetOrientationDegrees() - a.GetOrientationDegrees();
	// if (result < 0.f)
	// {
	// 	return -result;
	// }
	// return result;

	float dotProductAB = DotProduct2D(a, b);
	float lengthA = a.GetLength();
	float lengthB = b.GetLength();
	if (lengthA != 0.f && lengthB != 0.f)
	{
		float cosValue = dotProductAB / (lengthA * lengthB);
		cosValue = GetClamped(cosValue, -1.f, 1.f);
		float thetaAB = acosf(cosValue); // result has to translate from radians to degrees
		thetaAB *= (180.f / PI);
		return thetaAB;
	}
	else
	{
		return 0.f;
	}
}

Vec3 GetRandomDirectionInCone(Vec3 direction, float range)
{
	FloatRange yawRange(-range, range);
	FloatRange pitchRange(-range, range);

	float yawDeflection = g_rng->RollRandomFloatInFloatRange(yawRange);
	float pitchDeflection = g_rng->RollRandomFloatInFloatRange(pitchRange);

	EulerAngles orientation = direction.GetOrientation();
	orientation.m_yawDegrees += yawDeflection;
	orientation.m_pitchDegrees += pitchDeflection;

	return Vec3::GetDirectionForYawPitch(orientation.m_yawDegrees, orientation.m_pitchDegrees);
}

float GetDistance2D(Vec2 const& posA, Vec2 const& posB)
{
	return (posA - posB).GetLength();
}

float GetDistanceSquared2D(Vec2 const& posA, Vec2 const& posB)
{
	return (posA - posB).GetLengthSquared();
}

float GetDistance3D(Vec3 const& posA, Vec3 const& posB)
{
	return (posA - posB).GetLength();
}

float GetDistanceSquared3D(Vec3 const& posA, Vec3 const& posB)
{
	return (posA - posB).GetLengthSquared();
}

float GetDistanceXY3D(Vec3 const& posA, Vec3 const& posB)
{
	return GetDistance2D(Vec2 (posA.x, posA.y), Vec2 (posB.x, posB.y));
}

float GetDistanceXYSquared3D(Vec3 const& posA, Vec3 const& posB)
{
	return GetDistanceSquared2D(Vec2(posA.x, posA.y), Vec2(posB.x, posB.y));
}

int GetTaxicabDistance2D(IntVec2 const& pointA, IntVec2 const& pointB)
{
	IntVec2 disp = pointB - pointA;
	return (abs(disp.x) + abs(disp.y));
}

float GetProjectedLength2D(Vec2 const& vectorToProject, Vec2 const& vectorToProjectOnto)
{
	Vec2 normalToProject = vectorToProjectOnto.GetNormalized();
	return DotProduct2D(vectorToProject, normalToProject);
}

Vec2 const GetProjectedOnto2D(Vec2 const& vectorToProject, Vec2 const& vectorToProjectOnto)
{
	float projectedVectorLength = GetProjectedLength2D(vectorToProject, vectorToProjectOnto);
	Vec2 normalToProject = vectorToProjectOnto.GetNormalized();
	return Vec2(normalToProject * projectedVectorLength);
}

bool IsPointInsideDisc2D(Vec2 const& point, Vec2 const& discCenter, float discRadius)
{
	Vec2 disp = discCenter - point;
	float length = disp.GetLength();
	if (length < discRadius)
	{
		return true;
	}
	else return false;
}

bool IsPointInsideOrientedSector2D(Vec2 const& point, Vec2 const& sectorTip, float sectorForwardDegrees, float sectorApertureDegrees, float sectorRadius)
{
	// see if the point is out of pizza
	Vec2 disp = point - sectorTip;
	float length = disp.GetLength();
	if (length >= sectorRadius)
	{
		return false;
	}

	// calculate the angle between the point to the sector normal
	Vec2 normal = normal.MakeFromPolarDegrees(sectorForwardDegrees, 1.f);
	Vec2 tipToPoint = point - sectorTip;
	float anglePToNormal = GetAngleDegreesBetweenVectors2D(tipToPoint, normal);
	if (anglePToNormal < sectorApertureDegrees * 0.5f)
	{
		return true;
	}

	return false;
}

// the sectorForwardNormal divide the sector in half and it is pointing towards the part it occupies
bool IsPointInsideOrientedSector2D(Vec2 const& point, Vec2 const& sectorTip, Vec2 const& sectorForwardNormal, float sectorApertureDegrees, float sectorRadius)
{
	// see if the point is out of pizza
	Vec2 disp = point - sectorTip;
	float length = disp.GetLength();
	if (length >= sectorRadius)
	{
		return false;
	}

	// calculate the angle between the point to the sector normal
	Vec2 tipToPoint = point - sectorTip;
	float anglePToNormal = GetAngleDegreesBetweenVectors2D(tipToPoint, sectorForwardNormal);
	if (anglePToNormal < sectorApertureDegrees * 0.5f)
	{
		return true;
	}

	return false;
}

bool IsPointInsideCapsule2D(Vec2 const& point, Vec2 const& boneStart, Vec2 const& boneEnd, float radius)
{
	Capsule2 capsule(boneStart, boneEnd, radius);
	Vec2 nearestPointOnBone = GetNearestPointOnLineSegment2D(point, capsule.m_start, capsule.m_end);
	return IsPointInsideDisc2D(point, nearestPointOnBone, capsule.m_radius);
}

bool IsPointInsideCapsule2D(Vec2 const& point, Capsule2 capsule)
{
	Vec2 nearestPointOnBone = GetNearestPointOnLineSegment2D(point, capsule.m_start, capsule.m_end);
	return IsPointInsideDisc2D(point, nearestPointOnBone, capsule.m_radius);
}

bool IsPointInsideAABB2D(Vec2 const& referencePos, AABB2 const& box)
{
	Vec2 center = (box.m_mins + box.m_maxs) * 0.5f;
	float halfWidth = (box.m_maxs.x - box.m_mins.x) * 0.5f;
	float halfHeight = (box.m_maxs.y - box.m_mins.y) * 0.5f;
	if (referencePos.x >= center.x + halfWidth)
	{
		return false;
	}
	if (referencePos.x <= center.x - halfWidth)
	{
		return false;
	}
	if (referencePos.y >= center.y + halfHeight)
	{
		return false;
	}
	if (referencePos.y <= center.y - halfHeight)
	{
		return false;
	}
	return true;
}

bool IsPointInsideOBB2D(Vec2 const& point, OBB2 const& orientedBox)
{
	Vec2 posInLocalSpace = orientedBox.GetLocalPosForWorldPos(point);
	float Pi = posInLocalSpace.x;
	float Pj = posInLocalSpace.y;
	if (Pi >= orientedBox.m_halfDimensions.x)
	{
		return false;
	}
	if (Pi <= -orientedBox.m_halfDimensions.x)
	{
		return false;
	}
	if (Pj >= orientedBox.m_halfDimensions.y)
	{
		return false;
	}
	if (Pj <= -orientedBox.m_halfDimensions.y)
	{
		return false;
	}
	return true;
}

bool DoTwoFloatRangeOverlap(float ARangeStart, float ARangeEnd, float BRangeStart, float BRangeEnd)
{
	FloatRange A(ARangeStart, ARangeEnd);
	FloatRange B(BRangeStart, BRangeEnd);
	if (A.IsOverLappingWith(B))
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool DoFloatInFloatRange(float inputFloat, float rangeStart, float rangeEnd)
{
	FloatRange range(rangeStart, rangeEnd);
	if (range.IsInRange(inputFloat))
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool DoDiscsOverlap(Vec2 const& centerA, float radiusA, Vec2 const& centerB, float radiusB)
{
	float disp_sqr = GetDistanceSquared2D(centerA, centerB);
	if (disp_sqr < (radiusA + radiusB) * (radiusA + radiusB))
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool DoDiscOverlapAABB2(Vec2 const& discCenter, float discRadius, AABB2 const box)
{
	Vec2 nearestPt = GetNearestPointOnAABB2D(discCenter, box);
	float disp_sqr = GetDistanceSquared2D(discCenter, nearestPt);
	if (disp_sqr < (discRadius * discRadius))
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool DoTwoAABB2Overlap(AABB2 A, AABB2 B)
{
	float AminX = A.m_mins.x;
	float AmaxX = A.m_maxs.x;
	float BminX = B.m_mins.x;
	float BmaxX = B.m_maxs.x;

	float AminY = A.m_mins.y;
	float AmaxY = A.m_maxs.y;
	float BminY = B.m_mins.y;
	float BmaxY = B.m_maxs.y;

	// we are making sure that two AABB2 has intersections in both X and Y float range
	FloatRange Ax(AminX, AmaxX);
	FloatRange Bx(BminX, BmaxX);
	FloatRange Ay(AminY, AmaxY);
	FloatRange By(BminY, BmaxY);

	if (Ax.IsOverLappingWith(Bx) && Ay.IsOverLappingWith(By))
	{
		return true;
	}
	else
	{
		return false;
	}
}

Vec2 const GetNearestPointOnDisc2D(Vec2 const& referencePosition, Vec2 const& disCenter, float discRadius)
{
	// if the point is inside the disc, return the point itself
	if (IsPointInsideDisc2D(referencePosition, disCenter, discRadius))
	{
		return referencePosition;
	}
	else
	{
		// if the point is out side of the point
		Vec2 disp = referencePosition - disCenter;
		disp.ClampLength(discRadius);
		return (disCenter + disp);
	}
}

bool PushDiscOutOfFixedPoint2D(Vec2& mobileDiscCenter, float discRadius, Vec2 const& fixedPoint)
{
	// if the point is not inside the disc, no need to push
	if ( !IsPointInsideDisc2D(fixedPoint, mobileDiscCenter, discRadius) )
	{
		return false;
	}

	Vec2 disp = mobileDiscCenter - fixedPoint;// get the direction
	float overlap = discRadius - disp.GetLength();// get the moving length
	disp.SetLength(overlap);// get the translation vector

	mobileDiscCenter += disp;
	return true;
}

bool PushDiscOutOfFixedDisc2D(Vec2& mobileDiscCenter, float mobileDiscRadius, Vec2 const& fixedDiscCenter, float fixedDiscRadius)
{
	Vec2 disp = mobileDiscCenter - fixedDiscCenter;
	float overlap = mobileDiscRadius + fixedDiscRadius - disp.GetLength();
	// if there is a gap between two disc, no need to push out
	if (overlap <= 0.f)
	{
		return false;
	}
	// else make the moving disp equal to the overlap length
	disp.SetLength(overlap);
	mobileDiscCenter += disp;
	return true;
}

bool PushDiscOutOfEachOther2D(Vec2& aCenter, float aRadius, Vec2& bCenter, float bRadius)
{
	Vec2 disp = bCenter - aCenter;
	float overlap = aRadius + bRadius - disp.GetLength();
	// if there is a gap between two disc, no need to push out
	if (overlap <= 0.f)
	{
		return false;
	}
	// Each disc have to move half way out
	disp.SetLength(overlap * 0.5f);
	bCenter += disp;
	aCenter += (disp * (-1));
	return true;
}

// true for 
bool PushDiscOutOfFixedAABB2D(Vec2& mobileDiscCenter, float discRadius, AABB2 const& fixedBox)
{
	// when the center is inside the AABB2
	if (fixedBox.IsPointInside(mobileDiscCenter))
	{
		return false;
	}
	// when the disc is not touching or overlap with the AABB2
	Vec2 nearestPoint = fixedBox.GetNearestPoint(mobileDiscCenter);
	Vec2 disp = nearestPoint - mobileDiscCenter;
	if (disp.GetLength() >= discRadius)
	{
		return false;
	}

	// we only consider the dis center is on the outside of the AABB2 box
	PushDiscOutOfFixedPoint2D(mobileDiscCenter, discRadius, nearestPoint);
	return true;
}

void TransformPosition2D(Vec2& pos, float uniformScale, float rotationDegrees, Vec2 const& translation)
{
	// transform the cartisian system into the polar system
	float R = sqrtf((pos.x * pos.x) + (pos.y * pos.y));
	float theta_Radian = atan2f(pos.y, pos.x);

	// the distance scales up according to scaleXY
	R *= uniformScale;

	// transform the radian into degree because we use degree for rotation input
	float theta_Degree = theta_Radian * (180.f / PI);

	// rotate the ship in polar system
	// the zRotationDegrees will be in degree instead of Radian because degree is more intuitive
	theta_Degree += rotationDegrees;
	theta_Radian = theta_Degree * (PI / 180.f);

	// change the polar system into the cartisian system
	// adding translation
	pos.x = R * cosf(theta_Radian) + translation.x;
	pos.y = R * sinf(theta_Radian) + translation.y;
}

void TransformPosition2D(Vec2& posToTransform, Vec2 const& iBasis, Vec2 const& jBasis, Vec2 const& translation)
{
	//Vec2 world_x = Vec2 (1.f, 0.f);
	//float thetaDegrees= posToTransform.GetOrientationRadians(); // the radians to the x pos axis
	//
	//float pLength = posToTransform.GetLength();
	//
	//// get local space x length and y length
	//float length_i = pLength * cosf(thetaDegrees);
	//float length_j = pLength * sinf(thetaDegrees);
	//
	//Vec2 worldPos = length_i * iBasis + length_j * jBasis + translation;
	Vec2 worldPos = posToTransform.x * iBasis + posToTransform.y * jBasis + translation;
	posToTransform = worldPos;
}

void TransformPositionAroundPosOnXY(Vec3& pos, float scaleXY, float zRotationDegrees, Vec2 const& translationXY)
{
	// transform the cartisian system into the polar system
	float R = sqrtf((pos.x * pos.x) + (pos.y * pos.y));
	float theta_Radian = atan2f(pos.y, pos.x);

	// the distance scales up according to scaleXY
	R *= scaleXY;

	// transform the radian into degree because we use degree for rotation input
	float theta_Degree = theta_Radian * (180.f / PI);

	// rotate the ship in polar system
	// the zRotationDegrees will be in degree instead of Radian because degree is more intuitive
	theta_Degree += zRotationDegrees;
	theta_Radian = theta_Degree * (PI / 180.f);

	// change the polar system into the cartisian system
	// adding translation
	pos.x = R * cosf(theta_Radian) + translationXY.x;
	pos.y = R * sinf(theta_Radian) + translationXY.y;

	// pos.z remains unchanged now because we are using this function for 2D game
}

void TransformPositionAroundPosOnXY(Vec3& positionToTransform, Vec2 const& iBasis, Vec2 const& jBasis, Vec2 const& translation)
{
	Vec3 translationIn3D = Vec3(translation.x, translation.y, 0.f);
	Vec3 iBasis3D = Vec3(iBasis.x, iBasis.y, 0.f);
	Vec3 jBasis3D = Vec3(jBasis.x, jBasis.y, 0.f);
	Vec3 kBasis3D = Vec3(0.f, 0.f, 1.f);
	float z = positionToTransform.z; // preserve the z of the position for the layer info

	Vec3 worldPos = positionToTransform.x * iBasis3D + positionToTransform.y * jBasis3D + translationIn3D;
	positionToTransform = worldPos;
	positionToTransform.z = z;
}

void TransformPositionAroundPosOnXY(Vec3& pos, float scaleXY /*= 1.f*/, Vec2 const& rotationCenterPos /*= Vec2()*/, float zRotationDegrees /*= 0.f*/, 
									Vec2 const& translationXY /*= Vec2()*/)
{
	// transform the cartisian system into the polar system
	float XDist = pos.x - rotationCenterPos.x;
	float YDist = pos.y - rotationCenterPos.y;
	float R = sqrtf((XDist * XDist) + (YDist * YDist));
	float theta_Radian = atan2f(YDist, XDist);

	// the distance scales up according to scaleXY
	R *= scaleXY;

	// transform the radian into degree because we use degree for rotation input
	float theta_Degree = theta_Radian * (180.f / PI);

	// rotate the ship in polar system
	// the zRotationDegrees will be in degree instead of Radian because degree is more intuitive
	theta_Degree += zRotationDegrees;
	theta_Radian = theta_Degree * (PI / 180.f);

	// change the polar system into the cartisian system
	// adding translation
	pos.x = R * cosf(theta_Radian) + rotationCenterPos.x + translationXY.x;
	pos.y = R * sinf(theta_Radian) + rotationCenterPos.y + translationXY.y;

	// pos.z remains unchanged now because we are using this function for 2D game
}

void TransformPositionAroundPosOnXY(Vec2& positionToTransform, float scaleXY /*= 1.f*/, Vec2 const& rotationCenterPos /*= Vec2()*/, float zRotationDegrees /*= 0.f*/, Vec2 const& translationXY /*= Vec2()*/)
{
	// transform the cartisian system into the polar system
	float XDist = positionToTransform.x - rotationCenterPos.x;
	float YDist = positionToTransform.y - rotationCenterPos.y;
	float R = sqrtf((XDist * XDist) + (YDist * YDist));
	float theta_Radian = atan2f(YDist, XDist);

	// the distance scales up according to scaleXY
	R *= scaleXY;

	// transform the radian into degree because we use degree for rotation input
	float theta_Degree = theta_Radian * (180.f / PI);

	// rotate the ship in polar system
	// the zRotationDegrees will be in degree instead of Radian because degree is more intuitive
	theta_Degree += zRotationDegrees;
	theta_Radian = theta_Degree * (PI / 180.f);

	// change the polar system into the cartisian system
	// adding translation
	positionToTransform.x = R * cosf(theta_Radian) + rotationCenterPos.x + translationXY.x;
	positionToTransform.y = R * sinf(theta_Radian) + rotationCenterPos.y + translationXY.y;
}

// for example get a byte from the server and normalize it into float from 0.f to 1.f
float NormalizeByte(unsigned char inputChar)
{
	float result = (float)inputChar;
	result = RangeMapClamped(result, 0.f, 255.f, 0.f, 1.f);
	return result;
}

// for example compress a local float to byte to send over serve, char only take 1 byte instead of 4
// make sure when byte is translated
unsigned char DenormalizeByte(float inputFloat)
{
	// Each byte (from 0 to 255) covers a similar "area" in the [0.f, 1.f] number line region,
	// which makes the same range of floats should Denormalize into byte 0 as do byte 1 or byte 2 or byte 254 or byte 255 (or any byte).
	// therefore we should divide the range from 0.f to 1.f of float into 255+1 pieces(because range 0 takes one)
	float step = 1.f / 256.f;	// from 0 
	int stepNum = (int)floorf(inputFloat / step);

	stepNum = GetClamped(stepNum, 0, 255); // in case that the input float might accidentally out of the range
	return (unsigned char)stepNum;

	// todo:??? why not divide the range into 255 pieces and only let 0.f denormalized into unsigned char 0
	// because current method makes that small float value denormalized and normalized back to 0, which is a special case
	// and let float 0.f become the only case that equals to unsigned char 0
}

BillboardType GetBillboardTypeFromString(std::string type)
{
	if (type == "WorldUpFacing")
	{
		return BillboardType::WORLD_UP_CAMERA_FACING;
	}
	if (type == "WorldUpOpposing")
	{
		return BillboardType::WORLD_UP_CAMERA_OPOSSING;
	}
	if (type == "FullFacing")
	{
		return BillboardType::FULL_CAMERA_FACING;
	}
	if (type == "FullOpposing")
	{
		return BillboardType::FULL_CAMERA_OPPOSING;
	}
	if (type == "None")
	{
		return BillboardType::NONE;
	}

	ERROR_AND_DIE("Did not find matching billboard type for the input string");
}

Vec2 const GetNearestPointOnAABB2D(Vec2 const& referencePos, AABB2 const& box)
{
	if (IsPointInsideAABB2D(referencePos, box))
	{
		return referencePos;
	}
	else
	{
		Vec2 nearestPoint;
		nearestPoint.x = GetClamped(referencePos.x, box.m_mins.x, box.m_maxs.x);
		nearestPoint.y = GetClamped(referencePos.y, box.m_mins.y, box.m_maxs.y);
		return nearestPoint;
	}
}

Vec2 const GetNearestPointOnOBB2D(Vec2 const& referencePos, OBB2 const& orientedBox)
{
	Vec2 N_WorldPos;// the nearest point on OBB2
	if (IsPointInsideOBB2D(referencePos, orientedBox))
	{
		N_WorldPos = referencePos;
		return N_WorldPos;
	}
	else
	{
		Vec2 posInLocalSpace = orientedBox.GetLocalPosForWorldPos(referencePos);
		float Pi = posInLocalSpace.x;
		float Pj = posInLocalSpace.y;

		Vec2 N_LocalPos;
		N_LocalPos.x = GetClamped(Pi, -orientedBox.m_halfDimensions.x, orientedBox.m_halfDimensions.x);
		N_LocalPos.y = GetClamped(Pj, -orientedBox.m_halfDimensions.y, orientedBox.m_halfDimensions.y);
		Vec2 jBasisNormal = orientedBox.m_iBasisNormal.GetRotated90Degrees();
		N_WorldPos = orientedBox.m_center + (N_LocalPos.x * orientedBox.m_iBasisNormal) + (N_LocalPos.y * jBasisNormal);
		return N_WorldPos;
	}
}

Vec2 const GetNearestPointOnLineSegment2D(Vec2 referencePos, LineSegment2 const ls)
{
	Vec2 SE = ls.m_end - ls.m_start;
	// reference point is located in region I
	Vec2 SP = referencePos - ls.m_start;
	if (DotProduct2D(SE, SP) <= 0)
	{
		return ls.m_start;
	}
	// reference point is located in region IIT
	Vec2 EP = referencePos - ls.m_end;
	if (DotProduct2D(EP, SE) >= 0)
	{
		return ls.m_end;
	}
	// reference point is located in region II
	Vec2 SN = GetProjectedOnto2D(SP, SE);
	return ls.m_start + SN;
}

Vec2 const GetNearestPointOnLineSegment2D(Vec2 referencePos, Vec2 const& lineSegStart, Vec2 const& lineSegEnd)
{
	Vec2 SE = lineSegEnd - lineSegStart;
	// reference point is located in region I
	Vec2 SP = referencePos - lineSegStart;
	if (DotProduct2D(SE, SP) <= 0)
	{
		return lineSegStart;
	}
	// reference point is located in region IIT
	Vec2 EP = referencePos - lineSegEnd;
	if (DotProduct2D(EP, SE) >= 0)
	{
		return lineSegEnd;
	}
	// reference point is located in region II
	Vec2 SN = GetProjectedOnto2D(SP, SE);
	return lineSegStart + SN;
}

Vec2 const GetNearestPointOnInfiniteLine2D(Vec2 referencePos, LineSegment2 const infiniteLine)
{
	// considering all reference is in region II
	Vec2 SE = infiniteLine.m_end - infiniteLine.m_start;
	Vec2 SP = referencePos - infiniteLine.m_start;
	Vec2 SN = GetProjectedOnto2D(SP, SE);
	return infiniteLine.m_start + SN;
}

Vec2 const GetNearestPointOnInfiniteLine2D(Vec2 referencePos, Vec2 const& pointOnLine, Vec2 const& anotherPointOnline)
{
	LineSegment2* infinteLine = new LineSegment2(pointOnLine, anotherPointOnline);
	return GetNearestPointOnInfiniteLine2D(referencePos, *infinteLine);
}

Vec2 const GetNearestPointOnCapsule2D(Vec2 referencePos, Capsule2 const& capsule)
{
	if (IsPointInsideCapsule2D(referencePos, capsule))
	{
		// if the point is in side the capsule
		return referencePos;
	}
	else
	{
		// if the point is out side the capsule
		// the nearest point on the capsule must be on the ring whose center is the nearest point on the bone
		Vec2 nearestPointOnBone = GetNearestPointOnLineSegment2D(referencePos, capsule.m_start, capsule.m_end);
		return GetNearestPointOnDisc2D(referencePos, nearestPointOnBone, capsule.m_radius);
	}
}

Vec2 const GetNearestPointOnCapsule2D(Vec2 referencePos, Vec2 const& boneStart, Vec2 const& boneEnd, float radius)
{
	if (IsPointInsideCapsule2D(referencePos, boneStart, boneEnd, radius))
	{
		// if the point is in side the capsule
		return referencePos;
	}
	else
	{
		Vec2 nearestPointOnBone = GetNearestPointOnLineSegment2D(referencePos, boneStart, boneEnd);
		// if the point is out side the capsule
		// the nearest point on the capsule must be on the ring whose center is the nearest point on the bone
		return GetNearestPointOnDisc2D(referencePos, nearestPointOnBone, radius);
	}

}

Vec2 const GetNearestPointOnSector2D(Vec2 referencePos, Vec2 const& sectorTip, float sectorForwardDegrees, float sectorApertureDegrees, float sectorRadius)
{
	Vec2 disp = referencePos - sectorTip;
	Vec2 sectorForwardNormal = sectorForwardNormal.MakeFromPolarDegrees(sectorForwardDegrees, 1.f);
	if (IsPointInsideOrientedSector2D(referencePos, sectorTip, sectorForwardDegrees, sectorApertureDegrees, sectorRadius))
	{
		return referencePos;
	}
	else
	{
		float dispAngle = GetAngleDegreesBetweenVectors2D(disp, sectorForwardNormal);
		float orientationDegrees = disp.GetOrientationDegrees();
		// the reference point locates in the outer pie zone
		if (dispAngle <= sectorApertureDegrees * 0.5f)
		{
			Vec2 dispToNearestPoint = dispToNearestPoint.MakeFromPolarDegrees(orientationDegrees, sectorRadius);
			return dispToNearestPoint + sectorTip;
		}
		else
		{
			float toLeftSide = GetShortestAngularDispDegrees( orientationDegrees, (sectorForwardDegrees + sectorApertureDegrees * 0.5f) );
			float toRightSide = GetShortestAngularDispDegrees( orientationDegrees, (sectorForwardDegrees - sectorApertureDegrees * 0.5f) );
			toLeftSide = fabsf(toLeftSide);
			toRightSide = fabsf(toRightSide);
			if (toLeftSide >= toRightSide)
			{
				Vec2 rightSideTip = sectorTip + referencePos.MakeFromPolarDegrees((sectorForwardDegrees - sectorApertureDegrees * 0.5f), sectorRadius);
				return GetNearestPointOnLineSegment2D(referencePos, sectorTip, rightSideTip);
			}
			else
			{
				Vec2 leftSideTip = sectorTip + referencePos.MakeFromPolarDegrees((sectorForwardDegrees + sectorApertureDegrees * 0.5f), sectorRadius);
				return GetNearestPointOnLineSegment2D(referencePos, sectorTip, leftSideTip);
			}
		}
	}
}

Vec2 const GetNearestPointOnSector2D(Vec2 referencePos, Vec2 const& sectorTip, Vec2 const& sectorForwardNormal, float sectorApertureDegrees, float sectorRadius)
{
	float sectorForwardDegrees = sectorForwardNormal.GetOrientationDegrees();
	return GetNearestPointOnSector2D(referencePos, sectorTip, sectorForwardDegrees, sectorApertureDegrees, sectorRadius);
}

Mat44 GetBillboardMatrix(BillboardType billboardType, Mat44 const& cameraMatrix, Vec3 const& billboardPosition, Vec3 const& billboardScale /*= Vec3(1.f, 1.f, 1.f)*/)
{
	// Get the target information
	Vec3 It = cameraMatrix.GetIBasis3D();
	Vec3 Jt = cameraMatrix.GetJBasis3D();
	Vec3 Kt = cameraMatrix.GetKBasis3D();
	Vec3 Tt = cameraMatrix.GetTranslation3D();

	// each mode will modify three basis as different values
	Vec3 Ib;
	Vec3 Jb;
	Vec3 Kb;
	Vec3 Tb = billboardPosition;

	// world space info
	Vec3 worldY(0.f, 1.f, 0.f);
	Vec3 worldZ(0.f, 0.f, 1.f);

	switch (billboardType)
	{
	case BillboardType::WORLD_UP_CAMERA_FACING:
	{
		Kb = worldZ;
		Ib = (Tt - Tb).GetNormalized();
		if (fabs(DotProduct3D(Ib, worldZ)) != 1.f)
		{
			Ib.z = 0.f;
		}
		else
		{
			// don't billboard
		}
		Ib = Ib.GetNormalized();
		Jb = Vec3(-Ib.y, Ib.x, 0.f); // IbBasis rotated 90 around Z
	}break;
	case BillboardType::WORLD_UP_CAMERA_OPOSSING:
	{
		Kb = worldZ;
		Ib = It * (-1.f);
		if (fabs(DotProduct3D(Ib, worldZ)) != 1.f)
		{
			Ib.z = 0.f;
		}
		else
		{
			// don't billboard
		}
		Ib = Ib.GetNormalized();
		Jb = CrossProduct3D(Kb, Ib);
	}break;
	case BillboardType::FULL_CAMERA_FACING:
	{
		Ib = (Tt - billboardPosition).GetNormalized();

		// check if the billboard it facing towards the world Z direction
		if (abs(DotProduct3D(Ib, worldZ) != 1.f))
		{
			Jb = CrossProduct3D(worldZ, Ib).GetNormalized();
			Kb = CrossProduct3D(Ib, Jb);
		}
		else// if the billboard is facing the sky
		{
			Kb = CrossProduct3D(Ib, worldY); // todo: why???
			Jb = CrossProduct3D(Kb, Ib);
		}
	}break;
	case BillboardType::FULL_CAMERA_OPPOSING:
	{
		Ib = It * (-1.f);
		Jb = Jt * (-1.f);
		Kb = Kt;
	}break;
	case BillboardType::COUNT:
		break;
	default:
		break;
	}

	// append uniform scale
	Mat44 result(Ib, Jb, Kb, billboardPosition);
	Mat44 scale = Mat44::CreateNonUniformScale3D(billboardScale);
	result.Append(scale);
	return result;
}

Vec3 GetNearestPointOnZCylinder(Vec3 point, ZCylinder cylinder)
{
	Vec3 nearestPt;

	if (IsPointInsideZCylinder(point, cylinder))
	{
		nearestPt = point;
	}
	else
	{
		//Vec2 ptXY(point.x, point.y);
		//if (IsPointInsideDisc2D(ptXY, cylinder.CenterXY, cylinder.Radius))
		//{
		//	if (point.z >= cylinder.MinMaxZ.m_max)
		//	{
		//		nearestPt.x = point.x;
		//		nearestPt.y = point.y;
		//		nearestPt.z = cylinder.MinMaxZ.m_max;
		//		return nearestPt;
		//	}
		//	else
		//	{
		//		nearestPt.x = point.x;
		//		nearestPt.y = point.y;
		//		nearestPt.z = cylinder.MinMaxZ.m_max;
		//		return nearestPt;
		//	}
		//}

		// first clamp the point on disc2D
		Vec2 disp = Vec2(point.x, point.y) - cylinder.CenterXY;
		float length = disp.GetLength();
		length = GetClampedByFloatRange(length, FloatRange(0.f, cylinder.Radius));
		Vec2 direction = disp.GetNormalized();
		
		Vec2 pt = cylinder.CenterXY + (direction * length);
		nearestPt.x = pt.x;
		nearestPt.y = pt.y;

		// then clamp the point on float range
		nearestPt.z = GetClampedByFloatRange(point.z, cylinder.MinMaxZ);
	}
	return nearestPt;
}

Vec3 GetNearestPointOnAABB3(Vec3 point, AABB3 box)
{
	Vec3 nearestPt;

	if (IsPointInsideAABB3(point, box))
	{
		nearestPt = point;
	}
	else
	{
		nearestPt.x = GetClamped(point.x, box.m_mins.x, box.m_maxs.x);
		nearestPt.y = GetClamped(point.y, box.m_mins.y, box.m_maxs.y);
		nearestPt.z = GetClamped(point.z, box.m_mins.z, box.m_maxs.z);
	}
	return nearestPt;
}

Vec3 GetNearestPointOnOBB3(Vec3 const& point, OBB3 const& box)
{
	Vec3 nearestPt;

	if (IsPointInsideOBB3(point, box))
	{
		nearestPt = point;
	}
	else
	{
		// transfer all the raycast info into OBB3 local space and test the raycast vs AABB3
		Mat44 worldToLocalMat = box.GetModelMatrix().GetOrthonormalInverse();
		Vec3 localpt = worldToLocalMat.TransformPosition3D(point);

		// Get the OBB3 in local space, which is a AABB3
		Vec3 mins = box.m_halfDimensions * -1.f;
		Vec3 maxs = box.m_halfDimensions;
		AABB3 OBB3InLocal(mins, maxs);
		nearestPt = GetNearestPointOnAABB3(localpt, OBB3InLocal);

		// transform the result into world 
		Mat44 localToWorldMat = box.GetModelMatrix();
		nearestPt = localToWorldMat.TransformPosition3D(nearestPt);
	}
	return nearestPt;
}

Vec3 GetNearestPointOnPlane(Vec3 const& point, Plane3 const& plane)
{
	float refPosAltitude = plane.GetAltitudeOfPoint(point);
	Vec3 disp = refPosAltitude * (-plane.m_normal);
	return (point + disp);
}

Vec3 GetNearestPointOnSphere(Vec3 point, Sphere sphere)
{
	Vec3 nearestPoint;

	if (IsPointInsideSphere(point, sphere))
	{
		nearestPoint = point;
	}
	else
	{
		Vec3 disp = point - sphere.Center;
		float t = sphere.Radius / disp.GetLength();
		nearestPoint = sphere.Center + disp * t;
	}
	return nearestPoint;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
bool IsPointInsideAABB3(Vec3 point, AABB3 box)
{
	FloatRange rangeX(box.m_mins.x, box.m_maxs.x);
	FloatRange rangeY(box.m_mins.y, box.m_maxs.y);
	FloatRange rangeZ(box.m_mins.z, box.m_maxs.z);

	if (rangeX.IsInRange(point.x) &&
		rangeY.IsInRange(point.y) &&
		rangeZ.IsInRange(point.z))
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool IsPointInsideSphere(Vec3 point, Sphere sphere)
{
	Vec3 disp = point - sphere.Center;
	// not get length for performance optimization
	float lengthSq = disp.GetLengthSquared();
	float radiusSq = sphere.Radius * sphere.Radius;
	if ( lengthSq < radiusSq)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool IsPointInsideZCylinder(Vec3 point, ZCylinder cylinder)
{
	bool isInDiscXY;
	isInDiscXY = IsPointInsideDisc2D(Vec2(point.x, point.y), cylinder.CenterXY, cylinder.Radius);

	bool isInFloatRange;
	isInFloatRange = cylinder.MinMaxZ.IsInRange(point.z);

	if (isInDiscXY && isInFloatRange)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool IsPointInsideOBB3(Vec3 const& point, OBB3 const& targetbox)
{
	// transfer all the raycast info into OBB3 local space and test the raycast vs AABB3
	Mat44 worldToLocalMat = targetbox.GetModelMatrix().GetOrthonormalInverse();
	Vec3 localpt = worldToLocalMat.TransformPosition3D(point);

	// Get the OBB3 in local space, which is a AABB3
	Vec3 mins = targetbox.m_halfDimensions * -1.f;
	Vec3 maxs = targetbox.m_halfDimensions;
	AABB3 OBB3InLocal(mins, maxs);
	
	return IsPointInsideAABB3(localpt, OBB3InLocal);
}

bool DoAABB3sOverlap(AABB3 const& boxA, AABB3 const& boxB)
{
	FloatRange Ax(boxA.m_mins.x, boxA.m_maxs.x);
	FloatRange Ay(boxA.m_mins.y, boxA.m_maxs.y);
	FloatRange Az(boxA.m_mins.z, boxA.m_maxs.z);

	FloatRange Bx(boxB.m_mins.x, boxB.m_maxs.x);
	FloatRange By(boxB.m_mins.y, boxB.m_maxs.y);
	FloatRange Bz(boxB.m_mins.z, boxB.m_maxs.z);

	// if there is overlap in X, y and z view, the two boxes do overlap
	if (Ax.IsOverLappingWith(Bx) && Ay.IsOverLappingWith(By) && Az.IsOverLappingWith(Bz))
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool DoSpheresOverlap3D(Sphere A, Sphere B)
{
	float& Ra = A.Radius;
	float& Rb = B.Radius;
	float disp_Sqr = GetDistanceSquared3D(A.Center, B.Center);
	if (disp_Sqr < ((Ra + Rb) * (Ra + Rb)))
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool DoSpheresOverlap3D(Vec3 const& centerA, float radiusA, Vec3 const& centerB, float radiusB)
{
	float disp_Sqr = GetDistanceSquared3D(centerA, centerB);
	if (disp_Sqr < ((radiusA + radiusB) * (radiusA + radiusB)))
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool DoZCylindersOverlap3D(ZCylinder A, ZCylinder B)
{
	// first check if two cylinder's height float range has intersections
	if (A.MinMaxZ.IsOverLappingWith(B.MinMaxZ))
	{
		// secondly, check if two discs overlap in XY
		if (DoDiscsOverlap(A.CenterXY, A.Radius, B.CenterXY, B.Radius))
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
}

bool DoZCylindersOverlap3D(Vec2 cylinder1CenterXY, float cylinder1Radius, FloatRange cylinder1MinMaxZ, Vec2 cylinder2CenterXY, float cylinder2Radius, FloatRange cylinder2MinMaxZ)
{
	if (cylinder1MinMaxZ.IsOverLappingWith(cylinder2MinMaxZ))
	{
		// secondly, check if two discs overlap in XY
		if (DoDiscsOverlap(cylinder1CenterXY, cylinder1Radius, cylinder2CenterXY, cylinder2Radius))
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
}

bool DoSphereAndAABBOverlap3D(Vec3 sphereCenter, float sphereRadius, AABB3 box)
{
	Vec3 nearestPt = GetNearestPointOnAABB3(sphereCenter, box);
	float disp_Sqr = GetDistanceSquared3D(sphereCenter, nearestPt);
	if (disp_Sqr < (sphereRadius * sphereRadius))
	{
		// DebugAddWorldPoint(nearestPt, 0.25f, -1.f, Rgba8::LIGHT_ORANGE, Rgba8::LIGHT_ORANGE, DebugRenderMode::X_RAY);
		// DebugAddWorldPoint(sphereCenter, 0.25f, -1.f, Rgba8::LIGHT_ORANGE, Rgba8::LIGHT_ORANGE, DebugRenderMode::X_RAY);
		// DebugAddWorldWireSphere(sphereCenter, 5.f, -1.f, Rgba8::LIGHT_ORANGE, Rgba8::LIGHT_ORANGE, DebugRenderMode::X_RAY);
		return true;
	}
	else
	{
		return false;
	}
}

bool DoSphereAndAABBOverlap3D(Sphere sphere, AABB3 box)
{
	return DoSphereAndAABBOverlap3D(sphere.Center, sphere.Radius, box);
}

bool DoZCylinderAndAABBOverlap3D(Vec2 cylinderCenterXY, float cylinderRadius, FloatRange cylinderMinMaxZ, AABB3 box)
{
	// overlap need two shapes have intersections in both top view and side view
	bool heightOverlap = false;
	bool discOverlapAABB2D = false;

	// side view check
	FloatRange boxHeightRange(box.m_mins.z, box.m_maxs.z);

	if (cylinderMinMaxZ.IsOverLappingWith(boxHeightRange))
	{
		heightOverlap = true;
	}

	// check top view
	AABB2 box2D(Vec2(box.m_mins.x, box.m_mins.y), Vec2(box.m_maxs.x, box.m_maxs.y));
	if (DoDiscOverlapAABB2(cylinderCenterXY, cylinderRadius, box2D))
	{
		discOverlapAABB2D = true;
	}

	if (heightOverlap && discOverlapAABB2D)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool DoZCylinderAndAABBOverlap3D(ZCylinder cylinder, AABB3 box)
{
	return DoZCylinderAndAABBOverlap3D(cylinder.CenterXY, cylinder.Radius, cylinder.MinMaxZ, box);
}

bool DoZCylinderAndSphereOverlap3D(Vec2 cylinderCenterXY, float cylinderRadius, FloatRange cylinderMinMaxZ, Vec3 sphereCenter, float sphereRadius)
{
	ZCylinder cylinder(cylinderCenterXY, cylinderRadius, cylinderMinMaxZ);
	Vec3 nearestPt = GetNearestPointOnZCylinder(sphereCenter, cylinder);
	float dispSqr = GetDistanceSquared3D(sphereCenter, nearestPt);
	if (dispSqr < sphereRadius * sphereRadius)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool DoZCylinderAndSphereOverlap3D(ZCylinder cylinder, Sphere sphere)
{
	return DoZCylinderAndSphereOverlap3D(cylinder.CenterXY, cylinder.Radius, cylinder.MinMaxZ, sphere.Center, sphere.Radius);
}

bool DoPlaneIntersectSphere(Plane3 plane, Sphere sphere)
{
	Vec3 nearestPtOnPlane = GetNearestPointOnPlane(sphere.Center, plane);
	float dist = GetDistance3D(nearestPtOnPlane, sphere.Center);
	if (dist < sphere.Radius)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool DoPlaneIntersectAABB3(Plane3 plane, AABB3 box)
{
	// we need to check whether the eight point is all on the same side of the plane
	int sides[8];
	Vec3 pts[8];
	box.GetCornerPoints(pts);
	for (int i = 0; i < 8; ++i)
	{
		if (plane.IfThePointIsOnThePlane(pts[i]))
		{
			sides[i] = 0;
			continue;
		}
		else
		{
			if (plane.IfThePointIsInFrontOfPlane(pts[i]))
			{
				sides[i] = 1;
			}
			else
			{
				sides[i] = -1;
			}
		}
	}

	// start from the first result that is not 0
	int result = 0;
	for (int i = 0; i < 8; ++i)
	{
		if (sides[i] != 0)
		{
			result = sides[i];
		}
	}

	if (result == 0) // all points is on the plane
	{
		ERROR_AND_DIE("all points of the AABB3 is on the plane");
	}

	// if the result is -1, we want to check if there is any 1
	if (result == -1)
	{
		for (int i = 0; i < 8; ++i)
		{
			result = -1;

			if (sides[i] != 0)
			{
				result *= sides[i];
				if (result == -1)
				{
					return true;
				}
			}
		}

		return false;
	}

	// if the result is 1, we want to check if there is any -1
	if (result == 1)
	{
		for (int i = 0; i < 8; ++i)
		{
			result = 1;

			if (sides[i] != 0)
			{
				result *= sides[i];
				if (result == -1)
				{
					return true;
				}
			}
		}

		return false;
	}

	return false;
}

bool DoPlaneIntersectOBB3(Plane3 plane, OBB3 box)
{
	// we need to check whether the eight point is all on the same side of the plane
	int sides[8];
	Vec3 pts[8];
	box.GetCornerPoints(pts);
	for (int i = 0; i < 8; ++i)
	{
		if (plane.IfThePointIsOnThePlane(pts[i]))
		{
			sides[i] = 0;
			continue;
		}

		if (plane.IfThePointIsInFrontOfPlane(pts[i]))
		{
			sides[i] = 1;
		}
		else
		{
			sides[i] = -1;
		}
	}

	// start from the first result that is not 0
	int result = 0;
	for (int i = 0; i < 8; ++i)
	{
		if (sides[i] != 0)
		{
			result = sides[i];
		}
	}

	if (result == 0) // all points is on the plane
	{
		ERROR_AND_DIE("all points of the AABB3 is on the plane");
	}

	// if the result is -1, we want to check if there is any -1
	if (result == -1)
	{
		for (int i = 0; i < 8; ++i)
		{
			result = -1;
			if (sides[i] != 0)
			{
				result *= sides[i];
				if (result == -1)
				{
					return true;
				}
			}
		}

		return false;
	}

	// if the result is 1, we want to check if there is any -1
	if (result == 1)
	{
		for (int i = 0; i < 8; ++i)
		{
			result = 1;
			if (sides[i] != 0)
			{
				result *= sides[i];
				if (result == -1)
				{
					return true;
				}
			}
		}

		return false;
	}

	return false;
}
