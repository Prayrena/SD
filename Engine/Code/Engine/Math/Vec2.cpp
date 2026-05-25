#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/core/StringUtils.hpp"
#include <iostream>

Vec2 const Vec2::ZERO = Vec2(0.f, 0.f);
Vec2 const Vec2::ONE = Vec2(1.f, 1.f);


//will looking for functions in std in default if not specify using specific namespace
//using namespace std;


//-----------------------------------------------------------------------------------------------
Vec2::Vec2( const Vec2& copy )
	: x(copy.x), y(copy.y)
{
}

//constructor function's special syntax
Vec2::Vec2( float initialX, float initialY )
	: x( initialX ), y( initialY )
{
}

Vec2::Vec2(IntVec2 intVec2)
{
	x = static_cast<float>(intVec2.x);
	y = static_cast<float>(intVec2.y);
}

Vec2::Vec2(Vec3 vector3D)
{
	x = vector3D.x;
	y = vector3D.y;
}

Vec2 const Vec2::MakeFromPolarDegrees(float orientationDegrees, float length)
{
	float newX = length * cosf(orientationDegrees * (PI / 180.f));
	float newY = length * sinf(orientationDegrees * (PI / 180.f));
	return Vec2(newX, newY);
}

Vec2 const Vec2::MakeFromPolarRadians(float orientationRadians, float length)
{
	float theta_degrees = orientationRadians * (180 / PI);
	return MakeFromPolarDegrees(theta_degrees, length);
}

float Vec2::GetLength() const
{
	// the calculation of POW ()
	return sqrtf(x * x + y * y);
}

float Vec2::GetLengthSquared() const
{
	return (x * x + y * y);
}

float Vec2::GetOrientationRadians() const
{
	return atan2f(y, x);
}

float Vec2::GetOrientationDegrees() const
{
	float theta_Degrees = atan2f(y, x) * (180.f / PI);
	return theta_Degrees;
}


Vec2 const Vec2::GetRotated90Degrees() const
{
	return Vec2(-y, x);
}

Vec2 const Vec2::GetRotatedMinus90Degrees() const
{
	return Vec2(y, -x);
}

// get new Vec2 when the old Vec2 rotate deltaDegrees
Vec2 const Vec2::GetRotatedRadians(float deltaRadians) const
{
	float RotatedRadian = GetOrientationRadians() + deltaRadians;
	float dist = GetLength();
	return MakeFromPolarRadians(RotatedRadian, dist);
}

// get new Vec2 when the old Vec2 rotate deltaDegrees
Vec2 const Vec2::GetRotatedDegrees(float deltaDegrees) const
{
	float RotateDegrees = GetOrientationDegrees() + deltaDegrees;
	float dist = GetLength();
	return MakeFromPolarDegrees(RotateDegrees, dist);
}

// cut the excessive length if the vector is longer than the maxLength
// but keep the direction, otherwise, return a same copy
// testing
Vec2 const Vec2::GetClamped(float maxLength) const
{
	// float thetaRadian = atan2(y, x);
	// return MakeFromPolarRadians(thetaRadian, maxLength);		 
	float oldLength = GetLength();
	if (oldLength > maxLength)
	{
		float scale = maxLength / oldLength;
		float newX = x * scale;
		float newY = y * scale;
		return Vec2(newX, newY);  
	}
	return Vec2(x, y); // usually leave a default return
}

// get a vector that have same orientation but guarantee that the length is one
Vec2 const Vec2::GetNormalized() const
{
	float theta_Radian = atan2(y, x);
	return MakeFromPolarRadians(theta_Radian);
}

Vec2 const Vec2::GetReflected(Vec2 const& normal) const
{
	float normalMultipler = DotProduct2D(*this, normal);
	Vec2 normalVector = normalMultipler * normal;
	Vec2 tangentVector = *this - normalVector;
	normalVector *= (-1);
	return (normalVector + tangentVector);
}

void Vec2::SetOrientationRadians(float newOrientationRadians)
{
	float dist = GetLength();
	x = MakeFromPolarRadians(newOrientationRadians, dist).x;
	y = MakeFromPolarRadians(newOrientationRadians, dist).y;
}

void Vec2::SetOrientationDegrees(float newOrientationDegrees)
{
	float dist = GetLength();
	x = MakeFromPolarDegrees(newOrientationDegrees, dist).x;
	y = MakeFromPolarDegrees(newOrientationDegrees, dist).y;
}

void Vec2::SetPolarRadians(float newOrientationRadians, float newLength)
{
	*this = MakeFromPolarRadians(newOrientationRadians, newLength);
}

void Vec2::SetPolarDegrees(float newOrientationDegrees, float newLength)
{
	*this = MakeFromPolarDegrees(newOrientationDegrees, newLength);
}

void Vec2::Rotate90Degrees()
{
	*this = GetRotated90Degrees();
}

void Vec2::RotateMinus90Degrees()
{
	*this = GetRotatedMinus90Degrees();
}

void Vec2::RotateRadians(float deltaRadians)
{
	float dist = GetLength();
	float theta_Radians = GetOrientationRadians() + deltaRadians;
	*this = MakeFromPolarRadians(theta_Radians, dist);
}

