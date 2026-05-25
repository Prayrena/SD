#pragma once
#include "Engine/Math/IntVec2.hpp"
#include <math.h>

struct Vec3;

// each time we call Vec2, it creates an instance of Vec2, which stores the X,Y
//-----------------------------------------------------------------------------------------------
struct Vec2
{
public: // NOTE: this is one of the few cases where we break both the "m_" naming rule AND the avoid-public-members rule
	float x = 0.f;
	float y = 0.f;

public:
	// Construction/Destruction
	~Vec2() {}												// destructor (do nothing)
	Vec2() {}					                            // default constructor (do nothing)
	//add & when call yourself class
	Vec2( const Vec2& copyFrom );							// copy constructor (from another vec2)
	explicit Vec2(float initialX, float initialY);		    // explicit constructor (from x, y)
	explicit Vec2(IntVec2 intVec2);		
	explicit Vec2(Vec3 vector3D); // take out the z value

	// static methods (e.g. creation functions)
	static Vec2 const MakeFromPolarRadians(float orientationRadians, float length = 1.f);
	static Vec2 const MakeFromPolarDegrees(float orientationDegrees, float length = 1.f);

	// Accessors (const methods)
	float      GetLength() const;
	float      GetLengthSquared() const;
	float      GetOrientationRadians() const;
	float      GetOrientationDegrees() const;
	Vec2 const GetRotated90Degrees() const;
	Vec2 const GetRotatedMinus90Degrees() const;
	Vec2 const GetRotatedRadians( float deltaRadians ) const;
	Vec2 const GetRotatedDegrees( float deltaDegrees ) const;
	Vec2 const GetClamped(float maxLength)const;
	Vec2 const GetNormalized() const;
	Vec2 const GetReflected(Vec2 const& normal) const;

	// Mutators (non-const methods)
	void  SetOrientationRadians(float newOrientationRadians);
	void  SetOrientationDegrees(float newOrientationDegrees);
	void  SetPolarRadians(float newOrientationRadians, float newLength);
	void  SetPolarDegrees(float newOrientationDegrees, float newLength);
	void  Rotate90Degrees();
	void  RotateMinus90Degrees();
	void  RotateRadians(float deltaRadians);
	void  RotateDegrees(float deltaDegrees);
	void  SetLength(float newLength);
	void  ClampLength(float maxLength);
	void  Normalize();
	float NormalizeAndGetPreviousLength();
	Vec2  Reflect( Vec2 const normal);

	bool  SetFromText(char const* text);

	// Operators (const)
	bool		operator==( const Vec2& compare ) const;		// vec2 == vec2
	bool		operator!=( const Vec2& compare ) const;		// vec2 != vec2
	const Vec2	operator+( const Vec2& vecToAdd ) const;		// vec2 + vec2
	const Vec2	operator-( const Vec2& vecToSubtract ) const;	// vec2 - vec2
	const Vec2	operator-() const;								// -vec2, i.e. "unary negation"
	const Vec2	operator*( float uniformScale ) const;			// vec2 * float
	const Vec2	operator*( const Vec2& vecToMultiply ) const;	// vec2 * vec2
	const Vec2	operator/( float inverseScale ) const;			// vec2 / float

	bool		operator<(Vec2 const& compare) const;
	bool		operator>(Vec2 const& compare) const;

	// Operators (self-mutating / non-const)
	void		operator+=( const Vec2& vecToAdd );				// vec2 += vec2
	void		operator-=( const Vec2& vecToSubtract );		// vec2 -= vec2
	void		operator=(const Vec2& copyFrom);				// vec2 = vec2
	void		operator*=( const float uniformScale );			// vec2 *= float
	void		operator/=( const float uniformDivisor );		// vec2 /= float

	// Standalone "friend" functions that are conceptually, but not actually, part of Vec2::
	friend const Vec2 operator*( float uniformScale, const Vec2& vecToScale );	// float * vec2

	// constant static vector2
	static Vec2 const ZERO;
	static Vec2 const ONE;

};


