#include "Engine/Math/Vec4.hpp"
#include "Engine/core/StringUtils.hpp"
#include "Engine/Math/MathUtils.hpp"

Vec4::Vec4()
{

}


Vec4::Vec4(Vec3 vec3, float a)
{
	x = vec3.x;
	y = vec3.y;
	z = vec3.z;
	w = a;
}

Vec4::Vec4(Rgba8 color)
{
	x = NormalizeByte(color.r);
	y = NormalizeByte(color.g);
	z = NormalizeByte(color.b);
	w = NormalizeByte(color.a);
}

Vec4::~Vec4()
{

}

Vec4::Vec4(float initialX, float initialY, float initialZ, float initialW)
	: x(initialX), y(initialY), z(initialZ), w(initialW)
{

} 
      
//float Vec4::GetLength() const
//{
//	return sqrtf(x * x + y * y + z * z);
//}
//
//float Vec4::GetLengthXY() const
//{
//	return Vec2(x, y).GetLength();
//}
//
//float Vec4::GetLengthSquared() const
//{
//	return (x * x + y * y + z * z);
//}
//
//float Vec4::GetLengthXYSquared() const
//{
//	return Vec2(x, y).GetLengthSquared();
//}
//
//float Vec4::GetAngleAboutZRadians() const
//{
//	return atan2f(y, x);
//}
//
//float Vec4::GetAngleAboutZDegrees() const
//{
//	return GetAngleAboutZRadians() * (180.f / PI);
//}
//
//Vec4 const Vec4::GetRotatedAboutZRadians(float deltaRadians) const
//{
//	float dist = Vec2 (x, y).GetLength();
//	float theta_Radian = GetAngleAboutZRadians();
//	theta_Radian += deltaRadians;
//	Vec2 rotatedVec2 = rotatedVec2.MakeFromPolarRadians(theta_Radian, dist);//call vec2 function in Vec3 class
//	return Vec4(rotatedVec2.x, rotatedVec2.y, z);
//}
//
//Vec4 const Vec4::GetRotatedAboutZDegrees(float deltaDegrees) const
//{
//	float dist = Vec2(x, y).GetLength();
//	float theta_degree = Vec2(x, y).GetOrientationDegrees() + deltaDegrees;
//	float newX = dist * cosf(theta_degree * ( PI / 180.f ));
//	float newY = dist * sinf(theta_degree * ( PI / 180.f ));
//	return Vec4(newX, newY, z);
//}
//
//Vec4 const Vec4::GetClamped(float maxLength) const
//{
//	float dist = GetLength();
//	if (dist > maxLength)
//	{
//		float uniformDivisor = maxLength / dist;
//		return *this / uniformDivisor;
//	}
//	else 
//	{
//		return *this;
//	}
//}
//
//Vec4 const Vec4::GetNormalized() const
//{
//	float dist = GetLength();
//	return *this / dist;
//}

bool Vec4::SetFromText(char const* text)
{
	Strings asStrings = SplitStringOnDelimiter(text, ',');
	int numStrings = (int)asStrings.size();
	if (numStrings != 4)
	{
		return false;
	}
	else
	{
		x = (float)atof(asStrings[0].c_str());
		y = (float)atof(asStrings[1].c_str());
		z = (float)atof(asStrings[2].c_str());
		w = (float)atof(asStrings[2].c_str());
		return true; // for check if the set is successful
	}
}

bool Vec4::operator==(Vec4 const& compare) const
{
	if (x == compare.x && y == compare.y && z == compare.z && w == compare.w)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool Vec4::operator!=(Vec4 const& compare) const
{
	if (x != compare.x || y != compare.y || z != compare.z || w != compare.w)
	{
		return true;
	}
	else
	{
		return false;
	}
}

Vec4 const Vec4::operator+(Vec4 const& vecToAdd) const
{
	return Vec4(x + vecToAdd.x, y + vecToAdd.y, z + vecToAdd.z, w + vecToAdd.w);
}

Vec4 const Vec4::operator-(Vec4 const& vecToSubtract) const
{
	return Vec4(x - vecToSubtract.x, y - vecToSubtract.y, z - vecToSubtract.z, w - vecToSubtract.w);
}

Vec4 Vec4::operator*(Vec4 const& b) const
{
	return Vec4(x * b.x, y * b.y, z * b.z, w * b.w);
}

Vec4 Vec4::operator*(float a) const
{
	return Vec4(x * a, y * a, z * a, w * a);
}

Vec4 Vec4::operator/(Vec4 const& b) const
{
	return Vec4(x / b.x, y / b.y, z / b.z, w / b.w);
}

void Vec4::operator*=(float const& b)
{
	x *= b;
	y *= b;
	z *= b;
	w *= b;
}

void Vec4::operator*=(Vec4 const& b)
{
	x = b.x;
	y = b.y;
	z = b.z;
	w = b.w;
}

void Vec4::operator=(Vec4 const& copyFrom)
{
	x = copyFrom.x;
	y = copyFrom.y;
	z = copyFrom.z;
	w = copyFrom.w;
}

void Vec4::operator-=(Vec4 const& vecToSubtract)
{
	x -= vecToSubtract.x;
	y -= vecToSubtract.y;
	z -= vecToSubtract.z;
	w =  vecToSubtract.w;
}

void Vec4::operator+=(Vec4 const& vecToAdd)
{
	x += vecToAdd.x;
	y += vecToAdd.y;
	z += vecToAdd.z;
	w += vecToAdd.w;
}

Vec4 ConvertRGBAToVec4(Rgba8 const& color)
{
	Vec4 colorAsFloats;
	colorAsFloats.x = RangeMap(color.r, 0.f, 255.f, 0.f, 1.f);
	colorAsFloats.y = RangeMap(color.g, 0.f, 255.f, 0.f, 1.f);
	colorAsFloats.z = RangeMap(color.b, 0.f, 255.f, 0.f, 1.f);
	colorAsFloats.w = RangeMap(color.a, 0.f, 255.f, 0.f, 1.f);
	return colorAsFloats;
}
