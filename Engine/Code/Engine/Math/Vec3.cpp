#include "ThirdParty/PhysX/include/foundation/PxVec3.h"
// #include "ThirdParty/PhysX/include/PxPhysicsAPI.h"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/core/StringUtils.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "ThirdParty/OpenXR/include/openxr/openxr.h"

using namespace physx;
using namespace std;

Vec3 const Vec3::ZERO = Vec3(0.f, 0.f, 0.f);
Vec3 const Vec3::ONE = Vec3(1.f, 1.f, 1.f);
Vec3 const Vec3::INVALID = Vec3(numeric_limits<float>::infinity(), 
								numeric_limits<float>::infinity(), 
								numeric_limits<float>::infinity());

Vec3 :: Vec3(float initialX, float initialY, float initialZ) 
	: x(initialX), y(initialY), z(initialZ)
{

}

Vec3::Vec3(Vec2 vector2, float height /* = 0.f*/)
	: x(vector2.x)
	, y(vector2.y)
	, z(height)
{
}

Vec3::Vec3()
{
	x = 0.f;
	y = 0.f;
	z = 0.f;
}

Vec3::Vec3(XrVector3f xrVec3)
{
	x = xrVec3.x;
	y = xrVec3.y;
	z = xrVec3.z;
}

//#ifdef USING_PHYSX
//#endif
Vec3::Vec3(physx::PxVec3 pxVec3)
	: x(pxVec3.x)
	, y(pxVec3.y)
	, z(pxVec3.z)
{

}

