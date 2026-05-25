//-----------------------------------------------------------------------------------------------
// UnitTests_MP1A4.cpp
//
#include "Game/UnitTests_MP1A4.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//		DO NOT MODIFY ANY CODE BELOW HERE WITHOUT EXPRESS PERMISSION FROM YOUR PROFESSOR
//		(as doing so will be considered cheating and have serious academic consequences)
//
//
/////////////////////////////////////////////////////////////////////////////////////////////////


//-----------------------------------------------------------------------------------------------
// Forward declarations for selective helper functions needed here
//
bool IsMostlyEqual( float a, float b, float epsilon=0.001f );
bool IsMostlyEqual( Vector2Class const& vec2, float x, float y );
bool IsMostlyEqual( Vector2Class const& vec2a, Vector2Class const& vec2b );
bool IsMostlyEqual( int a, int b, int epsilon=0 );
bool IsMostlyEqual( Vector3Class const& vec3, float x, float y, float z );
bool IsMostlyEqual( Vector3Class const& vec3a, Vector3Class const& vec3b );
bool IsMostlyEqual( AABB2Class const& box1, AABB2Class const& box2 );
bool IsMostlyEqual( AABB2Class const& box, Vector2Class const& mins, Vector2Class const& maxs );
bool IsMostlyEqual( AABB2Class const& box, float minX, float minY, float maxX, float maxY );
bool IsEqual( IntVec2Class const& a, IntVec2Class const& b );
bool IsEqual( IntVec2Class const& ivec2, int x, int y );


//------------------------------------------------------------------------------------------------
Vector2Class const MakeDirection2D( float orientationDegrees )
{
	float orientationRadians = (3.1415926535897932384626433832795f / 180.f) * orientationDegrees;
	return Vector2Class( cosf( orientationRadians ), sinf( orientationRadians ) );
}


//////////////////////////////////////////////////////////////////////////////////////////////////
// Tests follow
//////////////////////////////////////////////////////////////////////////////////////////////////


