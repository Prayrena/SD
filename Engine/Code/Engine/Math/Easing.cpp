#include "Engine/Math/Easing.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/Splines.hpp"

float EaseInQuadratic(float t)
{
	float result = t * t;
	return result;
}

Vec2 EaseInQuadratic(Vec2 S, Vec2 E, float t)
{
	float newT= EaseInQuadratic(t);
	Vec2 disp = E - S;
	Vec2 result = S + disp * newT;
	return result;
}

Vec3 EaseInQuadratic(Vec3 S, Vec3 E, float t)
{
	float newT = EaseInQuadratic(t);
	Vec3 disp = E - S;
	Vec3 result = S + disp * newT;
	return result;
}

float EaseInCubic(float t)
{
	float result = t * t * t;
	return result;
}

Vec2 EaseInCubic(Vec2 S, Vec2 E, float t)
{
	float newT = EaseInCubic(t);
	Vec2 disp = E - S;
	Vec2 result = S + disp * newT;
	return result;
}

Vec3 EaseInCubic(Vec3 S, Vec3 E, float t)
{
	float newT = EaseInCubic(t);
	Vec3 disp = E - S;
	Vec3 result = S + disp * newT;
	return result;
}

float EaseInQuartic(float t)
{
	float result = t * t * t * t;
	return result;
}

Vec2 EaseInQuartic(Vec2 S, Vec2 E, float t)
{
	float newT = EaseInQuartic(t);
	Vec2 disp = E - S;
	Vec2 result = S + disp * newT;
	return result;
}

Vec3 EaseInQuartic(Vec3 S, Vec3 E, float t)
{
	float newT = EaseInQuartic(t);
	Vec3 disp = E - S;
	Vec3 result = S + disp * newT;
	return result;
}

float EaseInQuintic(float t)
{
	float result = t * t * t * t * t;
	return result;
}

Vec2 EaseInQuintic(Vec2 S, Vec2 E, float t)
{
	float newT = EaseInQuintic(t);
	Vec2 disp = E - S;
	Vec2 result = S + disp * newT;
	return result;
}

Vec3 EaseInQuintic(Vec3 S, Vec3 E, float t)
{
	float newT = EaseInQuintic(t);
	Vec3 disp = E - S;
	Vec3 result = S + disp * newT;
	return result;
}

float EaseInHexic(float t)
{
	float result = t * t * t * t * t * t;
	return result;
}

Vec2 EaseInHexic(Vec2 S, Vec2 E, float t)
{
	float newT = EaseInHexic(t);
	Vec2 disp = E - S;
	Vec2 result = S + disp * newT;
	return result;
}

