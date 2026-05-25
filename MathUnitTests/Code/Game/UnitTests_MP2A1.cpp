//-----------------------------------------------------------------------------------------------
// UnitTests_MP2A1.cpp
//
#include "Game/UnitTests_MP2A1.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <vector>


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
//bool IsMostlyEqual( const Vector2Class& vec2, float x, float y );
//bool IsMostlyEqual( const Vector2Class& vec2a, const Vector2Class& vec2b );
//bool IsMostlyEqual( int a, int b, int epsilon=0 );
bool IsMostlyEqual( const Vector3Class& vec3, float x, float y, float z );
bool IsMostlyEqual( const Vector3Class& vec3a, const Vector3Class& vec3b );
//bool IsMostlyEqual( const AABB2Class& box1, const AABB2Class& box2 );
//bool IsMostlyEqual( const AABB2Class& box, const Vector2Class& mins, const Vector2Class& maxs );
//bool IsMostlyEqual( const AABB2Class& box, float minX, float minY, float maxX, float maxY );
//bool IsEqual( const IntVec2Class& a, const IntVec2Class& b );
//bool IsEqual( const IntVec2Class& ivec2, int x, int y );
//bool IsMostlyEqual( const Vector4Class& vec4, float x, float y, float z, float w );
//bool IsMostlyEqual( const Vector4Class& vec4a, const Vector4Class& vec4b );
//bool IsMostlyEqual( const float* floatArray1, const float* floatArray2, int numArrayElements );
bool IsMostlyEqual( const Matrix44Class& matrix, const float* sixteenCorrectMatrixValues );
//bool IsMostlyEqual( const Matrix44Class& mat44a, const Matrix44Class& mat44b );


//------------------------------------------------------------------------------------------------
// New helper functions introduced here
//


//////////////////////////////////////////////////////////////////////////////////////////////////
// Tests follow
//////////////////////////////////////////////////////////////////////////////////////////////////


