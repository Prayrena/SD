#pragma once
#include "Engine/Math/Vec2.hpp"

// Smooth start 2
float EaseInQuadratic(float t);
Vec2  EaseInQuadratic(Vec2 S, Vec2 E, float t);
Vec3  EaseInQuadratic(Vec3 S, Vec3 E, float t);
// Smooth start 3
float EaseInCubic(float t);
Vec2  EaseInCubic(Vec2 S, Vec2 E, float t);
Vec3  EaseInCubic(Vec3 S, Vec3 E, float t);
// Smooth start 4
float EaseInQuartic(float t);
Vec2  EaseInQuartic(Vec2 S, Vec2 E, float t);
Vec3  EaseInQuartic(Vec3 S, Vec3 E, float t);
// Smooth start 5
float EaseInQuintic(float t);
Vec2  EaseInQuintic(Vec2 S, Vec2 E, float t);
Vec3  EaseInQuintic(Vec3 S, Vec3 E, float t);
// Smooth start 6
float EaseInHexic(float t);
Vec2  EaseInHexic(Vec2 S, Vec2 E, float t);
Vec3  EaseInHexic(Vec3 S, Vec3 E, float t);

//----------------------------------------------------------------------------------------------------------------------------------------------------
// Smooth stop 2
float EaseOutQuadratic(float t);
Vec2  EaseOutQuadratic(Vec2 S, Vec2 E, float t);
Vec3  EaseOutQuadratic(Vec3 S, Vec3 E, float t);
// Smooth stop 3
float EaseOutCubic(float t);
Vec2  EaseOutCubic(Vec2 S, Vec2 E, float t);
Vec3  EaseOutCubic(Vec3 S, Vec3 E, float t);
// Smooth stop 4
float EaseOutQuartic(float t);
Vec2  EaseOutQuartic(Vec2 S, Vec2 E, float t);
Vec3  EaseOutQuartic(Vec3 S, Vec3 E, float t);
// Smooth stop 5
float EaseOutQuintic(float t);
Vec2  EaseOutQuintic(Vec2 S, Vec2 E, float t);
Vec3  EaseOutQuintic(Vec3 S, Vec3 E, float t);
// Smooth stop 6
float EaseOutHexic(float t);
Vec2  EaseOutHexic(Vec2 S, Vec2 E, float t);
Vec3  EaseOutHexic(Vec3 S, Vec3 E, float t);

//----------------------------------------------------------------------------------------------------------------------------------------------------
// Smooth step 3
float SmoothStep3(float t);
Vec2  SmoothStep3(Vec2 S, Vec2 E, float t);
Vec3  SmoothStep3(Vec3 S, Vec3 E, float t);
// Smooth step 6
float SmoothStep5(float t);
Vec2  SmoothStep5(Vec2 S, Vec2 E, float t);
Vec3  SmoothStep5(Vec3 S, Vec3 E, float t);
// hesitate 3
float Hesitate3(float a, float b, float t);
Vec2  Hesitate3(Vec2 S, Vec2 E, float t);
Vec3  Hesitate3(Vec3 S, Vec3 E, float t);
// hesitate 5
float Hesitate5(float a, float b, float t);
Vec2  Hesitate5(Vec2 S, Vec2 E, float t);
Vec3  Hesitate5(Vec3 S, Vec3 E, float t);

//----------------------------------------------------------------------------------------------------------------------------------------------------
float ElevateBounce(float t);
float CustomEasing(float t);