//-----------------------------------------------------------------------------------------------
int TestSet_MP1A4_PushOutOf()
{
#if defined( ENABLE_TestSet_MP1A4_PushOutOf )

	Vector2Class centerA( 500.f, 100.f );
	Vector2Class const centerB( 580.f, 160.f ); // +(80,60) over from A (dist is 100)
	float smallRadiusA = 50.f;
	float smallRadiusB = 40.f; // sum of small radii is 90 (discs do not overlap)
	float largeRadiusA = 80.f;
	float largeRadiusB = 70.f; // sum of large radii is 150 (discs overlap)

	Vector2Class centerA1( centerA ); // temp copy we can push around
	Vector2Class centerA2( centerA ); // temp copy we can push around
	Vector2Class centerA3( centerA ); // temp copy we can push around
	Vector2Class centerA4( centerA ); // temp copy we can push around
	Vector2Class centerA5( centerA ); // temp copy we can push around
	Vector2Class centerA6( centerA ); // temp copy we can push around
	Vector2Class centerA7( centerA ); // temp copy we can push around
	Vector2Class centerA8( centerA ); // temp copy we can push around
	Vector2Class centerA9( centerA ); // temp copy we can push around
	Vector2Class centerA10( centerA ); // temp copy we can push around
	Vector2Class centerA11( centerA ); // temp copy we can push around
	Vector2Class centerA12( centerA ); // temp copy we can push around
	Vector2Class centerA13( centerA ); // temp copy we can push around
	Vector2Class centerA14( centerA ); // temp copy we can push around
	Vector2Class centerA15( centerA ); // temp copy we can push around
	Vector2Class centerA16( centerA ); // temp copy we can push around
	Vector2Class centerA17( centerA ); // temp copy we can push around
	Vector2Class centerA18( centerA ); // temp copy we can push around
	Vector2Class centerA19( centerA ); // temp copy we can push around

	Vector2Class centerB1( centerB ); // temp copy we can push around
	Vector2Class centerB2( centerB ); // temp copy we can push around
	Vector2Class centerB3( centerB ); // temp copy we can push around
	Vector2Class centerB4( centerB ); // temp copy we can push around
	Vector2Class centerB5( centerB ); // temp copy we can push around
	Vector2Class centerB6( centerB ); // temp copy we can push around
	Vector2Class centerB7( centerB ); // temp copy we can push around
	Vector2Class centerB8( centerB ); // temp copy we can push around
	Vector2Class centerB9( centerB ); // temp copy we can push around

	Vector2Class nearest1 = Function_GetNearestPointOnDisc2D( Vector2Class( 560.f, 100.f ), centerA, largeRadiusA ); // +60,0 (dist 60) from centerA
	Vector2Class nearest2 = Function_GetNearestPointOnDisc2D( Vector2Class( 560.f, 100.f ), centerA, smallRadiusA ); // +60,0 (dist 60) from centerA
	Vector2Class nearest3 = Function_GetNearestPointOnDisc2D( Vector2Class( 610.f, 200.f ), centerB, largeRadiusB ); // If this doesn't compile, make sure you're passing center as "Vec2 const&" // +30,40 (dist 50) from centerB
	Vector2Class nearest4 = Function_GetNearestPointOnDisc2D( Vector2Class( 610.f, 200.f ), centerB, smallRadiusB ); // +30,40 (dist 50) from centerB
	Vector2Class nearest5 = Function_GetNearestPointOnDisc2D( Vector2Class( 470.f,  60.f ), centerA, largeRadiusA ); // -30,-40 (dist 50) from centerA
	Vector2Class nearest6 = Function_GetNearestPointOnDisc2D( Vector2Class( 420.f,  40.f ), centerA, smallRadiusA ); // -80,-60 (dist 100) from centerA
	VerifyTestResult( IsMostlyEqual( nearest1, 560.f, 100.f ), "GetNearestPointOnDisc2D() incorrect for point inside (due east)" );
	VerifyTestResult( IsMostlyEqual( nearest2, 550.f, 100.f ), "GetNearestPointOnDisc2D() incorrect for point outside (due east)" );
	VerifyTestResult( IsMostlyEqual( nearest3, 610.f, 200.f ), "GetNearestPointOnDisc2D() incorrect for point inside (northeast)" );
	VerifyTestResult( IsMostlyEqual( nearest4, 604.f, 192.f ), "GetNearestPointOnDisc2D() incorrect for point outside (northeast)" );
	VerifyTestResult( IsMostlyEqual( nearest5, 470.f,  60.f ), "GetNearestPointOnDisc2D() incorrect for point inside (southwest)" );
	VerifyTestResult( IsMostlyEqual( nearest6, 460.f,  70.f ), "GetNearestPointOnDisc2D() incorrect for point outside (southwest)" );

	bool wasB1Pushed = Function_PushDiscOutOfPoint2D( centerB1, 99.f, centerA ); // If this doesn't compile, make sure you return a bool ("wasPushed")
	Function_PushDiscOutOfPoint2D( centerB2, 101.f, centerA );
	bool wasB3Pushed = Function_PushDiscOutOfPoint2D( centerB3, 200.f, centerA ); 
	bool wasA1Pushed = Function_PushDiscOutOfPoint2D( centerA1, 200.f, centerB ); // If this doesn't compile, check that your last argument (point) is "Vec2 const&"
	VerifyTestResult( wasB1Pushed == false,							"PushDiscOutOfPoint2D() returned true for point outside disc (incorrect - should NOT have pushed!)" );
	VerifyTestResult( IsMostlyEqual( centerB1, centerB ),			"PushDiscOutOfPoint2D() incorrect for point outside disc (should not push)" );
	VerifyTestResult( IsMostlyEqual( centerB2, 580.8f, 160.6f ),	"PushDiscOutOfPoint2D() incorrect for point slightly inside disc" );
	VerifyTestResult( wasB3Pushed == true,							"PushDiscOutOfPoint2D() returned false for point deep inside disc (incorrect - should have pushed!)" );
	VerifyTestResult( IsMostlyEqual( centerB3, 660.f, 220.f ),		"PushDiscOutOfPoint2D() incorrect for point deep inside disc" );
	VerifyTestResult( wasA1Pushed == true,							"PushDiscOutOfPoint2D() returned false for point deep inside disc (incorrect - should have pushed!)" );
	VerifyTestResult( IsMostlyEqual( centerA1, 420.f, 40.f ),		"PushDiscOutOfPoint2D() incorrect for point deep inside disc" );

	Function_PushDiscOutOfDisc2D( centerB4, 90.f, centerA, 110.f );
	bool wasA2Pushed = Function_PushDiscOutOfDisc2D( centerA2, 50.f, centerB, 49.f ); // If this doesn't compile, make sure your second center (third argument) is "Vec2 const&", and returns a bool ("wasPushed")
	bool wasA3Pushed = Function_PushDiscOutOfDisc2D( centerA3, 31.f, centerB, 70.f );
	bool wasA4Pushed = Function_PushDiscOutOfDisc2D( centerA4, 110.f, centerB, 90.f );
	VerifyTestResult( IsMostlyEqual( centerB4, 660.f, 220.f ),		"PushDiscOutOfDisc2D() incorrect for grossly-overlapping discs" );
	VerifyTestResult( wasA2Pushed == false,							"PushDiscOutOfDisc2D() returned true for non-overlapping discs (incorrect - should NOT have pushed!)" );
	VerifyTestResult( IsMostlyEqual( centerA2, centerA ),			"PushDiscOutOfDisc2D() incorrect for non-overlapping discs" );
	VerifyTestResult( wasA3Pushed == true,							"PushDiscOutOfDisc2D() returned false for slightly-overlapping discs (incorrect - should have pushed!)" );
	VerifyTestResult( IsMostlyEqual( centerA3, 499.2f, 99.4f ),		"PushDiscOutOfDisc2D() incorrect for slightly-overlapping discs" );
	VerifyTestResult( wasA4Pushed == true,							"PushDiscOutOfDisc2D() returned false for grossly-overlapping discs (incorrect - should have pushed!)" );
	VerifyTestResult( IsMostlyEqual( centerA4, 420.f, 40.f ),		"PushDiscOutOfDisc2D() incorrect for grossly-overlapping discs" );

	bool wereA5B5Pushed = Function_PushDiscsOutOfEachOther2D( centerA5, 50.f, centerB5, 49.f );  // Case #1: If this doesn't compile, make sure you return a bool ("wasPushed")
	bool wereA6B6Pushed = Function_PushDiscsOutOfEachOther2D( centerA6, 50.f, centerB6, 51.f );  // Case #2
	bool wereA7B7Pushed = Function_PushDiscsOutOfEachOther2D( centerA7, 10.f, centerB7, 140.f ); // Case #3
	bool wereA8B8Pushed = Function_PushDiscsOutOfEachOther2D( centerA8, 120.f, centerB8, 80.f ); // Case #4
	VerifyTestResult( wereA5B5Pushed == false,						"PushDiscsOutOfEachOther2D() returned true for non-overlapping discs (case #1 incorrect - should NOT have pushed!)" );
	VerifyTestResult( IsMostlyEqual( centerA5, centerA ),			"PushDiscsOutOfEachOther2D() first disc incorrect for non-overlapping discs (case #1a - should not have moved)" );
	VerifyTestResult( IsMostlyEqual( centerB5, centerB ),			"PushDiscsOutOfEachOther2D() second disc incorrect for non-overlapping discs (case #1b - should not have moved)" );
	VerifyTestResult( wereA6B6Pushed == true,						"PushDiscsOutOfEachOther2D() returned false for overlapping discs (case #2 incorrect - should have pushed!)" );
	VerifyTestResult( IsMostlyEqual( centerA6, 499.6f, 99.7f ),		"PushDiscsOutOfEachOther2D() first disc incorrect for overlapping discs (case #2a)" );
	VerifyTestResult( IsMostlyEqual( centerB6, 580.4f, 160.3f ),	"PushDiscsOutOfEachOther2D() second disc incorrect for overlapping discs (case #2b)" );
	VerifyTestResult( wereA7B7Pushed == true,						"PushDiscsOutOfEachOther2D() returned false for overlapping discs (case #3 incorrect - should have pushed!)" );
	VerifyTestResult( IsMostlyEqual( centerA7, 480.f, 85.f ),		"PushDiscsOutOfEachOther2D() first disc incorrect for overlapping discs (case #3a)" );
	VerifyTestResult( IsMostlyEqual( centerB7, 600.f, 175.f ),		"PushDiscsOutOfEachOther2D() second disc incorrect for overlapping discs (case #3b)" );
	VerifyTestResult( wereA8B8Pushed == true,						"PushDiscsOutOfEachOther2D() returned false for overlapping discs (case #4 incorrect - should have pushed!)" );
	VerifyTestResult( IsMostlyEqual( centerA8, 460.f, 70.f ),		"PushDiscsOutOfEachOther2D() first disc incorrect for overlapping discs (case #4a)" );
	VerifyTestResult( IsMostlyEqual( centerB8, 620.f, 190.f ),		"PushDiscsOutOfEachOther2D() second disc incorrect for overlapping discs (case #4b)" );

	AABB2Class box1( 560.f, 20.f, 580.f, 170.f );
	const AABB2Class box2( 350.f, 155.f, 800.f, 400.f );
	const AABB2Class box3( -800.f, -900.f, 469.f, 59.f );
	Function_PushDiscOutOfAABB2D( centerA9, smallRadiusA, box1 );
	Function_PushDiscOutOfAABB2D( centerA10, smallRadiusA, box2 ); // If this doesn't compile, check your const-ness for the AABB2 argument!
	bool wasA11Pushed = Function_PushDiscOutOfAABB2D( centerA11, smallRadiusA, box3 ); // If this doesn't compile, make sure you return a bool ("wasPushed")
	VerifyTestResult( wasA11Pushed == false,					"PushDiscOutOfBox2D() returned true for non-overlapping case #1 (incorrect - should NOT have pushed!)" );
	VerifyTestResult( IsMostlyEqual( centerA9, centerA ),		"PushDiscOutOfBox2D() incorrect for non-overlapping case #1" );
	VerifyTestResult( IsMostlyEqual( centerA10, centerA ),		"PushDiscOutOfBox2D() incorrect for non-overlapping case #2" );
	VerifyTestResult( IsMostlyEqual( centerA11, centerA ),		"PushDiscOutOfBox2D() incorrect for non-overlapping case #3" );

	Function_PushDiscOutOfAABB2D( centerA9, largeRadiusA, box1 );
	Function_PushDiscOutOfAABB2D( centerA10, largeRadiusA, box2 ); // If this doesn't compile, check your const-ness for the AABB2 argument!
	wasA11Pushed = Function_PushDiscOutOfAABB2D( centerA11, largeRadiusA, box3 ); // If this doesn't compile, make sure you return a bool ("wasPushed")
	VerifyTestResult( wasA11Pushed == true,									"PushDiscOutOfBox2D() returned false for overlapping case #1 (incorrect - should have pushed!)" );
	VerifyTestResult( IsMostlyEqual( centerA9, 480.f, 100.f ),				"PushDiscOutOfBox2D() incorrect for overlapping case #1" );
	VerifyTestResult( IsMostlyEqual( centerA10, 500.f, 75.f ),				"PushDiscOutOfBox2D() incorrect for overlapping case #2" );
	VerifyTestResult( IsMostlyEqual( centerA11, 517.248657f, 122.812744f ),	"PushDiscOutOfBox2D() incorrect for overlapping case #3" );

#endif
	return 40; // Number of tests expected
}