//-----------------------------------------------------------------------------------------------
int TestSet_MP2A1_ByteDenormalization()
{
#if defined( ENABLE_TestSet_MP2A1_ByteDenormalization )

	// First, check that byte 0,255 map correctly back and forth between floats 0.f,1.f
	float f0 = Function_NormalizeByte( 0 );
	float f1 = Function_NormalizeByte( 255 );
	unsigned char b0 = Function_DenormalizeByte( 0.f );
	unsigned char b255 = Function_DenormalizeByte( 1.f );

	VerifyTestResult( f0 == 0.f,	"Byte (0) did not normalize correctly to float (0.00000000f)" );
	VerifyTestResult( f1 == 1.f,	"Byte (255) did not normalize correctly to float (1.00000000f)" );
	VerifyTestResult( b0 == 0,		"Float (0.f) did not denormalize correctly to byte (0)" );
	VerifyTestResult( b255 == 255,	"Float (1.f) did not denormalize correctly to byte (255)" );

	// Next, check that each byte normalizes to a float bigger than the previous,
	//  and that all normalized values are in [0.0,1.0],
	//  and that all bytes are preserved when normalized and then denormalized back.
	std::vector<unsigned char> bytesNotInZeroToOne;
	std::vector<unsigned char> bytesNotBigger;
	std::vector<unsigned char> bytesNotPreserved;
	float prevNormalized = -0.001f;
	for( int i = 0; i <= 255; ++ i )
	{
		unsigned char originalByte = (unsigned char) i;
		float normalized = Function_NormalizeByte( originalByte );
		if( normalized <= prevNormalized && i != 0 )
		{
			bytesNotBigger.push_back( originalByte );
		}
		if( normalized < 0.f || normalized > 1.f )
		{
			bytesNotInZeroToOne.push_back( originalByte );
		}
		unsigned char backToByte = Function_DenormalizeByte( normalized );
		if( backToByte != originalByte )
		{
			bytesNotPreserved.push_back( originalByte );
		}
		prevNormalized = normalized;
	}

	VerifyTestResult( bytesNotInZeroToOne.empty(), "One or more bytes normalized to a float outside of [0,1]:" );
	if( !bytesNotInZeroToOne.empty() )
	{
		printf( "\n" );
		for( int i = 0; i < (int) bytesNotInZeroToOne.size(); ++ i )
		{
			if( i == 5 )
			{
				printf( "  ...and %i other(s) were wrong as well\n", (int) bytesNotInZeroToOne.size() - 5 );
				break;
			}
			unsigned char badByte = bytesNotInZeroToOne[i];
			float badFloat = Function_NormalizeByte( badByte );
			printf( "  byte [%i] -> float [%.09f], but normalized floats must be within the range [0,1]\n", badByte, badFloat );
		}
	}

	VerifyTestResult( bytesNotBigger.empty(), "One or more bytes normalized to out-of-order float values (each float should be larger):" );
	if( !bytesNotBigger.empty() )
	{
		printf( "\n" );
		for( int i = 0; i < (int) bytesNotBigger.size(); ++ i )
		{
			if( i == 5 )
			{
				printf( "  ...and %i other(s) were wrong as well\n", (int) bytesNotBigger.size() - 5 );
				break;
			}
			unsigned char badByte = bytesNotBigger[i];
			unsigned char prevByte = badByte - 1;
			float badFloat = Function_NormalizeByte( badByte );
			float prevFloat = Function_NormalizeByte( prevByte );
			printf( "  byte [%i] -> float [%.09f], but byte [%i] -> float [%.09f]\n", prevByte, prevFloat, badByte, badFloat );
		}
	}

	VerifyTestResult( bytesNotPreserved.empty(), "One or more bytes was not preserved when normalized to float & denormalized back:" );
	if( !bytesNotPreserved.empty() )
	{
		printf( "\n" );
		for( int i = 0; i < (int) bytesNotPreserved.size(); ++ i )
		{
			if( i == 5 )
			{
				printf( "  ...and %i other(s) were wrong as well\n", (int) bytesNotPreserved.size() - 5 );
				break;
			}
			unsigned char badByte = bytesNotPreserved[i];
			float normalized = Function_NormalizeByte( badByte );
			unsigned char badReturn = Function_DenormalizeByte( normalized );
			printf( "  byte [%i] -> float [%.09f] -> byte [%i]\n", badByte, normalized, badReturn );
		}
	}

	// Check for mostly-even distribution of floats into byte-buckets; each bucket should get roughly the same number of floats
	//	mapped to it, given a nice even spread of floats from 0.0 to 1.0.
	constexpr float FLOAT_STEP = (1.f / 8192.f);
	int numFloatsPerByteBucket[256] = {};
	float largestErrorFloat = 0.f;
	float largestError = 0.f;
	for( float f = 0.f; f <= 1.f; f += FLOAT_STEP )
	{
		unsigned char asByte = Function_DenormalizeByte( f );
		++ numFloatsPerByteBucket[ asByte ];
		float backToFloat = Function_NormalizeByte( asByte );
		float error = fabsf( backToFloat - f );
		if( error > largestError )
		{
			largestError = error;
			largestErrorFloat = f;
		}
	}

	int leastBucket = 0;
	int mostBucket = 0;
	int leastInBucket = 999999999;
	int mostInBucket = 0;
	for( int i = 0; i < 256; ++ i )
	{
		if( numFloatsPerByteBucket[i] < leastInBucket )
		{
			leastInBucket = numFloatsPerByteBucket[i];
			leastBucket = i;
		}

		if( numFloatsPerByteBucket[i] > mostInBucket )
		{
			mostInBucket = numFloatsPerByteBucket[i];
			mostBucket = i;
		}
	}

	bool isEvenlyDistributed = mostInBucket - leastInBucket <= 1;
	VerifyTestResult( isEvenlyDistributed, "Floats in [0,1] did not distribute evenly into bytes in [0,255]:" );
	if( !isEvenlyDistributed )
	{
		printf( "\n" );
		printf( "  byte[%i] got only %i out of 8192 floats tested between 0.0 and 1.0;\n", leastBucket, leastInBucket );
		printf( "  whereas byte[%i] got %i out of 8192 floats tested between 0.0 and 1.0.\n", mostBucket, mostInBucket);
	}

//	float oneOverError = 1.f / largestError;
//	unsigned char largestErrorByte = Function_DenormalizeByte( largestErrorFloat );
//	float largestErrorFloatAfter = Function_NormalizeByte( largestErrorByte );
//	printf( "\n  (largest float->byte->float error was (%.09f == 1/%.09f)\n  for %.09ff -> [%i] -> %.09ff)\n", largestError, oneOverError, largestErrorFloat, largestErrorByte, largestErrorFloatAfter );

#endif
	return 8; // Number of tests expected
}


