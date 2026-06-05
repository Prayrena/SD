#pragma once
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/Vec4.hpp"
#include "Engine/core/Rgba8.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Math/OBB2.hpp"
#include "Engine/Math/OBB3.hpp"
#include "Engine/Math/Plane3.hpp"
#include "Engine/Math/Capsule2.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Math/LineSegment2.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Math/Quat.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/core/EngineCommon.hpp"
#include <string>

// this is global math function library
//----------------------------------------------------------------------------------------------------------------------------------------------------
// template
template <typename T>
T GetClamped(T value, T minValue, T maxValue);

// Specialization for Vec3
template <>
Vec3 GetClamped<Vec3>(Vec3 value, Vec3 minValue, Vec3 maxValue);

template <typename Container, typename Predicate>
void erase_if(Container& container, Predicate predicate)
{
	container.erase
	(
		std::remove_if(container.begin(), container.end(), predicate),
		container.end()
	);
}

//-----------------------------------------------------------------------------------------------------------------------------
// Clamp and lerp
int GetMax(int a, int b);

float GetClamped(float value, float minValue, float maxValue);

float GetClampedByFloatRange(float value, FloatRange range);
int	  GetClamped(int value, int minValue, int maxValue);

float GetClampedZeroToOne(float value);
float Interpolate(float start, float end, float fractionTowardEnd);
Vec2  Interpolate(Vec2 start, Vec2 end, float fractionTowardEnd);
Vec3  Interpolate(Vec3 start, Vec3 end, float fractionTowardEnd);
Rgba8 InterpolateRGB(Rgba8 start, Rgba8 end, float fractionOfEnd);//get a blend color between the start and end color
Rgba8 InterpolateRGBA(Rgba8 start, Rgba8 end, float fractionOfEnd);//get a blend color between the start and end color

float GetFractionWithinRange(float value, float rangeStart, float rangeEnd);
float RangeMap(float inValue, float inStart, float inEnd, float outStart, float outEnd);
float RangeMapClamped(float inValue, float inStart, float inEnd, float outStart, float outEnd);
int   RoundDownToInt(float value);

//----------------------------------------------------------------------------------------------------------------------------------------------------
// todo: converting functions
Quat		ConvertEulerAnglesToQuat(EulerAngles e);
EulerAngles ConvertQuatToEulerAngles(Quat const& q);
EulerAngles ConvertQuatToEulerAngles(float x, float y, float z, float w);

//-----------------------------------------------------------------------------------------------------------------------------
// dot and cross product
float DotProduct2D(Vec2 const& a, Vec2 const& b);// tell if two vectors are at same direction or not
float DotProductQuat(Quat const& q1, Quat const& q2);
float DotProduct3D(Vec3 const& a, Vec3 const& b);
float DotProduct4D(Vec4 const& copyOfMatrix, Vec4 const& appendMatrix);
float DotProduct4D(Quat const& q1, Quat const& q2);

float CrossProduct2D(Vec2 a, Vec2 b);// use to tell if the second vectors are turning right or left comparing to the first one
Vec3  CrossProduct3D(Vec3 const& a, Vec3 const& b);
bool  CheckIfTwoVec3AreInTheSameDirection(Vec3 const& a, Vec3 const& b);

//-----------------------------------------------------------------------------------------------------------------------------
// Angle utilities
float ConvertDegreesToRadians(float degrees);
float ConvertRadiansToDegrees(float Radians);
float CosDegrees(float degrees);
float SinDegrees(float degrees);
float Atan2Degrees(float y, float x);

float GetShortestAngularDispDegrees(float startDegrees, float endDegrees);
float GetTurnedTowardDegrees(float currentDegrees, float goalDegrees, float maxDeltaDegrees);
float GetAngleDegreesBetweenVectors2D(Vec2 const& a, Vec2 const& b);

//-----------------------------------------------------------------------------------------------------------------------------
// Basic 2D & 3D utilities
float GetDistance2D(Vec2 const& posA, Vec2 const& posB);
float GetDistanceSquared2D(Vec2 const& posA, Vec2 const& posB);
float GetDistance3D(Vec3 const& posA, Vec3 const& posB);
float GetDistanceSquared3D(Vec3 const& posA, Vec3 const& posB);
float GetDistanceXY3D(Vec3 const& posA, Vec3 const& posB);
float GetDistanceXYSquared3D(Vec3 const& posA, Vec3 const& posB);
int	  GetTaxicabDistance2D(IntVec2 const& pointA, IntVec2 const& pointB);

float GetProjectedLength2D(Vec2 const& vectorToProject, Vec2 const& vectorToProjectOnto);// works if Vecs not normalized
Vec2 const GetProjectedOnto2D(Vec2 const& vectorToProject, Vec2 const& vectorToProjectOnto);// works if Vecs not normalized

