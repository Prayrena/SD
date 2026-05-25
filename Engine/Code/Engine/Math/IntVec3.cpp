#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/IntVec3.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/core/StringUtils.hpp"
#include "Engine/core/EngineCommon.hpp"
#include <iostream>
//will looking for functions in std in default if not specify using specific namespace
//using namespace std;


//-----------------------------------------------------------------------------------------------
IntVec3::IntVec3(const IntVec3& copy)
	: x(copy.x)
	, y(copy.y)
	, z(copy.z)
{
}

//constructor function's special syntax
IntVec3::IntVec3(int initialX, int initialY, int initialZ)
	: x(initialX)
	, y(initialY)
	, z(initialZ)
{
}

float IntVec3::GetLength() const
{
	return sqrtf (static_cast<float>(x * x + y * y + z * z));
}

int IntVec3::GetTaxicabLength() const
{
	return (abs(x) + abs(y) + abs(z));
}

int IntVec3::GetLengthSquared() const
{
	return (x * x + y * y+ z * z);
}
 
float IntVec3::GetOrientationRadiansOnXY() const	
{
	float newX = static_cast<float>(x);
	float newY = static_cast<float>(y);
	return atan2f(newY, newX);
}

float IntVec3::GetOrientationDegreesOnXY() const
{
	float newX = static_cast<float> (x);
	float newY = static_cast<float> (y);
	float theta_Degrees = atan2(newY, newX) * (180.f / PI);
	return theta_Degrees;
} 


IntVec3 const IntVec3::GetRotated90DegreesOnXY() const
{
	return IntVec3(-y, x, z);
}

IntVec3 const IntVec3::GetRotatedMinus90DegreesOnXY() const
{
	return IntVec3(y, -x, z);
}


void IntVec3::Rotate90DegreesOnXY()
{
	*this = GetRotated90DegreesOnXY();
}

void IntVec3::RotateMinus90DegreesOnXY()
{
	*this = GetRotatedMinus90DegreesOnXY();
}

bool IntVec3::SetFromText(char const* text)
{
	Strings asStrings = SplitStringOnDelimiter(text, ',');
	int numStrings = (int)asStrings.size();
	if (numStrings != 3)
	{
		return false;
	}
	else
	{
		x = atoi(asStrings[0].c_str());
		y = atoi(asStrings[1].c_str());
		z = atoi(asStrings[2].c_str());
		return true; // for check if the set is successful
	}
}

const IntVec3 IntVec3::operator + (const IntVec3& vecToAdd) const
{
	return IntVec3(vecToAdd.x + x, vecToAdd.y + y, vecToAdd.z + z);
}


//-----------------------------------------------------------------------------------------------
const IntVec3 IntVec3::operator-(const IntVec3& vecToSubtract) const
{
	return IntVec3(x - vecToSubtract.x, y - vecToSubtract.y, z - vecToSubtract.z);
}

//will there be any difference when unary minus (-) operator overloaded for prefix as well as postfix usage?
const IntVec3 IntVec3::operator-() const
{
	return IntVec3(-x, -y, -z);
}

//------------------------------------------------------------------------------------------------
const IntVec3 IntVec3::operator*(const IntVec3& vecToMultiply) const
{
	return IntVec3(x * vecToMultiply.x, y * vecToMultiply.y, z * vecToMultiply.z);
}

//-----------------------------------------------------------------------------------------------
void IntVec3::operator+=(const IntVec3& vecToAdd)
{
	x += vecToAdd.x;
	y += vecToAdd.y;
	z += vecToAdd.z;
}


//-----------------------------------------------------------------------------------------------
void IntVec3::operator-=(const IntVec3& vecToSubtract)
{
	x -= vecToSubtract.x;
	y -= vecToSubtract.y;
	z -= vecToSubtract.z;
}

//-----------------------------------------------------------------------------------------------
void IntVec3::operator=(const IntVec3& copyFrom)
{
	x = copyFrom.x;
	y = copyFrom.y;
	z = copyFrom.z;
}

//-----------------------------------------------------------------------------------------------
bool IntVec3::operator==(const IntVec3& compare) const
{
	return (x == compare.x && y == compare.y && z == compare.z);
}


//-----------------------------------------------------------------------------------------------
bool IntVec3::operator!=(const IntVec3& compare) const
{
	//if (x != compare.x || y != compare.y) {
	//	return true;
	//}

	//else {
	//	return false;
	//}

	return x != compare.x || y != compare.y || z != compare.z ? true : false;
}