//-----------------------------------------------------------------------------------------------
int TestSet_MP2A1_DotAndCross()
{
#if defined( ENABLE_TestSet_MP2A1_DotAndCross )
	Vector3Class const xAxis( 1.f, 0.f, 0.f );
	Vector3Class const yAxis( 0.f, 1.f, 0.f );
	Vector3Class const zAxis( 0.f, 0.f, 1.f );
	Vector3Class const a( 2.f, 3.f, -4.f );
	Vector3Class const b( 5.f, -6.f, 7.f );
	Vector3Class const c( -8.f, -11.f, 13.f );
	Vector2Class const d( 4.f, 7.f );
	Vector2Class const e( -3.f, 5.f );
	Vector2Class const f( 6.f, -8.f );

	float xDotX = Function_DotProduct3D( xAxis, xAxis );
	float yDotY = Function_DotProduct3D( yAxis, yAxis );
	float zDotZ = Function_DotProduct3D( zAxis, zAxis );
	float xDotY = Function_DotProduct3D( xAxis, yAxis );
	float yDotZ = Function_DotProduct3D( yAxis, zAxis );
	float zDotX = Function_DotProduct3D( zAxis, xAxis );

	VerifyTestResult( IsMostlyEqual( xDotX, 1.f ), "Dot(xAxis,xAxis) should be 1.0 (aligned, normalized)" );
	VerifyTestResult( IsMostlyEqual( yDotY, 1.f ), "Dot(yAxis,yAxis) should be 1.0 (aligned, normalized)" );
	VerifyTestResult( IsMostlyEqual( zDotZ, 1.f ), "Dot(zAxis,zAxis) should be 1.0 (aligned, normalized)" );
	VerifyTestResult( IsMostlyEqual( xDotY, 0.f ), "Dot(xAxis,yAxis) should be 0.0 (perpendicular)" );
	VerifyTestResult( IsMostlyEqual( yDotZ, 0.f ), "Dot(yAxis,zAxis) should be 0.0 (perpendicular)" );
	VerifyTestResult( IsMostlyEqual( zDotX, 0.f ), "Dot(zAxis,xAxis) should be 0.0 (perpendicular)" );

	float ab = Function_DotProduct3D( a, b );
	float ac = Function_DotProduct3D( a, c );
	float bc = Function_DotProduct3D( b, c );
	float ba = Function_DotProduct3D( b, a );
	float ca = Function_DotProduct3D( c, a );
	float cb = Function_DotProduct3D( c, b );

	VerifyTestResult( IsMostlyEqual( ab, -36.f ), "Dot(a,b) should be -36" );
	VerifyTestResult( IsMostlyEqual( ac, -101.f ), "Dot(a,c) should be -101" );
	VerifyTestResult( IsMostlyEqual( bc, 117.f ), "Dot(b,c) should be 117" );
	VerifyTestResult( IsMostlyEqual( ab, ba ), "Dot(a,b) != Dot(b,a)  -  should be commutative!" );
	VerifyTestResult( IsMostlyEqual( ac, ca ), "Dot(a,c) != Dot(c,a)  -  should be commutative!" );
	VerifyTestResult( IsMostlyEqual( bc, cb ), "Dot(b,c) != Dot(c,b)  -  should be commutative!" );

	float DxE = Function_CrossProduct2D( d, e );
	float ExF = Function_CrossProduct2D( e, f );
	float FxD = Function_CrossProduct2D( f, d );
	float ExD = Function_CrossProduct2D( e, d );
	float FxE = Function_CrossProduct2D( f, e );
	float DxF = Function_CrossProduct2D( d, f );

	VerifyTestResult( IsMostlyEqual( DxE, 41.f ), "Cross2D(d,e) should be 41" );
	VerifyTestResult( IsMostlyEqual( ExF, -6.f ), "Cross2D(e,f) should be -6" );
	VerifyTestResult( IsMostlyEqual( FxD, 74.f ), "Cross2D(f,d) should be 74" );
	VerifyTestResult( IsMostlyEqual( ExD, -DxE ), "Cross2D(e,d) should equal -Cross2D(d,e)" );
	VerifyTestResult( IsMostlyEqual( FxE, -ExF ), "Cross2D(f,e) should equal -Cross2D(e,f)" );
	VerifyTestResult( IsMostlyEqual( DxF, -FxD ), "Cross2D(d,f) should equal -Cross2D(f,d)" );

	Vector3Class AxB = Function_CrossProduct3D( a, b );
	Vector3Class BxC = Function_CrossProduct3D( b, c );
	Vector3Class CxA = Function_CrossProduct3D( c, a );
	Vector3Class BxA = Function_CrossProduct3D( b, a );
	Vector3Class CxB = Function_CrossProduct3D( c, b );
	Vector3Class AxC = Function_CrossProduct3D( a, c );

	VerifyTestResult( IsMostlyEqual( AxB, Vector3Class(-3.f, -34.f, -27.f) ), "Cross3D(a,b) should be (-3, -34, -27)" );
	VerifyTestResult( IsMostlyEqual( BxC, Vector3Class(-1.f, -121.f, -103.f) ), "Cross3D(b,c) should be (-1, -121, -103)" );
	VerifyTestResult( IsMostlyEqual( CxA, Vector3Class(5.f, -6.f, -2.f) ), "Cross3D(c,a) should be (5, -6, -2)" );
	VerifyTestResult( IsMostlyEqual( BxA, AxB * -1.f ), "Cross3D(b,a) should equal -Cross2D(a,b)" );
	VerifyTestResult( IsMostlyEqual( CxB, BxC * -1.f ), "Cross3D(c,b) should equal -Cross2D(b,c)" );
	VerifyTestResult( IsMostlyEqual( AxC, CxA * -1.f ), "Cross3D(a,c) should equal -Cross2D(c,a)" );

#endif
	return 24; // Number of tests expected
}