Vec3 EaseInHexic(Vec3 S, Vec3 E, float t)
{
	float newT = EaseInHexic(t);
	Vec3 disp = E - S;
	Vec3 result = S + disp * newT;
	return result;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
float EaseOutQuadratic(float t)
{
	float s = 1.f - t;
	float result = 1 - s * s;
	return result;
}

Vec2 EaseOutQuadratic(Vec2 S, Vec2 E, float t)
{
	float newT = EaseOutQuadratic(t);
	Vec2 disp = E - S;
	Vec2 result = S + disp * newT;
	return result;
}

Vec3 EaseOutQuadratic(Vec3 S, Vec3 E, float t)
{
	float newT = EaseOutQuadratic(t);
	Vec3 disp = E - S;
	Vec3 result = S + disp * newT;
	return result;
}

float EaseOutCubic(float t)
{
	float s = 1.f - t;
	float result = 1 - s * s * s;
	return result;
}

Vec2 EaseOutCubic(Vec2 S, Vec2 E, float t)
{
	float newT = EaseOutCubic(t);
	Vec2 disp = E - S;
	Vec2 result = S + disp * newT;
	return result;
}

Vec3 EaseOutCubic(Vec3 S, Vec3 E, float t)
{
	float newT = EaseOutCubic(t);
	Vec3 disp = E - S;
	Vec3 result = S + disp * newT;
	return result;
}

float EaseOutQuartic(float t)
{
	float s = 1.f - t;
	float result = 1 - s * s * s * s;
	return result;
}

Vec2 EaseOutQuartic(Vec2 S, Vec2 E, float t)
{
	float newT = EaseOutQuartic(t);
	Vec2 disp = E - S;
	Vec2 result = S + disp * newT;
	return result;
}

Vec3 EaseOutQuartic(Vec3 S, Vec3 E, float t)
{
	float newT = EaseOutQuartic(t);
	Vec3 disp = E - S;
	Vec3 result = S + disp * newT;
	return result;
}

float EaseOutQuintic(float t)
{
	float s = 1.f - t;
	float result = 1 - s * s * s * s * s;
	return result;
}

Vec2 EaseOutQuintic(Vec2 S, Vec2 E, float t)
{
	float newT = EaseOutQuintic(t);
	Vec2 disp = E - S;
	Vec2 result = S + disp * newT;
	return result;
}

Vec3 EaseOutQuintic(Vec3 S, Vec3 E, float t)
{
	float newT = EaseOutQuintic(t);
	Vec3 disp = E - S;
	Vec3 result = S + disp * newT;
	return result;
}

float EaseOutHexic(float t)
{
	float s = 1.f - t;
	float result = 1 - s * s * s * s * s * s;
	return result;
}

Vec2 EaseOutHexic(Vec2 S, Vec2 E, float t)
{
	float newT = EaseOutHexic(t);
	Vec2 disp = E - S;
	Vec2 result = S + disp * newT;
	return result;
}

Vec3 EaseOutHexic(Vec3 S, Vec3 E, float t)
{
	float newT = EaseOutHexic(t);
	Vec3 disp = E - S;
	Vec3 result = S + disp * newT;
	return result;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
float SmoothStep3(float t)
{
	float A = EaseInCubic(t);
	float B = EaseOutCubic(t);
	float mix = A * (1.f - t) + B * t;
	return mix;
}

Vec2 SmoothStep3(Vec2 S, Vec2 E, float t)
{
	float newT = SmoothStep3(t);
	Vec2 disp = E - S;
	Vec2 mix = S + disp * newT;
	return mix;
}

Vec3 SmoothStep3(Vec3 S, Vec3 E, float t)
{
	float newT = SmoothStep3(t);
	Vec3 disp = E - S;
	Vec3 mix = S + disp * newT;
	return mix;
}

float SmoothStep5(float t)
{
	float A = EaseInQuintic(t);
	float B = EaseOutQuintic(t);
	float mix = A * (1.f - t) + B * t;
	return mix;
}

Vec2 SmoothStep5(Vec2 S, Vec2 E, float t)
{
	float newT = SmoothStep5(t);
	Vec2 disp = E - S;
	Vec2 mix = S + disp * newT;
	return mix;
}

Vec3 SmoothStep5(Vec3 S, Vec3 E, float t)
{
	float newT = SmoothStep3(t);
	Vec3 disp = E - S;
	Vec3 mix = S + disp * newT;
	return mix;
}

float Hesitate3(float a, float b, float t)
{
	return ComputeCubicBezier1D(a, b, a, b, t);
}

Vec2 Hesitate3(Vec2 S, Vec2 E, float t)
{
	Vec2 mix;
	mix.x = Hesitate3(S.x, E.x, t);
	mix.y = Hesitate3(S.y, E.y, t);
	return mix;
}

Vec3 Hesitate3(Vec3 S, Vec3 E, float t)
{
	Vec3 mix;
	mix.x = Hesitate3(S.x, E.x, t);
	mix.y = Hesitate3(S.y, E.y, t);
	mix.z = Hesitate3(S.z, E.z, t);

	return mix;
}

float Hesitate5(float a, float b, float t)
{
	return ComputeQuinticBezier1D(a, b, a, b, a, b, t);
}

Vec2 Hesitate5(Vec2 S, Vec2 E, float t)
{
	Vec2 mix;
	mix.x = Hesitate5(S.x, E.x, t);
	mix.y = Hesitate5(S.y, E.y, t);
	return mix;

}

Vec3 Hesitate5(Vec3 S, Vec3 E, float t)
{
	Vec3 mix;
	mix.x = Hesitate5(S.x, E.x, t);
	mix.y = Hesitate5(S.y, E.y, t);
	mix.z = Hesitate5(S.z, E.z, t);

	return mix;
}
	
float ElevateBounce(float t)
{
	float A = abs(cosf(EaseOutCubic(t) * 4.5f) * (1.f - t));
	return (1 - A);
}

float CustomEasing(float t)
{
	float A = EaseOutQuintic(t);
	float B = Hesitate5(0.f, 1.f, t);
	float mix = A * (1.f - t) + B * t;
	return mix;
}

// how to get bounce?