//-----------------------------------------------------------------------------------------------
int TestSet_MP1A4_ProjectAndReflect()
{
#if defined( ENABLE_TestSet_MP1A4_ProjectAndReflect )

	Vector2Class x( 1.f, 0.f );
	Vector2Class const y( 0.f, 1.f );
	Vector2Class const z( 10.f, 0.f );
	Vector2Class const i( 0.8f, 0.6f );
	Vector2Class const j( -0.6f, 0.8f );
	Vector2Class const m( -0.8f, 0.6f );
	Vector2Class const n( -0.8f, -0.6f );
	Vector2Class const p( -0.6f, -0.8f );
	Vector2Class const a( 8.f, 6.f );
	Vector2Class b( 120.f, 50.f );
	Vector2Class c( -300.f, -400.f );

	float Xx = Function_GetProjectedLength2D( x, x );
	float Ax = Function_GetProjectedLength2D( a, x ); // If this doesn't compile, check your const-ness of your first argument (should be "Vec2 const&")
	float Ay = Function_GetProjectedLength2D( a, y ); // If this doesn't compile, check your const-ness of your second argument (should be "Vec2 const&")
	float Ai = Function_GetProjectedLength2D( a, i );
	float Aj = Function_GetProjectedLength2D( a, j );
	float Ap = Function_GetProjectedLength2D( a, p );
	float Ii = Function_GetProjectedLength2D( i, i );
	float Az = Function_GetProjectedLength2D( a, z );
	float Bi = Function_GetProjectedLength2D( b, i );
	float Ba = Function_GetProjectedLength2D( b, a );
	float Ab = Function_GetProjectedLength2D( a, b );
	float Mn = Function_GetProjectedLength2D( m, n );
	float Nm = Function_GetProjectedLength2D( n, m );
	float Bc = Function_GetProjectedLength2D( b, c );
	float Bb = Function_GetProjectedLength2D( b, b );
	float Cc = Function_GetProjectedLength2D( c, c );

	VerifyTestResult( IsMostlyEqual( Xx, 1.f ),		"GetProjectedLength2D() failed for case Xx (project x-axis onto itself)" );
	VerifyTestResult( IsMostlyEqual( Ax, 8.f ),		"GetProjectedLength2D() failed for case Ax (project onto x axis)" );
	VerifyTestResult( IsMostlyEqual( Ay, 6.f ),		"GetProjectedLength2D() failed for case Ay (project onto y axis)" );
	VerifyTestResult( IsMostlyEqual( Ai, 10.f ),	"GetProjectedLength2D() failed for case Ai (project onto normalized iBasis)" );
	VerifyTestResult( IsMostlyEqual( Aj, 0.f ),		"GetProjectedLength2D() failed for case Aj (project onto normalized jBasis)" );
	VerifyTestResult( IsMostlyEqual( Ap, -9.6f ),	"GetProjectedLength2D() failed for case Ap (project onto normalized -xy)" );
	VerifyTestResult( IsMostlyEqual( Ii, 1.f ),		"GetProjectedLength2D() failed for case Ii (project onto normalized self)" );
	VerifyTestResult( IsMostlyEqual( Az, 8.f ),		"GetProjectedLength2D() failed for case Az (project onto a non-normalized vector)" );
	VerifyTestResult( IsMostlyEqual( Bi, 126.f ),	"GetProjectedLength2D() failed for case B-projectedLengthOn-i" );
	VerifyTestResult( IsMostlyEqual( Ba, Bi ),		"GetProjectedLength2D() failed for case B-projectedLengthOn-a" );
	VerifyTestResult( IsMostlyEqual( Ab, 9.69230747f ), "GetProjectedLength2D() failed for case A-projectedLengthOn-b" );
	VerifyTestResult( IsMostlyEqual( Mn, 0.28f ),	"GetProjectedLength2D() failed for case Mn (both normalized, same as Nm)" );
	VerifyTestResult( IsMostlyEqual( Nm, Nm ),		"GetProjectedLength2D() failed for case Nm (both normalized, same as Mn)" );
	VerifyTestResult( IsMostlyEqual( Bc, -112.f ),	"GetProjectedLength2D() failed for case B-projectedLengthOn-c" );
	VerifyTestResult( IsMostlyEqual( Bb, 130.f ),	"GetProjectedLength2D() failed for case Bb (project onto non-normalized self)" );
	VerifyTestResult( IsMostlyEqual( Cc, 500.f ),	"GetProjectedLength2D() failed for case Cc (project onto non-normalized self)" );

	Vector2Class vecXx = Function_GetProjectedOnto2D( x, x );
	Vector2Class vecAx = Function_GetProjectedOnto2D( a, x ); // If this doesn't compile, check your const-ness of your first argument (should be "Vec2 const&")
	Vector2Class vecAy = Function_GetProjectedOnto2D( a, y ); // If this doesn't compile, check your const-ness of your second argument (should be "Vec2 const&")
	Vector2Class vecAi = Function_GetProjectedOnto2D( a, i );
	Vector2Class vecAj = Function_GetProjectedOnto2D( a, j );
	Vector2Class vecAp = Function_GetProjectedOnto2D( a, p );
	Vector2Class vecIi = Function_GetProjectedOnto2D( i, i );
	Vector2Class vecAz = Function_GetProjectedOnto2D( a, z );
	Vector2Class vecBi = Function_GetProjectedOnto2D( b, i );
	Vector2Class vecBa = Function_GetProjectedOnto2D( b, a );
	Vector2Class vecAb = Function_GetProjectedOnto2D( a, b );
	Vector2Class vecMn = Function_GetProjectedOnto2D( m, n );
	Vector2Class vecNm = Function_GetProjectedOnto2D( n, m );
	Vector2Class vecBc = Function_GetProjectedOnto2D( b, c );
	Vector2Class vecBb = Function_GetProjectedOnto2D( b, b );
	Vector2Class vecCc = Function_GetProjectedOnto2D( c, c );

	VerifyTestResult( IsMostlyEqual( vecXx, 1.f, 0.f ),			"GetProjectedOnto2D() failed for x-projectOnto-x" );
	VerifyTestResult( IsMostlyEqual( vecAx, 8.f, 0.f ),			"GetProjectedOnto2D() failed for A-projectOnto-x" );
	VerifyTestResult( IsMostlyEqual( vecAy, 0.f, 6.f ),			"GetProjectedOnto2D() failed for A-projectOnto-y" );
	VerifyTestResult( IsMostlyEqual( vecAi, 8.f, 6.f ),			"GetProjectedOnto2D() failed for A-projectOnto-i" );
	VerifyTestResult( IsMostlyEqual( vecAj, 0.f, 0.f ),			"GetProjectedOnto2D() failed for A-projectOnto-j" );
	VerifyTestResult( IsMostlyEqual( vecAp, 5.76f, 7.68f ),		"GetProjectedOnto2D() failed for A-projectOnto-p" );
	VerifyTestResult( IsMostlyEqual( vecIi, 0.8f, 0.6f ),		"GetProjectedOnto2D() failed for I-projectOnto-i" );
	VerifyTestResult( IsMostlyEqual( vecAz, 8.f, 0.f ),			"GetProjectedOnto2D() failed for A-projectOnto-z" );
	VerifyTestResult( IsMostlyEqual( vecBi, 100.8f, 75.6f ),	"GetProjectedOnto2D() failed for B-projectOnto-i" );
	VerifyTestResult( IsMostlyEqual( vecBa, vecBi ),			"GetProjectedOnto2D() failed for B-projectOnto-a" );
	VerifyTestResult( IsMostlyEqual( vecAb, 8.94674587f, 3.72781062f ), "GetProjectedOnto2D() failed for A-projectOnto-b" );
	VerifyTestResult( IsMostlyEqual( vecMn, -0.224f, -0.168f ),	"GetProjectedOnto2D() failed for M-projectOnto-n" );
	VerifyTestResult( IsMostlyEqual( vecNm, vecNm ),			"GetProjectedOnto2D() failed for N-projectOnto-m" );
	VerifyTestResult( IsMostlyEqual( vecBc, 67.2f, 89.6f ),		"GetProjectedOnto2D() failed for B-projectOnto-c" );
	VerifyTestResult( IsMostlyEqual( vecBb, 120.f, 50.f ),		"GetProjectedOnto2D() failed for B-projectOnto-b" );
	VerifyTestResult( IsMostlyEqual( vecCc, -300.f, -400.f ),	"GetProjectedOnto2D() failed for C-projectOnto-c" );

	float degBtwnXX = Function_GetDegreesBetweenVectors2D( x, x );
	float degBtwnXY = Function_GetDegreesBetweenVectors2D( x, y ); // If this doesn't compile, check your const-ness of your second argument (should be "Vec2 const&")
	float degBtwnII = Function_GetDegreesBetweenVectors2D( i, i ); // If this doesn't compile, check your const-ness of your first argument (should be "Vec2 const&")
	float degBtwnIJ = Function_GetDegreesBetweenVectors2D( i, j );
	float degBtwnIN = Function_GetDegreesBetweenVectors2D( i, n );
	float degBtwnAI = Function_GetDegreesBetweenVectors2D( a, i ); // If this test fails, consider a's length...
	float degBtwnIM = Function_GetDegreesBetweenVectors2D( i, m );
	float degBtwnAM = Function_GetDegreesBetweenVectors2D( a, m );
	float degBtwnAB = Function_GetDegreesBetweenVectors2D( a, b );
	float degBtwnBA = Function_GetDegreesBetweenVectors2D( b, a );
	float degBtwnBC = Function_GetDegreesBetweenVectors2D( b, c );
	float degBtwnCB = Function_GetDegreesBetweenVectors2D( c, b );
	float degBtwnCC = Function_GetDegreesBetweenVectors2D( c, c );

	VerifyTestResult( IsMostlyEqual( degBtwnXX, 0.f ),			"GetDegreesBetweenVectors2D() incorrect for x-vs-x" );
	VerifyTestResult( IsMostlyEqual( degBtwnXY, 90.f ),			"GetDegreesBetweenVectors2D() incorrect for x-vs-y" );
	VerifyTestResult( IsMostlyEqual( degBtwnII, 0.f ),			"GetDegreesBetweenVectors2D() incorrect for x-vs-i" );
	VerifyTestResult( IsMostlyEqual( degBtwnIJ, 90.f ),			"GetDegreesBetweenVectors2D() incorrect for i-vs-j" );
	VerifyTestResult( IsMostlyEqual( degBtwnIN, 180.f ),		"GetDegreesBetweenVectors2D() incorrect for i-vs-n" );
	VerifyTestResult( IsMostlyEqual( degBtwnAI, 0.f ),			"GetDegreesBetweenVectors2D() incorrect for i-vs-i" );
	VerifyTestResult( IsMostlyEqual( degBtwnIM, 106.260201f ),	"GetDegreesBetweenVectors2D() incorrect for a-vs-m" );
	VerifyTestResult( IsMostlyEqual( degBtwnAM, degBtwnIM ),	"GetDegreesBetweenVectors2D() incorrect for i-vs-m" );
	VerifyTestResult( IsMostlyEqual( degBtwnAB, 14.2500315f ),	"GetDegreesBetweenVectors2D() incorrect for a-vs-b" );
	VerifyTestResult( IsMostlyEqual( degBtwnBA, degBtwnAB ),	"GetDegreesBetweenVectors2D() incorrect for a-vs-a" );
	VerifyTestResult( IsMostlyEqual( degBtwnBC, 149.489761f ),	"GetDegreesBetweenVectors2D() incorrect for b-vs-c" );
	VerifyTestResult( IsMostlyEqual( degBtwnCB, degBtwnBC ),	"GetDegreesBetweenVectors2D() incorrect for b-vs-b" );
	VerifyTestResult( IsMostlyEqual( degBtwnCC, 0.f ),			"GetDegreesBetweenVectors2D() incorrect for c-vs-c" );

	Vector2Class x_reflectOffNormal_x = x.Vec2_GetReflected( x );
	Vector2Class x_reflectOffNormal_y = x.Vec2_GetReflected( y ); // If this doesn't compile, check your const-ness of your argument (should be "Vec2 const&")
	Vector2Class a_reflectOffNormal_x = a.Vec2_GetReflected( x ); // If this doesn't compile, make sure your Vec2::GetReflected() function is a "const" method, i.e.   Vec2::GetReflected(...) const <--- 
	Vector2Class a_reflectOffNormal_y = a.Vec2_GetReflected( y );
	Vector2Class a_reflectOffNormal_i = a.Vec2_GetReflected( i );
	Vector2Class a_reflectOffNormal_j = a.Vec2_GetReflected( j );
	Vector2Class b_reflectOffNormal_i = b.Vec2_GetReflected( i );
	Vector2Class b_reflectOffNormal_p = b.Vec2_GetReflected( p );
	Vector2Class c_reflectOffNormal_i = c.Vec2_GetReflected( i );
	Vector2Class c_reflectOff_a		  = c.Vec2_GetReflected( a ); // This should NOT be the same result as if a were normalized; if it is, you're normalizing (and shouldn't be!)

	VerifyTestResult( IsMostlyEqual( x_reflectOffNormal_x, -1.f, 0.f ),			"Vec2::  x.GetReflected( x ) was incorrect" );
	VerifyTestResult( IsMostlyEqual( x_reflectOffNormal_y, 1.f, 0.f ),			"Vec2::  x.GetReflected( y ) was incorrect" );
	VerifyTestResult( IsMostlyEqual( a_reflectOffNormal_x, -8.f, 6.f ),			"Vec2::  a.GetReflected( x ) was incorrect" );
	VerifyTestResult( IsMostlyEqual( a_reflectOffNormal_y, 8.f, -6.f ),			"Vec2::  a.GetReflected( y ) was incorrect" );
	VerifyTestResult( IsMostlyEqual( a_reflectOffNormal_i, -8.f, -6.f ),		"Vec2::  a.GetReflected( i ) was incorrect" );
	VerifyTestResult( IsMostlyEqual( a_reflectOffNormal_j, 8.f, 6.f ),			"Vec2::  a.GetReflected( j ) was incorrect" );
	VerifyTestResult( IsMostlyEqual( b_reflectOffNormal_i, -81.6f, -101.2f ),	"Vec2::  b.GetReflected( i ) was incorrect" );
	VerifyTestResult( IsMostlyEqual( b_reflectOffNormal_p, -14.4f, -129.2f ),	"Vec2::  b.GetReflected( p ) was incorrect" );
	VerifyTestResult( IsMostlyEqual( c_reflectOffNormal_i, 468.f, 176.f ),		"Vec2::  c.GetReflected( i ) was incorrect" );
	VerifyTestResult( false == IsMostlyEqual( c_reflectOff_a, 468.f, 176.f ),	"Vec2::  c.GetReflected( a ) was incorrect; note: do NOT normalize 'a' inside!" );

	Vector2Class c2( c );
	b.Vec2_Reflect( p );
	c.Vec2_Reflect( i );
	c2.Vec2_Reflect( a ); // a is the same as i, but NOT normalized; this should NOT produce the same result!  If it does, you must be normalizing 'a' inside the function, which we want to avoid.

	VerifyTestResult( IsMostlyEqual( b, -14.4f, -129.2f ),			"Vec2::  b.Reflect( p ) was incorrect" );
	VerifyTestResult( IsMostlyEqual( c, 468.f, 176.f ),				"Vec2::  c.Reflect( i ) was incorrect" );
	VerifyTestResult( false == IsMostlyEqual( c2, 468.f, 176.f ),	"Vec2::  c.Reflect( a ) was incorrect; note: do NOT normalize 'a' inside!" );

#endif
	return 58; // Number of tests expected
}


