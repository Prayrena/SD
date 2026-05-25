#pragma once

namespace physx {
	template <typename T>
	class PxVec3T;					// Forward declaration of template
	typedef PxVec3T<float> PxVec3;	// Forward declaration of PxVec3
}

struct Vec2;
struct EulerAngles;
struct XrVector3f;

struct Vec3
{
public:
	// the default include
	Vec3();

	float x = 0.f;
	float y = 0.f;
	float z = 0.f;
	 
	explicit Vec3(float initialX, float initialY, float initialZ);
	explicit Vec3(Vec2 vector2, float height = 0.f); // make z as 0.f
	explicit Vec3(XrVector3f xrVec3);

//#ifdef USING_PHYSX
//	explicit Vec3(physx::PxVec3 pxVec3);
//#endif
	explicit Vec3(physx::PxVec3 pxVec3);

	// Accessors (const methods)
	float GetLength() const;
	float GetLengthXY() const;
	float GetLengthSquared() const;
	float GetLengthXYSquared() const;
	float GetAngleAboutZRadians() const;
	float GetAngleAboutZDegrees() const;
	Vec3 const GetRotatedAboutZRadians(float deltaRadians) const;
	Vec3 const GetRotatedAboutZDegrees(float deltaDegrees) const;
	Vec3 const GetClamped(float maxLength) const;
	Vec3 const GetNormalized() const;

	bool  SetFromText(char const* text);

	float GetYawDegrees() const; // Get yaw degrees
	EulerAngles GetOrientation() const;

	// static methods (e.g. creation functions)
	static Vec3 const MakeFromPolarRadians(float longitudeRadians, float latitudeRadians, float length = 1.f);
	static Vec3 const MakeFromPolarDegrees(float longitudeDegrees, float latitudeDegrees, float length = 1.f);
	static Vec3 const GetDirectionForYawPitch(float YawDegrees, float PitchDegrees);

	XrVector3f MakeXrVector3f() const;

	// Operators overloading (const)
	bool operator ==(Vec3 const& compare) const; // vec3 == vec3
	bool operator !=(Vec3 const& compare) const; // vec3 != vec3
	Vec3 const operator+(Vec3 const& vecToAdd) const;// vec3 + vec3
	Vec3 const operator-(Vec3 const& vecToSubtract) const;// vec3 - vec3
	Vec3 operator-() const;
	Vec3 const operator*(float uniformScale) const;// vec3 * float
	Vec3 const operator*(Vec3 const& scale) const;// vec3 * vec3
	Vec3 const operator/(float inverseScale) const;// vec3 / vec3
	bool operator<(const Vec3& other) const;
	bool operator>(const Vec3& other) const;

	// Operators (self-mutating / non-const)
	void operator += (Vec3 const& vecToAdd);//Vec3 += Vec3
	void operator -= (Vec3 const& vecToSubtract);//Vec3 -= Vec3
	void operator *= (float const& uniformScale);//Vec3 *= Vec3
	void operator /= (float const& uniformDivisor);//Vec3 /= Vec3
	void operator = (Vec3 const& copyFrom); //Vec3 = Vec3


	// Standalone "friend" functions that are conceptually, but not actually, part of Vec3
	friend Vec3 const operator*(float uniformScale, Vec3 const& vecToScale);	// float * vec3

	// constant static vector2
	static Vec3 const ZERO;
	static Vec3 const ONE;
	static Vec3 const INVALID;
};