bool Vec3::operator==(Vec3 const& compare) const
{
	if (x == compare.x && y == compare.y && z == compare.z)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool Vec3::operator!=(Vec3 const& compare) const
{
	if (x != compare.x || y != compare.y || z != compare.z)
	{
		return true;
	}
	else
	{
		return false;
	}
}

float Vec3::GetLength() const
{
	return sqrtf(x * x + y * y + z * z);
}

float Vec3::GetLengthXY() const
{
	return Vec2(x, y).GetLength();
}

float Vec3::GetLengthSquared() const
{
	return (x * x + y * y + z * z);
}

float Vec3::GetLengthXYSquared() const
{
	return Vec2(x, y).GetLengthSquared();
}

float Vec3::GetAngleAboutZRadians() const
{
	return atan2f(y, x);
}

float Vec3::GetAngleAboutZDegrees() const
{
	return GetAngleAboutZRadians() * (180.f / PI);
}

Vec3 const Vec3::GetRotatedAboutZRadians(float deltaRadians) const
{
	float dist = Vec2 (x, y).GetLength();
	float theta_Radian = GetAngleAboutZRadians();
	theta_Radian += deltaRadians;
	Vec2 rotatedVec2 = rotatedVec2.MakeFromPolarRadians(theta_Radian, dist);//call vec2 function in Vec3 class
	return Vec3(rotatedVec2.x, rotatedVec2.y, z);
}

Vec3 const Vec3::GetRotatedAboutZDegrees(float deltaDegrees) const
{
	float dist = Vec2(x, y).GetLength();
	float theta_degree = Vec2(x, y).GetOrientationDegrees() + deltaDegrees;
	float newX = dist * cosf(theta_degree * ( PI / 180.f ));
	float newY = dist * sinf(theta_degree * ( PI / 180.f ));
	return Vec3(newX, newY, z);
}

Vec3 const Vec3::GetClamped(float maxLength) const
{
	float dist = GetLength();
	if (dist > maxLength)
	{
		float uniformDivisor = maxLength / dist;
		return *this / uniformDivisor;
	}
	else 
	{
		return *this;
	}
}

Vec3 const Vec3::GetNormalized() const
{
	float dist = GetLength();
	if (dist <= 0.f)
	{
		return *this;
	}
	return *this / dist;
}

bool Vec3::SetFromText(char const* text)
{
	Strings asStrings = SplitStringOnDelimiter(text, ',');
	int numStrings = (int)asStrings.size();
	if (numStrings != 3)
	{
		return false;
	}
	else
	{
		x = (float)atof(asStrings[0].c_str());
		y = (float)atof(asStrings[1].c_str());
		z = (float)atof(asStrings[2].c_str());
		return true; // for check if the set is successful
	}
}

float Vec3::GetYawDegrees() const
{
	float theta_Degrees = atan2f(y, x) * (180.f / PI);
	return theta_Degrees;
}

EulerAngles Vec3::GetOrientation() const
{
	float yaw_Degrees = atan2f(y, x) * (180.f / PI);
	float XYLength = sqrtf(y * y + x * x);
	float pitch_Degrees = atan2f(z * (-1.f), XYLength) * (180.f / PI);
	EulerAngles orientation(yaw_Degrees, pitch_Degrees, 0.f);
	return orientation;
}

Vec3 const Vec3::MakeFromPolarRadians(float longitudeRadians, float latitudeRadians, float length /*= 1.f*/)
{
	float latitudeDegrees	= latitudeRadians * (180 / PI);
	float longitudeDegrees	= longitudeRadians * (180 / PI);
	return MakeFromPolarDegrees(latitudeDegrees, longitudeDegrees, length);
}


Vec3 const Vec3::MakeFromPolarDegrees(float longitudeDegrees, float latitudeDegrees, float length /*= 1.f*/)
{
	Vec3 direction = GetDirectionForYawPitch(longitudeDegrees, latitudeDegrees);
	return (direction * length);
}

Vec3 const Vec3::GetDirectionForYawPitch(float YawDegrees, float PitchDegrees)
{
	float yawRadians = ConvertDegreesToRadians(YawDegrees);
	float pitchRadians = ConvertDegreesToRadians(PitchDegrees);

	return Vec3(cosf(pitchRadians) * cosf(yawRadians), cosf(pitchRadians) * sinf(yawRadians), -sinf(pitchRadians));

	// this is calculated by
	// Mat44 yawMat = Mat44::CreateZRotationDegrees(YawDegrees);
	// Mat44 pitchMat = Mat44::CreateYRotationDegrees(PitchDegrees);
	// yawMat.Append(pitchMat);
	// Vec3 direction = Vec3(yawMat.m_values[Mat44::Ix], yawMat.m_values[Mat44::Iy], yawMat.m_values[Mat44::Iz]);
	// return direction;
}

XrVector3f Vec3::MakeXrVector3f() const
{
	XrVector3f XrVec3;
	XrVec3.x = x;
	XrVec3.y = y;
	XrVec3.z = z;
	return XrVec3;
}


Vec3 const Vec3::operator+(Vec3 const& vecToAdd) const
{
	return Vec3(x + vecToAdd.x, y + vecToAdd.y, z + vecToAdd.z);
}

Vec3 const Vec3::operator-(Vec3 const& vecToSubtract) const
{
	return Vec3(x - vecToSubtract.x, y - vecToSubtract.y, z - vecToSubtract.z);
}

Vec3 Vec3::operator-() const
{
	return Vec3(-x, -y, -z);
}

void Vec3::operator/=(float const& uniformDivisor)
{
	x = x / uniformDivisor;
	y = y / uniformDivisor;
	z = z / uniformDivisor;
}

bool Vec3::operator<(const Vec3& other) const
{
	// return ((GetLength() < other.GetLength()));
	return (x < other.x && y < other.y && z < other.z);
}

bool Vec3::operator>(const Vec3& other) const
{
	// return GetLength() > other.GetLength();
	return (x > other.x && y > other.y && z > other.z);
}

void Vec3::operator=(Vec3 const& copyFrom)
{
	x = copyFrom.x;
	y = copyFrom.y;
	z = copyFrom.z;
}

Vec3 const Vec3::operator*(float uniformScale) const
{
	return Vec3(x * uniformScale, y * uniformScale, z * uniformScale);
}

Vec3 const Vec3::operator*(Vec3 const& scale) const
{
	return Vec3(x * scale.x, y * scale.y, z * scale.z);
}

Vec3 const Vec3::operator/(float inverseScale) const
{
	return Vec3(x / inverseScale, y / inverseScale, z / inverseScale);
}

void Vec3::operator*=(float const& uniformScale)
{
	x = uniformScale * x;
	y = uniformScale * y;
	z = uniformScale * z;
}

void Vec3::operator-=(Vec3 const& vecToSubtract)
{
	x -= vecToSubtract.x;
	y -= vecToSubtract.y;
	z -= vecToSubtract.z;

}

void Vec3::operator+=(Vec3 const& vecToAdd)
{
	x += vecToAdd.x;
	y += vecToAdd.y;
	z += vecToAdd.z;
}

const Vec3 operator*(float uniformScale, Vec3 const& vecToScale)
{
	return Vec3(uniformScale * vecToScale.x, uniformScale * vecToScale.y, uniformScale * vecToScale.z);
}