//Geometric query utilities
bool IsPointInsideDisc2D( Vec2 const& point, Vec2 const& discCenter, float discRadius );
bool IsPointInsideOrientedSector2D(Vec2 const& point, Vec2 const& sectorTip, float sectorForwardDegrees, 
										float sectorApertureDegrees, float sectorRadius);
bool IsPointInsideOrientedSector2D(Vec2 const& point, Vec2 const& sectorTip, Vec2 const& sectorForwardNormal, 
										float sectorApertureDegrees, float sectorRadius);
bool IsPointInsideCapsule2D(Vec2 const& point, Vec2 const& boneStart, Vec2 const& boneEnd, float radius);
bool IsPointInsideCapsule2D(Vec2 const& point, Capsule2 capsule);
bool IsPointInsideAABB2D(Vec2 const& referencePos, AABB2 const& box);
bool IsPointInsideOBB2D(Vec2 const& point, OBB2 const& orientedBox);

// collision functions
bool DoTwoFloatRangeOverlap(float ARangeStart, float ARangeEnd, float BRangeStart, float BRangeEnd);
bool DoFloatInFloatRange(float inputFloat, float rangeStart, float rangeEnd);
bool DoDiscsOverlap(Vec2 const& centerA, float radiusA, Vec2 const& centerB, float radiusB);
bool DoDiscOverlapAABB2(Vec2 const& discCenter, float discRadius, AABB2 const box);
bool DoTwoAABB2Overlap(AABB2 A, AABB2 B);

// get nearest point on shapes
Vec2 const GetNearestPointOnDisc2D(Vec2 const& referencePosition, Vec2 const& disCenter, float discRadius);
// Vec2 const GetNearestPointOnSector2D(Vec2 const& point, Vec2 const& sectorTip, float sectorForwardDegrees, float sectorApertureDegrees, float sectorRadius);
// Vec2 const GetNearestPointOnSector2D(Vec2 const& point, Vec2 const& sectorTip, Vec2 const& sectorForwardNormal, float sectorApertureDegrees, float sectorRadius);
Vec2 const GetNearestPointOnAABB2D(Vec2 const& referencePos, AABB2 const& box);
Vec2 const GetNearestPointOnOBB2D(Vec2 const& referencePos, OBB2 const& orientedBox);
Vec2 const GetNearestPointOnLineSegment2D(Vec2 referencePos, LineSegment2 const ls);
Vec2 const GetNearestPointOnLineSegment2D(Vec2 referencePos, Vec2 const& lineSegStart, Vec2 const& lineSegEnd);
Vec2 const GetNearestPointOnInfiniteLine2D(Vec2 referencePos, LineSegment2 const infiniteLine);
Vec2 const GetNearestPointOnInfiniteLine2D(Vec2 referencePos, Vec2 const& pointOnLine, Vec2 const& anotherPointOnline);
Vec2 const GetNearestPointOnCapsule2D(Vec2 referencePos, Capsule2 const& capsule);
Vec2 const GetNearestPointOnSector2D(Vec2 referencePos, Vec2 const& sectorTip, Vec2 const& sectorForwardNormal, float sectorApertureDegrees, float sectorRadius);
Vec2 const GetNearestPointOnSector2D(Vec2 referencePos, Vec2 const& sectorTip, float sectorForwardDegrees, float sectorApertureDegrees, float sectorRadius);

// following functions need to change the center while return the bool
bool PushDiscOutOfFixedPoint2D(Vec2& mobileDiscCenter, float discRadius, Vec2 const& fixedPoint);
bool PushDiscOutOfFixedDisc2D(Vec2& mobileDiscCenter, float mobileDiscRadius, Vec2 const& fixedDiscCenter, float fixedDiscRadius );
bool PushDiscOutOfEachOther2D(Vec2& aCenter, float aRadius, Vec2& bCenter, float bRadius);
bool PushDiscOutOfFixedAABB2D(Vec2& mobileDiscCenter, float discRadius, AABB2 const& fixedBox);

// Transform Utilities
void TransformPosition2D(Vec2& pos, float uniformScale, float rotationDegrees, Vec2 const& translation);
void TransformPosition2D(Vec2& posToTransform, Vec2 const& iBasis, Vec2 const& jBasis, Vec2 const& translation);// transform a 2D point based on new 2D reference point world coordinate and 2D translation, knowing of the new local i and j vectors in world position

void TransformPositionAroundPosOnXY(Vec3& positionToTransform, float scaleXY, float zRotationDegrees, Vec2 const& translationXY);
void TransformPositionAroundPosOnXY(Vec3& positionToTransform, float scaleXY = 1.f, Vec2 const& rotationCenterPos = Vec2(), float zRotationDegrees = 0.f, Vec2 const& translationXY = Vec2());
void TransformPositionAroundPosOnXY(Vec2& positionToTransform, float scaleXY = 1.f, Vec2 const& rotationCenterPos = Vec2(), float zRotationDegrees = 0.f, Vec2 const& translationXY = Vec2());
void TransformPositionAroundPosOnXY(Vec3& positionToTransform, Vec2 const& iBasis, Vec2 const& jBasis, Vec2 const& translation);// transform a 3D point based on new 2D reference point world coordinate and 2D translation, knowing of the new local i and j vectors in world position