//////////////////////////////////////////////////////////////////////////////////////////////////
// TEMPORARY HACK to correct a forward-compatibility issue with other unit tests
//////////////////////////////////////////////////////////////////////////////////////////////////
#if defined( ENABLE_TestSet_MP1A4_TransformUsingIJ )
//------------------------------------------------------------------------------------------------
Vector2Class MathUnitTest_TransformPosition2D_IJ( Vector2Class const& pos, Vector2Class const& iBasis, Vector2Class const& jBasis, Vector2Class const& translation )
{
	Vector2Class transformedPos = pos;
	Function_TransformPosition2D_IJ( transformedPos, iBasis, jBasis, translation );
	return transformedPos;
}


//------------------------------------------------------------------------------------------------
Vector3Class MathUnitTest_TransformPositionXY3D_IJ( Vector3Class const& pos, Vector2Class const& iBasisXY, Vector2Class const& jBasisXY, Vector2Class const& translationXY )
{
	Vector3Class transformedPos = pos;
	Function_TransformPositionXY3D_IJ( transformedPos, iBasisXY, jBasisXY, translationXY );
	return transformedPos;
}
#endif
//////////////////////////////////////////////////////////////////////////////////////////////////
// END HACK
//////////////////////////////////////////////////////////////////////////////////////////////////


