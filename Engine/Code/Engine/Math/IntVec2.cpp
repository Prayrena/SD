#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/core/StringUtils.hpp"
#include "Engine/core/EngineCommon.hpp"
#include <iostream>
//will looking for functions in std in default if not specify using specific namespace
//using namespace std;


//-----------------------------------------------------------------------------------------------
IntVec2::IntVec2(const IntVec2& copy)
	: x(copy.x), y(copy.y)
{
}

//constructor function's special syntax
IntVec2::IntVec2(int initialX, int initialY)
	: x(initialX), y(initialY)
{
}

float IntVec2::GetLength() const
{
	// the calculation of POW ()
	return sqrtf (static_cast<float>(x * x + y * y));//?????????????????????????????????????????????????????????????????
}

int IntVec2::GetTaxicabLength() const
{
	return (abs(x) + abs(y));
}

int IntVec2::GetManhattanDistBetweenHexCoords(IntVec2 pos) const
{
	int dist = (abs(x - pos.x) + abs(x + y - pos.x - pos.y) + abs(x - pos.y)) / 2;
	return dist;
	// return static_cast<int>(std::ceil(dist));
}

int IntVec2::GetLengthSquared() const
{
	return (x * x + y * y);
}
 
int IntVec2::GetLengthSquaredToThisCoords(IntVec2 coords) const
{
	return (((coords.x - x) * (coords.x - x)) + ((coords.y - y) * (coords.y - y)));
}

//?????????????????????????????????????????????????? possible loss of data
float IntVec2::GetOrientationRadians() const	
{
	float newX = static_cast<float>(x);
	float newY = static_cast<float>(y);
	return atan2f(newY, newX);
}

float IntVec2::GetOrientationDegrees() const
{
	float newX = static_cast<float> (x);
	float newY = static_cast<float> (y);
	float theta_Degrees = atan2(newY, newX) * (180.f / PI);
	return theta_Degrees;
} 


IntVec2 const IntVec2::GetRotated90Degrees() const
{
	return IntVec2(-y, x);
}

IntVec2 const IntVec2::GetRotatedMinus90Degrees() const
{
	return IntVec2(y, -x);
}


void IntVec2::Rotate90Degrees()
{
	*this = GetRotated90Degrees();
}

void IntVec2::RotateMinus90Degrees()
{
	*this = GetRotatedMinus90Degrees();
}

bool IntVec2::SetFromText(char const* text)
{
	Strings asStrings = SplitStringOnDelimiter(text, ',');
	int numStrings = (int)asStrings.size();
	if (numStrings != 2)
	{
		return false;
	}
	else
	{
		x = atoi(asStrings[0].c_str());
		y = atoi(asStrings[1].c_str());
		return true; // for check if the set is successful
	}
}

// it should be Vec2 const& vecToAdd, and it will directly use the vecToAdd instead creating a copy of it
// which will slow down the calculation when it is used thousands of time
const IntVec2 IntVec2::operator + (const IntVec2& vecToAdd) const
{
	return IntVec2(vecToAdd.x + x, vecToAdd.y + y);
}


//-----------------------------------------------------------------------------------------------
const IntVec2 IntVec2::operator-(const IntVec2& vecToSubtract) const
{
	return IntVec2(x - vecToSubtract.x, y - vecToSubtract.y);
}



//will there be any difference when unary minus (-) operator overloaded for prefix as well as postfix usage?
const IntVec2 IntVec2::operator-() const
{
	return IntVec2(-x, -y);
}

//------------------------------------------------------------------------------------------------
const IntVec2 IntVec2::operator*(const IntVec2& vecToMultiply) const
{
	return IntVec2(x * vecToMultiply.x, y * vecToMultiply.y);
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
//bool IntVec2::operator<(IntVec2 const& b) const
//{
//	if (y < b.y)
//	{
//		return true;
//	}
//	else if (y > b.y)
//	{
//		return false;
//	}
//	else
//	{
//		return (x < b.x);
//	} 
//}

//-----------------------------------------------------------------------------------------------
void IntVec2::operator+=(const IntVec2& vecToAdd)
{
	x += vecToAdd.x;
	y += vecToAdd.y;
}


//-----------------------------------------------------------------------------------------------
void IntVec2::operator-=(const IntVec2& vecToSubtract)
{
	x -= vecToSubtract.x;
	y -= vecToSubtract.y;
}

//-----------------------------------------------------------------------------------------------
void IntVec2::operator=(const IntVec2& copyFrom)
{
	x = copyFrom.x;
	y = copyFrom.y;
}

//-----------------------------------------------------------------------------------------------
bool IntVec2::operator==(const IntVec2& compare) const
{
	return (x == compare.x && y == compare.y);
}


//-----------------------------------------------------------------------------------------------
bool IntVec2::operator!=(const IntVec2& compare) const
{
	//if (x != compare.x || y != compare.y) {
	//	return true;
	//}

	//else {
	//	return false;
	//}

	return x != compare.x || y != compare.y ? true : false;
}




