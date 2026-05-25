#pragma once
#include "Engine/Math/Vec3.hpp"
#include "Engine/core/Rgba8.hpp"

struct Vec4
{
public:
	// the default include
	Vec4();
	~Vec4();

	float x = 0.f;
	float y = 0.f;
	float z = 0.f;
	float w = 0.f;
	  
	explicit Vec4(float initialX, float initialY, float initialZ, float initialW);
	explicit Vec4(Rgba8 color);
	explicit Vec4(Vec3 vec3, float a = 1.f);
	bool SetFromText(char const* text);

	// Operators overloading (const)
	bool operator ==(Vec4 const& compare) const;			// vec4 == vec4
	bool operator !=(Vec4 const& compare) const;			// vec4 != vec4
	Vec4 const operator+(Vec4 const& vecToAdd) const;		// vec4 + vec4
	Vec4 const operator-(Vec4 const& vecToSubtract) const;	// vec4 - vec4~
	Vec4 operator*(Vec4 const& b) const; // Vec4 * Vec4
	Vec4 operator*(float a) const; // Vec4 * Vec4
	Vec4 operator/(Vec4 const& b) const; // Vec4 / Vec4

	// Operators (self-mutating / non-const)
	void operator += (Vec4 const& vecToAdd);		//Vec4 += Vec4
	void operator -= (Vec4 const& vecToSubtract);	//Vec4 -= Vec4
	void operator *= (Vec4 const& b);	//Vec4 *= Vec4
	void operator *= (float const& b);	//Vec4 *= float
	void operator = (Vec4 const& copyFrom);			//Vec4 =  Vec4

	// Accessors (const methods)
	// float GetLength() const;
	// float GetLengthXY() const;
	// float GetLengthSquared() const;
	// float GetLengthXYSquared() const;
	// float GetAngleAboutZRadians() const;
	// float GetAngleAboutZDegrees() const;
	// Vec4 const GetRotatedAboutZRadians(float deltaRadians) const;
	// Vec4 const GetRotatedAboutZDegrees(float deltaDegrees) const;
	// Vec4 const GetClamped(float maxLength) const;
	// Vec4 const GetNormalized() const;
};

Vec4 ConvertRGBAToVec4(Rgba8 const& color);