//-----------------------------------------------------------------------------------------------
int TestSet_MP1A4_TransformUsingIJ()
{
#if defined( ENABLE_TestSet_MP1A4_TransformUsingIJ )

	Vector2Class origin( 0.f, 0.f );
	Vector2Class x( 1.f, 0.f );
	Vector2Class y( 0.f, 1.f );
	Vector2Class const i( 0.8f, 0.6f );
	Vector2Class const j( -0.6f, 0.8f );
	Vector2Class const I( -8.f, -6.f );
	Vector2Class const J( 6.f, -8.f );
	Vector2Class const p( 800.f, 100.f );
	Vector2Class const q( -500.f, -1200.f );	

	Vector2Class a = MathUnitTest_TransformPosition2D_IJ(origin, x, y, origin);	// Identity on origin
	Vector2Class b = MathUnitTest_TransformPosition2D_IJ(p, x, y, origin);		// Identity on P
	Vector2Class c = MathUnitTest_TransformPosition2D_IJ(origin, i, j, origin);	// Rotation on origin (does nothing)
	Vector2Class d = MathUnitTest_TransformPosition2D_IJ( p, i, j, origin );		// Rotation (only) on P
	Vector2Class e = MathUnitTest_TransformPosition2D_IJ( origin, x, y, q );		// Translation (only) of origin
	Vector2Class f = MathUnitTest_TransformPosition2D_IJ( p, x, y, q );				// Translation (only) of P
	Vector2Class g = MathUnitTest_TransformPosition2D_IJ( q, i, j, p );				// Rotation and translation of Q
	Vector2Class h = MathUnitTest_TransformPosition2D_IJ( p, x*5.f, y*5.f, origin ); // 5x uniform scale of P
	Vector2Class m = MathUnitTest_TransformPosition2D_IJ( p, I, J, origin );		// Rotation and 10x uniform scale of P
	Vector2Class n = MathUnitTest_TransformPosition2D_IJ( p, I, J, q );				// Rotation, 10x uniform scale, and Translation of P
	Vector2Class o = MathUnitTest_TransformPosition2D_IJ( q, i, J, p );				// Rotation, (1,10)x non-uniform scale, and Translation of Q

	VerifyTestResult(IsMostlyEqual(a, 0.f, 0.f), "TransformPosition2D() with i,j was incorrect for Identity on origin");
	VerifyTestResult(IsMostlyEqual(b, 800.f, 100.f), "TransformPosition2D() with i,j was incorrect for Identity on P");
	VerifyTestResult(IsMostlyEqual(c, 0.f, 0.f), "TransformPosition2D() with i,j was incorrect for Rotation on origin (does nothing)");
	VerifyTestResult(IsMostlyEqual(d, 580.f, 560.f), "TransformPosition2D() with i,j was incorrect for Rotation (only) on P");
	VerifyTestResult( IsMostlyEqual( e, -500.f, -1200.f ),	"TransformPosition2D() with i,j was incorrect for Translation (only) of origin" );
	VerifyTestResult( IsMostlyEqual( f, 300.f, -1100.f ),	"TransformPosition2D() with i,j was incorrect for Translation (only) of P" );
	VerifyTestResult( IsMostlyEqual( g, 1120.f, -1160.f ),	"TransformPosition2D() with i,j was incorrect for Rotation and translation of Q" );
	VerifyTestResult( IsMostlyEqual( h, 4000.f, 500.f ),		"TransformPosition2D() with i,j was incorrect for 5x uniform scale of P" );
	VerifyTestResult( IsMostlyEqual( m, -5800.f, -5600.f ), "TransformPosition2D() with i,j was incorrect for Rotation and 10x uniform scale of P" );
	VerifyTestResult( IsMostlyEqual( n, -6300.f, -6800.f ), "TransformPosition2D() with i,j was incorrect for Rotation, 10x uniform scale, and Translation of P" );
	VerifyTestResult( IsMostlyEqual( o, -6800.f, 9400.f ),	"TransformPosition2D() with i,j was incorrect for Rotation, (1,10)x non-uniform scale, and Translation of Q" );

	Vector3Class origin3D( 0.f, 0.f, 0.f );
	Vector3Class x3D( 1.f, 0.f, 0.f );
	Vector3Class y3D( 0.f, 1.f, 0.f );
	Vector3Class const i3D( 0.8f, 0.6f, 0.f );
	Vector3Class const j3D( -0.6f, 0.8f, 0.f );
	Vector3Class const I3D( -8.f, -6.f, 0.f );
	Vector3Class const J3D( 6.f, -8.f, 0.f );
	Vector3Class const p3D( 800.f, 100.f, 0.f );
	Vector3Class const q3D( -500.f, -1200.f, 5.f ); // Note: z=5 here - must be preserved through all 2D (XY in 3D) transforms!

	Vector3Class a3D = MathUnitTest_TransformPositionXY3D_IJ( origin3D, x, y, origin );	// Identity on origin
	Vector3Class b3D = MathUnitTest_TransformPositionXY3D_IJ( p3D, x, y, origin );		// Identity on P
	Vector3Class c3D = MathUnitTest_TransformPositionXY3D_IJ( origin3D, i, j, origin );	// Rotation on origin (does nothing)
	Vector3Class d3D = MathUnitTest_TransformPositionXY3D_IJ( p3D, i, j, origin );		// Rotation (only) on P
	Vector3Class e3D = MathUnitTest_TransformPositionXY3D_IJ( origin3D, x, y, q );		// Translation (only) of origin
	Vector3Class f3D = MathUnitTest_TransformPositionXY3D_IJ( p3D, x, y, q );			// Translation (only) of P
	Vector3Class g3D = MathUnitTest_TransformPositionXY3D_IJ( q3D, i, j, p );			// Rotation and translation of Q
	Vector3Class h3D = MathUnitTest_TransformPositionXY3D_IJ( p3D, x*5.f, y*5.f, origin ); // 5x uniform scale of P
	Vector3Class m3D = MathUnitTest_TransformPositionXY3D_IJ( p3D, I, J, origin );		// Rotation and 10x uniform scale of P
	Vector3Class n3D = MathUnitTest_TransformPositionXY3D_IJ( p3D, I, J, q );			// Rotation, 10x uniform scale, and Translation of P
	Vector3Class o3D = MathUnitTest_TransformPositionXY3D_IJ( q3D, i, J, p );			// Rotation, (1,10)x non-uniform scale, and Translation of Q

	VerifyTestResult( IsMostlyEqual( a3D, 0.f, 0.f, 0.f ),			"TransformPosition3DXY() with i,j was incorrect for Identity on origin" );
	VerifyTestResult( IsMostlyEqual( b3D, 800.f, 100.f, 0.f ),		"TransformPosition3DXY() with i,j was incorrect for Identity on P" );
	VerifyTestResult( IsMostlyEqual( c3D, 0.f, 0.f, 0.f ),			"TransformPosition3DXY() with i,j was incorrect for Rotation on origin (does nothing)" );
	VerifyTestResult( IsMostlyEqual( d3D, 580.f, 560.f, 0.f ),		"TransformPosition3DXY() with i,j was incorrect for Rotation (only) on P" );
	VerifyTestResult( IsMostlyEqual( e3D, -500.f, -1200.f, 0.f ),	"TransformPosition3DXY() with i,j was incorrect for Translation (only) of origin" );
	VerifyTestResult( IsMostlyEqual( f3D, 300.f, -1100.f, 0.f ),	"TransformPosition3DXY() with i,j was incorrect for Translation (only) of P" );
	VerifyTestResult( IsMostlyEqual( g3D, 1120.f, -1160.f, 5.f ),	"TransformPosition3DXY() with i,j was incorrect for Rotation and translation of Q (preserving Z=5!)" );
	VerifyTestResult( IsMostlyEqual( h3D, 4000.f, 500.f, 0.f ),		"TransformPosition3DXY() with i,j was incorrect for 5x uniform scale of P" );
	VerifyTestResult( IsMostlyEqual( m3D, -5800.f, -5600.f, 0.f ),	"TransformPosition3DXY() with i,j was incorrect for Rotation and 10x uniform scale of P" );
	VerifyTestResult( IsMostlyEqual( n3D, -6300.f, -6800.f, 0.f ),	"TransformPosition3DXY() with i,j was incorrect for Rotation, 10x uniform scale, and Translation of P" );
	VerifyTestResult( IsMostlyEqual( o3D, -6800.f, 9400.f, 5.f ),	"TransformPosition3DXY() with i,j was incorrect for Rotation, (1,10)x non-uniform scale, and Translation of Q (preserving Z=5!)" );

#endif
	return 22; // Number of tests expected
}