// normalize and denormalize
float NormalizeByte(unsigned char inputChar); // transfer char into float
unsigned char DenormalizeByte(float inputFloat); // transfer char into float

//----------------------------------------------------------------------------------------------------------------------------------------------------
// 3D
// billboard matrix
enum class BillboardType
{
	NONE = -1,
	WORLD_UP_CAMERA_FACING,
	WORLD_UP_CAMERA_OPOSSING,
	FULL_CAMERA_FACING,
	FULL_CAMERA_OPPOSING,
	COUNT
};

BillboardType GetBillboardTypeFromString(std::string type);

struct ZCylinder 
{
	ZCylinder() = default;
	ZCylinder(Vec2 basePos, float radius, FloatRange heightRange)
		: CenterXY(basePos)
		, Radius(radius)
		, MinMaxZ(heightRange)
	{
		// Height = MinMaxZ.GetRangeLength();
	};
	ZCylinder(ZCylinder const& copyfrom)
	{
		CenterXY = copyfrom.CenterXY;
		Radius = copyfrom.Radius;
		MinMaxZ = copyfrom.MinMaxZ;
		// Height = MinMaxZ.GetRangeLength();
	};
	~ZCylinder() {};

	Vec2 CenterXY; //take the 2D circle center of the base surface
	float Radius = 0.f;
	// float Height = 0.f;
	FloatRange MinMaxZ; // base and top height

	void SetUniformScale(float uniformScale);
};

struct Sphere
{
	Sphere() = default;
	Sphere(Vec3 center, float radius)
		: Center(center)
		, Radius(radius)
	{}
	Sphere(Sphere const& copyfrom)
	{
		Center = copyfrom.Center;
		Radius = copyfrom.Radius;
	};
	~Sphere() {}

	Vec3 Center;
	float Radius;
};

Mat44 GetBillboardMatrix(BillboardType billboardType,
	Mat44 const& cameraMatrix,
	Vec3 const& billboardPosition,
	Vec3 const& billboardScale = Vec3(1.f, 1.f, 1.f));

// Get nearest point
Vec3 GetNearestPointOnZCylinder(Vec3 point, ZCylinder cylinder);
Vec3 GetNearestPointOnAABB3(Vec3 point, AABB3 box);
Vec3 GetNearestPointOnOBB3(Vec3 const& point, OBB3 const& box);
Vec3 GetNearestPointOnPlane(Vec3 const& point, Plane3 const& plane);
Vec3 GetNearestPointOnSphere(Vec3 point, Sphere sphere);

// is point inside 3D shape
bool IsPointInsideAABB3(Vec3 point, AABB3 box);
bool IsPointInsideSphere(Vec3 point, Sphere sphere);
bool IsPointInsideZCylinder(Vec3 point, ZCylinder cylinder);
bool IsPointInsideOBB3(Vec3 const& point, OBB3 const& targetbox);

// overlap functions
bool DoAABB3sOverlap(AABB3 const& first, AABB3 const& second);
bool DoSpheresOverlap3D(Vec3 const& centerA, float radiusA, Vec3 const& centerB, float radiusB);
bool DoSpheresOverlap3D(Sphere A, Sphere B);
bool DoZCylindersOverlap3D(Vec2 cylinder1CenterXY, float cylinder1Radius, FloatRange cylinder1MinMaxZ,
	Vec2 cylinder2CenterXY, float cylinder2Radius, FloatRange cylinder2MinMaxZ);
bool DoZCylindersOverlap3D(ZCylinder A, ZCylinder B);
bool DoSphereAndAABBOverlap3D(Vec3 sphereCenter, float sphereRadius, AABB3 box);
bool DoSphereAndAABBOverlap3D(Sphere sphere, AABB3 box);
bool DoZCylinderAndAABBOverlap3D(Vec2 cylinderCenterXY, float cylinderRadius, FloatRange cylinderMinMaxZ, AABB3 box);
bool DoZCylinderAndAABBOverlap3D(ZCylinder cylinder, AABB3 box);
bool DoZCylinderAndSphereOverlap3D(Vec2 cylinderCenterXY, float cylinderRadius, FloatRange cylinderMinMaxZ,
	Vec3 sphereCenter, float sphereRadius);
bool DoZCylinderAndSphereOverlap3D(ZCylinder cylinder, Sphere sphere);

// Plane intersections
bool DoPlaneIntersectSphere(Plane3 plane, Sphere sphere);
bool DoPlaneIntersectAABB3(Plane3 plane, AABB3 box);
bool DoPlaneIntersectOBB3(Plane3 plane, OBB3 box);
// bool DoPlaneIntersectZCylinder(Plane3 plane, ZCylinder box);

Vec3 GetRandomDirectionByDirectionAndSpread(Vec3 const& direction, float spread);