void Vec2::RotateDegrees(float deltaDegrees)
{
	float dist = GetLength();
	float theta_Degrees = GetOrientationDegrees() + deltaDegrees;
	*this = MakeFromPolarDegrees(theta_Degrees, dist);
}

void Vec2::SetLength(float newLength)
{
	*this = GetNormalized() * newLength;
}

// set up a length limit (like a speed limit)
void Vec2::ClampLength(float maxLength)
{
	*this = GetClamped(maxLength);
}

void Vec2::Normalize()
{
	*this = GetNormalized();
}

float Vec2::NormalizeAndGetPreviousLength()
{
	float length = GetLength();
	Normalize();
	return length;
}


Vec2 Vec2::Reflect(Vec2 const normal)
{
	//Vec2 normal = vectorOfWall.GetNormalized();

	float normalMultipler = DotProduct2D(*this, normal);
	Vec2 normalVector = normalMultipler * normal;
	Vec2 tangentVector = *this - normalVector;
	normalVector *= (-1);// change direction after reflection
	*this = (normalVector + tangentVector);
	return *this;
}

bool Vec2::SetFromText(char const* text)
{
	Strings asStrings = SplitStringOnDelimiter(text, ',');
	int numStrings = (int)asStrings.size();
	if (numStrings != 2)
	{
		return false;
	}
	else
	{
		x = (float)atof(asStrings[0].c_str());//convert to number
		y = (float)atof(asStrings[1].c_str());
		return true; // for check if the set is successful
	}
}

// it should be Vec2 const& vecToAdd, and it will directly use the vecToAdd instead creating a copy of it
// which will slow down the calculation when it is used thousands of time
const Vec2 Vec2::operator + ( const Vec2& vecToAdd ) const
{
	return Vec2(vecToAdd.x + x, vecToAdd.y + y );
}


//-----------------------------------------------------------------------------------------------
const Vec2 Vec2::operator-( const Vec2& vecToSubtract ) const
{
	return Vec2(x - vecToSubtract.x, y - vecToSubtract.y);
}



//will there be any difference when unary minus (-) operator overloaded for prefix as well as postfix usage?
const Vec2 Vec2::operator-() const
{
	return Vec2( -x, -y);
}

//Vec2 testing = Vec2(1.f, 2.f);
//Vec2 testing2 = -testing;
//Vec2 testing3 = testing - Vec2(1.f, 2.f);
//

//-----------------------------------------------------------------------------------------------
const Vec2 Vec2::operator*( float uniformScale ) const
{
	return Vec2(x * uniformScale, y * uniformScale);
}

//float testing_f = 1.f;
//Vec2 testing4 = testing_f * testing;
//void testing_function() {
//	std::cout << testing4;
//}
//Vec2 testing4 = testing * testing_f;
//cout << testing4;


//------------------------------------------------------------------------------------------------
const Vec2 Vec2::operator*( const Vec2& vecToMultiply ) const
{
	return Vec2( x * vecToMultiply.x, y * vecToMultiply.y);
}


//-----------------------------------------------------------------------------------------------
const Vec2 Vec2::operator/( float inverseScale ) const
{
	return Vec2( x / inverseScale, y / inverseScale );
}

bool Vec2::operator<(Vec2 const& compare) const
{
	return (x < compare.x && y < compare.y);
}

bool Vec2::operator>(Vec2 const& compare) const
{
	return (x > compare.x && y > compare.y);
}

//-----------------------------------------------------------------------------------------------
void Vec2::operator+=( const Vec2& vecToAdd )
{
	x += vecToAdd.x;
	y += vecToAdd.y;
}


//-----------------------------------------------------------------------------------------------
void Vec2::operator-=( const Vec2& vecToSubtract )
{
	x -= vecToSubtract.x;
	y -= vecToSubtract.y;
}


//-----------------------------------------------------------------------------------------------
void Vec2::operator*=( const float uniformScale )
{
	x *= uniformScale;
	y *= uniformScale;
}


//-----------------------------------------------------------------------------------------------
void Vec2::operator/=( const float uniformDivisor )
{
	x /= uniformDivisor;
	y /= uniformDivisor;
}


//???-----------------------------------------------------------------------------------------------
void Vec2::operator=( const Vec2& copyFrom )
{
	x = copyFrom.x;
	y = copyFrom.y;
}


//???-----------------------------------------------------------------------------------------------
const Vec2 operator*( float uniformScale, const Vec2& vecToScale )
{
	return Vec2( uniformScale*vecToScale.x, uniformScale * vecToScale.y);
}


//-----------------------------------------------------------------------------------------------
bool Vec2::operator==( const Vec2& compare ) const
{
	return (x == compare.x && y == compare.y);
}


//-----------------------------------------------------------------------------------------------
bool Vec2::operator!=(const Vec2& compare) const
{
	//if (x != compare.x || y != compare.y) {
	//	return true;
	//}

	//else {
	//	return false;
	//}

	return x != compare.x || y != compare.y ? true : false;
}