//-----------------------------------------------------------------------------------------------
int TestSet_MP2A1_EulerAngles()
{
#if defined( ENABLE_TestSet_MP2A1_EulerAngles )

	EulerAnglesClass eulerIdentity;
	EulerAnglesClass eulerYaw( 30.f, 0.f, 0.f );
	EulerAnglesClass eulerPitch( 0.f, 30.f, 0.f );
	EulerAnglesClass eulerRoll( 0.f, 0.f, 30.f );
	EulerAnglesClass eulerYawPitch( 30.f, 20.f, 0.f );
	EulerAnglesClass eulerPitchRoll( 0.f, 20.f, 10.f );
	EulerAnglesClass eulerYawPitchRoll( 30.f, 20.f, 10.f );
	EulerAnglesClass eulerYawPitchRollNeg( -15.f, -25.f, -45.f );

	float matrixIdentityValues[16] = { 1.f, 0.f, 0.f, 0.f,   0.f, 1.f, 0.f, 0.f,   0.f, 0.f, 1.f, 0.f,   0.f, 0.f, 0.f, 1.f };
	float yawOnlyMatrixValues[16] = { 0.866025f, 0.5f, 0.f, 0.f,   -0.5f, 0.866025f, 0.f, 0.f,   0.f, 0.f, 1.f, 0.f,   0.f, 0.f, 0.f, 1.f };
	float pitchOnlyMatrixValues[16] = { 0.866025f, 0.f, -0.5f, 0.f,   0.f, 1.f, 0.f, 0.f,   0.5f, 0.f, 0.866025f, 0.f,   0.f, 0.f, 0.f, 1.f };
	float rollOnlyMatrixValues[16] = { 1.f, 0.f, 0.f, 0.f,   0.f, 0.866025f, 0.5f, 0.f,   0.f, -0.5f, 0.866025f, 0.f,   0.f, 0.f, 0.f, 1.f };
	float yawPitchMatrixValues[16] = { 0.813798f, 0.469846f, -0.34202f, 0.f,   -0.5f, 0.866025f, 0.f, 0.f,   0.296198f, 0.17101f, 0.939693f, 0.f,   0.f, 0.f, 0.f, 1.f };
	float pitchRollMatrixValues[16] = { 0.939693f, 0.f, -0.34202f, 0.f,   0.0593912f, 0.984808f, 0.163176f, 0.f,   0.336824f, -0.173648f, 0.925417f, 0.f,   0.f, 0.f, 0.f, 1.f };
	float yprMatrixValues[16] = { 0.813798f, 0.469846f, -0.34202f, 0.f,   -0.44097f, 0.882564f, 0.163176f, 0.f,   0.378522f, 0.0180283f, 0.925417f, 0.f,   0.f, 0.f, 0.f, 1.f };
	float yprNegMatrixValues[16] = { 0.875426f, -0.23457f, 0.422618f, 0.f,   0.471666f, 0.605668f, -0.640856f, 0.f,   -0.105641f, 0.760357f, 0.640856f, 0.f,   0.f, 0.f, 0.f, 1.f };

	Vector3Class iForward, jLeft, kUp; // NOTE: these are used and re-used throughout this function!  Place breakpoints carefully!
	eulerIdentity.EulerAngles_GetAsVectors_XFwd_YLeft_ZUp( iForward, jLeft, kUp );
	Matrix44Class identityAsMatrix = eulerIdentity.EulerAngles_GetAsMatrix_XFwd_YLeft_ZUp();
	VerifyTestResult( IsMostlyEqual( iForward, 1.f, 0.f, 0.f ),			"EulerAngles::GetAsVectors_XFwd_YLeft_ZUp() incorrect - iForward should be (1,0,0)" );
	VerifyTestResult( IsMostlyEqual( jLeft, 0.f, 1.f, 0.f ),			"EulerAngles::GetAsVectors_XFwd_YLeft_ZUp() incorrect - jLeft should be (0,1,0)" );
	VerifyTestResult( IsMostlyEqual( kUp, 0.f, 0.f, 1.f ),				"EulerAngles::GetAsVectors_XFwd_YLeft_ZUp() incorrect - kUp should be (0,0,1)" );
	VerifyTestResult( IsMostlyEqual( identityAsMatrix, matrixIdentityValues ), "EulerAngles::GetAsMatrix_XFwd_YLeft_ZUp() was incorrect for identity" );

	eulerYaw.EulerAngles_GetAsVectors_XFwd_YLeft_ZUp( iForward, jLeft, kUp );
	Matrix44Class asMatrix = eulerYaw.EulerAngles_GetAsMatrix_XFwd_YLeft_ZUp();
	VerifyTestResult( IsMostlyEqual( iForward, 0.866025f, 0.5f, 0.f ),	"EulerAngles::GetAsVectors_XFwd_YLeft_ZUp() - iForward incorrect for yaw=30" );
	VerifyTestResult( IsMostlyEqual( jLeft, -0.5f, 0.866025f, 0.f ),		"EulerAngles::GetAsVectors_XFwd_YLeft_ZUp() - jLeft incorrect for yaw=30" );
	VerifyTestResult( IsMostlyEqual( kUp, 0.f, 0.f, 1.f ),		"EulerAngles::GetAsVectors_XFwd_YLeft_ZUp() - kUp incorrect for yaw=30" );
	VerifyTestResult( IsMostlyEqual( asMatrix, yawOnlyMatrixValues ), "EulerAngles::GetAsMatrix_XFwd_YLeft_ZUp() was incorrect for yaw=30" );

	eulerPitch.EulerAngles_GetAsVectors_XFwd_YLeft_ZUp( iForward, jLeft, kUp );
	asMatrix = eulerPitch.EulerAngles_GetAsMatrix_XFwd_YLeft_ZUp();
	VerifyTestResult( IsMostlyEqual( iForward, 0.866025f, 0.f, -0.5f ),	"EulerAngles::GetAsVectors_XFwd_YLeft_ZUp() - iForward incorrect for pitch=30" );
	VerifyTestResult( IsMostlyEqual( jLeft, 0.f, 1.f, 0.f ),			"EulerAngles::GetAsVectors_XFwd_YLeft_ZUp() - jLeft incorrect for pitch=30" );
	VerifyTestResult( IsMostlyEqual( kUp, 0.5f, 0.f, 0.866025f ),		"EulerAngles::GetAsVectors_XFwd_YLeft_ZUp() - kUp incorrect for pitch=30" );
	VerifyTestResult( IsMostlyEqual( asMatrix, pitchOnlyMatrixValues ), "EulerAngles::GetAsMatrix_XFwd_YLeft_ZUp() was incorrect for pitch=30" );

	eulerRoll.EulerAngles_GetAsVectors_XFwd_YLeft_ZUp( iForward, jLeft, kUp );
	asMatrix = eulerRoll.EulerAngles_GetAsMatrix_XFwd_YLeft_ZUp();
	VerifyTestResult( IsMostlyEqual( iForward, 1.f, 0.f, 0.f ),			"EulerAngles::GetAsVectors_XFwd_YLeft_ZUp() - iForward incorrect for roll=30" );
	VerifyTestResult( IsMostlyEqual( jLeft, 0.f, 0.866025f, 0.5f ),		"EulerAngles::GetAsVectors_XFwd_YLeft_ZUp() - jLeft incorrect for roll=30" );
	VerifyTestResult( IsMostlyEqual( kUp, 0.f, -0.5f, 0.866025f ),		"EulerAngles::GetAsVectors_XFwd_YLeft_ZUp() - kUp incorrect for roll=30" );
	VerifyTestResult( IsMostlyEqual( asMatrix, rollOnlyMatrixValues ), "EulerAngles::GetAsMatrix_XFwd_YLeft_ZUp() was incorrect for roll=30" );

	eulerYawPitch.EulerAngles_GetAsVectors_XFwd_YLeft_ZUp( iForward, jLeft, kUp );
	asMatrix = eulerYawPitch.EulerAngles_GetAsMatrix_XFwd_YLeft_ZUp();
	VerifyTestResult( IsMostlyEqual( iForward, 0.813798f, 0.469846f, -0.34202f ),	"EulerAngles::GetAsVectors_XFwd_YLeft_ZUp() - iForward incorrect for yaw=30, pitch=20" );
	VerifyTestResult( IsMostlyEqual( jLeft, -0.5f, 0.866025f, 0.f ),				"EulerAngles::GetAsVectors_XFwd_YLeft_ZUp() - jLeft incorrect for yaw=30, pitch=20" );
	VerifyTestResult( IsMostlyEqual( kUp, 0.296198f, 0.17101f, 0.939693f ),			"EulerAngles::GetAsVectors_XFwd_YLeft_ZUp() - kUp incorrect for yaw=30, pitch=20" );
	VerifyTestResult( IsMostlyEqual( asMatrix, yawPitchMatrixValues ), "EulerAngles::GetAsMatrix_XFwd_YLeft_ZUp() was incorrect for yaw=30, pitch=20" );

	eulerPitchRoll.EulerAngles_GetAsVectors_XFwd_YLeft_ZUp( iForward, jLeft, kUp );
	asMatrix = eulerPitchRoll.EulerAngles_GetAsMatrix_XFwd_YLeft_ZUp();
	VerifyTestResult( IsMostlyEqual( iForward, 0.939693f, 0.f, -0.34202f ),		"EulerAngles::GetAsVectors_XFwd_YLeft_ZUp() - iForward incorrect for pitch=20, roll=10" );
	VerifyTestResult( IsMostlyEqual( jLeft, 0.0593912f, 0.984808f, 0.163176f ),	"EulerAngles::GetAsVectors_XFwd_YLeft_ZUp() - jLeft incorrect for pitch=20, roll=10" );
	VerifyTestResult( IsMostlyEqual( kUp, 0.336824f, -0.173648f, 0.925417f ),	"EulerAngles::GetAsVectors_XFwd_YLeft_ZUp() - kUp incorrect for pitch=20, roll=10" );
	VerifyTestResult( IsMostlyEqual( asMatrix, pitchRollMatrixValues ), "EulerAngles::GetAsMatrix_XFwd_YLeft_ZUp() was incorrect for pitch=20, roll=10" );

	eulerYawPitchRoll.EulerAngles_GetAsVectors_XFwd_YLeft_ZUp( iForward, jLeft, kUp );
	asMatrix = eulerYawPitchRoll.EulerAngles_GetAsMatrix_XFwd_YLeft_ZUp();
	VerifyTestResult( IsMostlyEqual( iForward, 0.813798f, 0.469846f, -0.34202f ),	"EulerAngles::GetAsVectors_XFwd_YLeft_ZUp() - iForward incorrect for yaw=30, pitch=20, roll=10" );
	VerifyTestResult( IsMostlyEqual( jLeft, -0.44097f, 0.882564f, 0.163176f ),		"EulerAngles::GetAsVectors_XFwd_YLeft_ZUp() - jLeft incorrect for yaw=30, pitch=20, roll=10" );
	VerifyTestResult( IsMostlyEqual( kUp, 0.378522f, 0.0180283f, 0.925417f ),		"EulerAngles::GetAsVectors_XFwd_YLeft_ZUp() - kUp incorrect for yaw=30, pitch=20, roll=10" );
	VerifyTestResult( IsMostlyEqual( asMatrix, yprMatrixValues ), "EulerAngles::GetAsMatrix_XFwd_YLeft_ZUp() was incorrect for yaw=30, pitch=20, roll=10" );

	eulerYawPitchRollNeg.EulerAngles_GetAsVectors_XFwd_YLeft_ZUp( iForward, jLeft, kUp );
	asMatrix = eulerYawPitchRollNeg.EulerAngles_GetAsMatrix_XFwd_YLeft_ZUp();
	VerifyTestResult( IsMostlyEqual( iForward, 0.875426f, -0.23457f, 0.422618f ),	"EulerAngles::GetAsVectors_XFwd_YLeft_ZUp() - iForward incorrect for yaw=-15, pitch=-25, roll=-45" );
	VerifyTestResult( IsMostlyEqual( jLeft, 0.471666f, 0.605668f, -0.640856f ),		"EulerAngles::GetAsVectors_XFwd_YLeft_ZUp() - jLeft incorrect for yaw=-15, pitch=-25, roll=-45" );
	VerifyTestResult( IsMostlyEqual( kUp, -0.105641f, 0.760357f, 0.640856f ),		"EulerAngles::GetAsVectors_XFwd_YLeft_ZUp() - kUp incorrect for yaw=-15, pitch=-25, roll=-45" );
	VerifyTestResult( IsMostlyEqual( asMatrix, yprNegMatrixValues ), "EulerAngles::GetAsMatrix_XFwd_YLeft_ZUp() was incorrect for yaw=-15, pitch=-25, roll=-45" );
//	printf( "\n  ijkt = %gf, %gf, %gf, 0.f,   %gf, %gf, %gf, 0.f,   %gf, %gf, %gf, 0.f,   0.f, 0.f, 0.f, 1.f\n", iForward.x, iForward.y, iForward.z, jLeft.x, jLeft.y, jLeft.z, kUp.x, kUp.y, kUp.z );

#endif
	return 32; // Number of tests expected
}


//-----------------------------------------------------------------------------------------------
void RunTests_MP2A1()
{
	printf( "Running tests for MP2-A1...\n" );
	RunTestSet( true, TestSet_MP2A1_ByteDenormalization,		"MP2-A1: Byte math & [de]normalization" );
	RunTestSet( true, TestSet_MP2A1_DotAndCross,				"MP2-A1: Dot and Cross" );
	RunTestSet( true, TestSet_MP2A1_EulerAngles,				"MP2-A1: EulerAngles" );
}


