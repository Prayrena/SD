//-----------------------------------------------------------------------------------------------
// GameCommon.hpp
//
#pragma once
#include <DirectXMath.h>
#include "Engine/Math/Vec3.hpp"


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	DO NOT MODIFY ANY CODE BELOW HERE WITHOUT EXPRESS PERMISSION FROM YOUR PROFESSOR
//	(as doing so will be considered cheating and have serious academic consequences)
//
/////////////////////////////////////////////////////////////////////////////////////////////////


//-----------------------------------------------------------------------------------------------
// Global typedefs
//
typedef int (TestSetFunctionType)(); // Function signature type for all test functions

//----------------------------------------------------------------------------------------------------------------------------------------------------
constexpr float WORLD_SIZE_X = 200.f;
constexpr float WORLD_SIZE_Y = 100.f;

constexpr int NUM_SIDE_X = 4;
constexpr int NUM_SIDE_Y = 4;
constexpr int NUM_CUBES = 16;
constexpr float CUBE_SIZE = 0.12f;
constexpr float DIST_PLAYER_TO_CUBES_PLAYING = 0.75f;
constexpr float DIST_PLAYER_TO_CUBES_SCORES = 1.2f;
constexpr float HEIGHT_CUBES_TO_GROUND = 0.5f;

constexpr float SCALE_DEFAULT_BEAT_MAX = 1.5f;
constexpr float DIST_SLIDEOUT = 0.42f;

constexpr float HAND_SPHERE_RADIUS = 0.1f;

constexpr float DURATION_GRAB_TO_START = 1.5f;
constexpr float DURATION_SONG_VOLUME_DECREASE_TO_END = 6.f;

constexpr float TEXT_HEIGHT_DEFAULT = 0.1f;
constexpr float TEXT_HEIGHT_LARGE = 0.2f;
constexpr float TEXT_HEIGHT_SMALL = 0.12f;
constexpr float FONT_ASPECT = 0.6f;

constexpr int NUM_PILLARS = 256;
constexpr int NUM_PILLAR_SIDE_X = 32;
constexpr int NUM_PILLAR_SIDE_Y = 32;
constexpr float PILLAR_FIXED_RADIUS = 2.f;
constexpr float PILLAR_MAX_SCALEZ = 0.6f;
constexpr float PILLAR_MIN_SCALEZ = 0.01f;
constexpr float PILLAR_TIME_OFFSET_AMOUNT = 1.f;


// constexpr float PI = 3.1415926f;

//-----------------------------------------------------------------------------------------------
// Functions provided by Main.cpp, but globally accessible to all test files
//
void RunTestSet( bool isGraded, TestSetFunctionType testSetFunction, const char* testSetName );
void VerifyTestResult( bool isCorrect, const char* testName );