//-----------------------------------------------------------------------------------------------
int TestSet_MP1A4_MiscGeometricUtilities()
{
#if defined( ENABLE_TestSet_MP1A4_MiscGeometricUtilities )

	// Tests for: Taxicab Distance (IntVec2 standalone)
	IntVec2Class origin( 0, 0 );
	IntVec2Class a( 5, 0 );
	IntVec2Class const b( 4, 3 );
	IntVec2Class const c( -2, -1 );
	int taxiOA = Function_GetTaxicabDistance2D( origin, a );
	int taxiAO = Function_GetTaxicabDistance2D( a, origin );
	int taxiAB = Function_GetTaxicabDistance2D( a, b ); // If this doesn't compile, make sure your second argument is "const IntVec2&"
	int taxiBA = Function_GetTaxicabDistance2D( b, a ); // If this doesn't compile, make sure your first argument is "const IntVec2&"
	int taxiBC = Function_GetTaxicabDistance2D( b, c );
	int taxiCA = Function_GetTaxicabDistance2D( c, a );
	int taxiCC = Function_GetTaxicabDistance2D( c, c );

	VerifyTestResult( taxiOA == 5, "GetTaxicabDistance2D() from (0,0) to (5,0) should be 5" );
	VerifyTestResult( taxiAO == 5, "GetTaxicabDistance2D() from (5,0) to (0,0) should be 5" );
	VerifyTestResult( taxiAB == 4, "GetTaxicabDistance2D() from (5,0) to (4,3) should be 4" );
	VerifyTestResult( taxiBA == 4, "GetTaxicabDistance2D() from (4,3) to (5,0) should be 4" );
	VerifyTestResult( taxiBC == 10, "GetTaxicabDistance2D() from (4,3) to (-2,-1) should be 10" );
	VerifyTestResult( taxiCA == 8, "GetTaxicabDistance2D() from (-2,-1) to (5,0) should be 8" );
	VerifyTestResult( taxiCC == 0, "GetTaxicabDistance2D() from (-2,-1) to (-2,-1) should be 0" );

	// Tests for: degrees-oriented sector
	Vector2Class m( 0.f, 0.f );
	Vector2Class n( 2000.f, 1000.f );
	Vector2Class const p( -3000.f, -4000.f );
	Vector2Class e( 94.f, 34.f );
	Vector2Class f = n + e;
	Vector2Class const g = p + e;
	Vector2Class const h = p + Vector2Class( -40.f, 30.f ); // 50 units WNW of "p" (at a heading of about 143.1301 degrees)

	bool oriented_true1		= Function_IsPointInOrientedSector2D( e, m, 20.f, 10.f, 105.f );	// Sector just within reach (within maxRange by 5 units)
	bool oriented_false1a	= Function_IsPointInOrientedSector2D( e, m, 20.f, 10.f, 95.f );		// Sector just short of reach (beyond maxRange by 5 units)
	bool oriented_false1b	= Function_IsPointInOrientedSector2D( e, m, 30.f, 10.f, 105.f );	// Sector just to the left (centered +10 deg.) of point
	bool oriented_false1c	= Function_IsPointInOrientedSector2D( e, m, 10.f, 10.f, 105.f );	// Sector just to the right (centered -10 deg.) of point

	bool oriented_true2		= Function_IsPointInOrientedSector2D( f, n, 20.f, 10.f, 105.f );	// Sector just short of reach (beyond maxRange by 5 units)
	bool oriented_false2a	= Function_IsPointInOrientedSector2D( f, n, 20.f, 10.f, 95.f );		// Sector just short of reach (beyond maxRange by 5 units)
	bool oriented_false2b	= Function_IsPointInOrientedSector2D( f, n, 30.f, 10.f, 105.f );	// Sector just to the left (centered +10 deg.) of point
	bool oriented_false2c	= Function_IsPointInOrientedSector2D( f, n, 10.f, 10.f, 105.f );	// Sector just to the right (centered -10 deg.) of point

	bool oriented_true3		= Function_IsPointInOrientedSector2D( g, p, 20.f, 10.f, 105.f );
	bool oriented_false3	= Function_IsPointInOrientedSector2D( g, p, 20.f, 10.f, 95.f );	 // If this doesn't compile, check that your first two arguments are both "Vec2 const&"

	bool oriented_true4		= Function_IsPointInOrientedSector2D( h, p, 140.f, 8.f, 51.f );		// just within sector
	bool oriented_false4a	= Function_IsPointInOrientedSector2D( h, p, 140.f, 2.f, 51.f );		// just to the left of the (now-narrower) sector cone
	bool oriented_false4b	= Function_IsPointInOrientedSector2D( h, p, 146.f, 2.f, 51.f );		// just to the right of the (now-narrower) sector cone
	bool oriented_false4c	= Function_IsPointInOrientedSector2D( h, p, 143.f, 40.f, 49.f );	// just short of the (now-narrower) sector cone

	bool oriented_true5		= Function_IsPointInOrientedSector2D( h, p, 0.f, 288.f, 51.f );		// sector is now SO aperture-wide it looks like a pac-man (facing *away* from the "forward" direction)
	bool oriented_false5a	= Function_IsPointInOrientedSector2D( h, p, 0.f, 280.f, 51.f );		// pac-man mouth widened a bit, now point is no longer inside pac-man

	bool oriented_false6	= Function_IsPointInOrientedSector2D( h, p, 323.f, 350.f, 51.f );	// pac-man is now almost entire pie, with tiny 10-deg pie-piece sliver missing (and point is in the missing piece!)
	bool oriented_true6a	= Function_IsPointInOrientedSector2D( h, p, 313.f, 350.f, 51.f );	// pac-man is now almost entire pie, with tiny 10-deg pie-piece sliver missing (but point made it inside pac-man)
	bool oriented_true6b	= Function_IsPointInOrientedSector2D( h, p, 333.f, 350.f, 51.f );	// pac-man is now almost entire pie, with tiny 10-deg pie-piece sliver missing (but point made it inside pac-man)
	bool oriented_false6a	= Function_IsPointInOrientedSector2D( h, p, 313.f, 350.f, 49.f );	// same as true6a, but now just outside maxRange
	bool oriented_false6b	= Function_IsPointInOrientedSector2D( h, p, 333.f, 350.f, 49.f );	// same as true6b, but now just outside maxRange

	VerifyTestResult( oriented_true1 == true,	 "IsPointInOrientedSector2D() incorrect for case true1" );
	VerifyTestResult( oriented_false1a == false, "IsPointInOrientedSector2D() incorrect for case false1a" );
	VerifyTestResult( oriented_false1b == false, "IsPointInOrientedSector2D() incorrect for case false1b" );
	VerifyTestResult( oriented_false1c == false, "IsPointInOrientedSector2D() incorrect for case false1c" );
	VerifyTestResult( oriented_true2 == true,	 "IsPointInOrientedSector2D() incorrect for case true2" );
	VerifyTestResult( oriented_false2a == false, "IsPointInOrientedSector2D() incorrect for case false2a" );
	VerifyTestResult( oriented_false2b == false, "IsPointInOrientedSector2D() incorrect for case false2b" );
	VerifyTestResult( oriented_false2c == false, "IsPointInOrientedSector2D() incorrect for case false2c" );
	VerifyTestResult( oriented_true3 == true,	 "IsPointInOrientedSector2D() incorrect for case true3" );
	VerifyTestResult( oriented_false3 == false,	 "IsPointInOrientedSector2D() incorrect for case false3" );
	VerifyTestResult( oriented_true4 == true,	 "IsPointInOrientedSector2D() incorrect for case true4" );
	VerifyTestResult( oriented_false4a == false, "IsPointInOrientedSector2D() incorrect for case false4a" );
	VerifyTestResult( oriented_false4b == false, "IsPointInOrientedSector2D() incorrect for case false4b" );
	VerifyTestResult( oriented_false4c == false, "IsPointInOrientedSector2D() incorrect for case false4c" );
	VerifyTestResult( oriented_true5 == true,	 "IsPointInOrientedSector2D() incorrect for case true5" );
	VerifyTestResult( oriented_false5a == false, "IsPointInOrientedSector2D() incorrect for case false5a" );
	VerifyTestResult( oriented_false6 == false,	 "IsPointInOrientedSector2D() incorrect for case false6" );
	VerifyTestResult( oriented_true6a == true,	 "IsPointInOrientedSector2D() incorrect for case true6a" );
	VerifyTestResult( oriented_true6b == true,	 "IsPointInOrientedSector2D() incorrect for case true6b" );
	VerifyTestResult( oriented_false6a == false, "IsPointInOrientedSector2D() incorrect for case false6a" );
	VerifyTestResult( oriented_false6b == false, "IsPointInOrientedSector2D() incorrect for case false6b" );

	// Tests for: forward-direction sector
	Vector2Class const direction0Degrees	= MakeDirection2D( 0.f );
	Vector2Class const direction10Degrees	= MakeDirection2D( 10.f );
	Vector2Class const direction20Degrees	= MakeDirection2D( 20.f );
	Vector2Class const direction30Degrees	= MakeDirection2D( 30.f );
	Vector2Class const direction140Degrees	= MakeDirection2D( 140.f );
	Vector2Class const direction143Degrees	= MakeDirection2D( 143.f );
	Vector2Class const direction146Degrees	= MakeDirection2D( 146.f );
	Vector2Class const direction313Degrees	= MakeDirection2D( 313.f );
	Vector2Class const direction323Degrees	= MakeDirection2D( 323.f );
	Vector2Class const direction333Degrees	= MakeDirection2D( 333.f );

	bool directed_true1		= Function_IsPointInDirectedSector2D( e, m, direction20Degrees, 10.f, 105.f );	// Sector just within reach (within maxRange by 5 units)
	bool directed_false1a	= Function_IsPointInDirectedSector2D( e, m, direction20Degrees, 10.f, 95.f );	// Sector just short of reach (beyond maxRange by 5 units)
	bool directed_false1b	= Function_IsPointInDirectedSector2D( e, m, direction30Degrees, 10.f, 105.f );	// Sector just to the left (centered +10 deg.) of point
	bool directed_false1c	= Function_IsPointInDirectedSector2D( e, m, direction10Degrees, 10.f, 105.f );	// Sector just to the right (centered -10 deg.) of point

	bool directed_true2		= Function_IsPointInDirectedSector2D( f, n, direction20Degrees, 10.f, 105.f );	// Sector just short of reach (beyond maxRange by 5 units)
	bool directed_false2a	= Function_IsPointInDirectedSector2D( f, n, direction20Degrees, 10.f, 95.f );	// Sector just short of reach (beyond maxRange by 5 units)
	bool directed_false2b	= Function_IsPointInDirectedSector2D( f, n, direction30Degrees, 10.f, 105.f );	// Sector just to the left (centered +10 deg.) of point
	bool directed_false2c	= Function_IsPointInDirectedSector2D( f, n, direction10Degrees, 10.f, 105.f );	// Sector just to the right (centered -10 deg.) of point

	bool directed_true3		= Function_IsPointInDirectedSector2D( g, p, direction20Degrees, 10.f, 105.f );
	bool directed_false3	= Function_IsPointInDirectedSector2D( g, p, direction20Degrees, 10.f, 95.f ); // If this doesn't compile, check that your first two arguments are both "Vec2 const&"

	bool directed_true4		= Function_IsPointInDirectedSector2D( h, p, direction140Degrees, 8.f, 51.f );	// just within sector
	bool directed_false4a	= Function_IsPointInDirectedSector2D( h, p, direction140Degrees, 2.f, 51.f );	// just to the left of the (now-narrower) sector cone
	bool directed_false4b	= Function_IsPointInDirectedSector2D( h, p, direction146Degrees, 2.f, 51.f );	// just to the right of the (now-narrower) sector cone
	bool directed_false4c	= Function_IsPointInDirectedSector2D( h, p, direction143Degrees, 40.f, 49.f );	// just short of the (now-narrower) sector cone

	bool directed_true5		= Function_IsPointInDirectedSector2D( h, p, direction0Degrees, 288.f, 51.f );	// sector is now SO aperture-wide it looks like a pac-man (facing *away* from the "forward" direction)
	bool directed_false5a	= Function_IsPointInDirectedSector2D( h, p, direction0Degrees, 280.f, 51.f );	// pac-man mouth widened a bit, now point is no longer inside pac-man

	bool directed_false6	= Function_IsPointInDirectedSector2D( h, p, direction323Degrees, 350.f, 51.f );	// pac-man is now almost entire pie, with tiny 10-deg pie-piece sliver missing (and point is in the missing piece!)
	bool directed_true6a	= Function_IsPointInDirectedSector2D( h, p, direction313Degrees, 350.f, 51.f );	// pac-man is now almost entire pie, with tiny 10-deg pie-piece sliver missing (but point made it inside pac-man)
	bool directed_true6b	= Function_IsPointInDirectedSector2D( h, p, direction333Degrees, 350.f, 51.f );	// pac-man is now almost entire pie, with tiny 10-deg pie-piece sliver missing (but point made it inside pac-man)
	bool directed_false6a	= Function_IsPointInDirectedSector2D( h, p, direction313Degrees, 350.f, 49.f );	// same as true6a, but now just outside maxRange
	bool directed_false6b	= Function_IsPointInDirectedSector2D( h, p, direction333Degrees, 350.f, 49.f );	// same as true6b, but now just outside maxRange

	VerifyTestResult( directed_true1 == true,	 "IsPointInDirectedSector2D() incorrect for case true1" );
	VerifyTestResult( directed_false1a == false, "IsPointInDirectedSector2D() incorrect for case false1a" );
	VerifyTestResult( directed_false1b == false, "IsPointInDirectedSector2D() incorrect for case false1b" );
	VerifyTestResult( directed_false1c == false, "IsPointInDirectedSector2D() incorrect for case false1c" );
	VerifyTestResult( directed_true2 == true,	 "IsPointInDirectedSector2D() incorrect for case true2" );
	VerifyTestResult( directed_false2a == false, "IsPointInDirectedSector2D() incorrect for case false2a" );
	VerifyTestResult( directed_false2b == false, "IsPointInDirectedSector2D() incorrect for case false2b" );
	VerifyTestResult( directed_false2c == false, "IsPointInDirectedSector2D() incorrect for case false2c" );
	VerifyTestResult( directed_true3 == true,	 "IsPointInDirectedSector2D() incorrect for case true3" );
	VerifyTestResult( directed_false3 == false,	 "IsPointInDirectedSector2D() incorrect for case false3" );
	VerifyTestResult( directed_true4 == true,	 "IsPointInDirectedSector2D() incorrect for case true4" );
	VerifyTestResult( directed_false4a == false, "IsPointInDirectedSector2D() incorrect for case false4a" );
	VerifyTestResult( directed_false4b == false, "IsPointInDirectedSector2D() incorrect for case false4b" );
	VerifyTestResult( directed_false4c == false, "IsPointInDirectedSector2D() incorrect for case false4c" );
	VerifyTestResult( directed_true5 == true,	 "IsPointInDirectedSector2D() incorrect for case true5" );
	VerifyTestResult( directed_false5a == false, "IsPointInDirectedSector2D() incorrect for case false5a" );
	VerifyTestResult( directed_false6 == false,	 "IsPointInDirectedSector2D() incorrect for case false6" );
	VerifyTestResult( directed_true6a == true,	 "IsPointInDirectedSector2D() incorrect for case true6a" );
	VerifyTestResult( directed_true6b == true,	 "IsPointInDirectedSector2D() incorrect for case true6b" );
	VerifyTestResult( directed_false6a == false, "IsPointInDirectedSector2D() incorrect for case false6a" );
	VerifyTestResult( directed_false6b == false, "IsPointInDirectedSector2D() incorrect for case false6b" );

#endif
	return 49; // Number of tests expected
}


//-----------------------------------------------------------------------------------------------
void RunTests_MP1A4()
{
	printf( "Running tests for MP1-A4 (version 2)...\n" );
	RunTestSet( true, TestSet_MP1A4_PushOutOf,				"MP1-A4: Push-out-of" );
	RunTestSet( true, TestSet_MP1A4_ProjectAndReflect,		"MP1-A4: Project and reflect" );
	RunTestSet( true, TestSet_MP1A4_TransformUsingIJ,		"MP1-A4: Transform using IJ bases" );
	RunTestSet( true, TestSet_MP1A4_MiscGeometricUtilities,	"MP1-A4: Misc. geometric utilities" );
}


