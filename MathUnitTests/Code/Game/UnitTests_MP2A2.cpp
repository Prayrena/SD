//-----------------------------------------------------------------------------------------------
// UnitTests_MP2A2.cpp
//
#include "Game/UnitTests_MP2A2.hpp"
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
bool IsMostlyEqual(const Vector2Class& vec2, float x, float y);
bool IsMostlyEqual(const Vector2Class& vec2a, const Vector2Class& vec2b);
bool IsMostlyEqual(int a, int b, int epsilon = 0);
bool IsMostlyEqual(const Vector3Class& vec3, float x, float y, float z);
bool IsMostlyEqual(const Vector3Class& vec3a, const Vector3Class& vec3b);
bool IsMostlyEqual(const AABB2Class& box1, const AABB2Class& box2);
bool IsMostlyEqual(const AABB2Class& box, const Vector2Class& mins, const Vector2Class& maxs);
bool IsMostlyEqual(const AABB2Class& box, float minX, float minY, float maxX, float maxY);
bool IsEqual(const IntVec2Class& a, const IntVec2Class& b);
bool IsEqual(const IntVec2Class& ivec2, int x, int y);
bool IsMostlyEqual(const Vector4Class& vec4, float x, float y, float z, float w);
bool IsMostlyEqual( const Vector4Class& vec4a, const Vector4Class& vec4b );
bool IsMostlyEqual( const float* floatArray1, const float* floatArray2, int numArrayElements );
bool IsMostlyEqual( const Matrix44Class& matrix, const float* sixteenCorrectMatrixValues );
bool IsMostlyEqual( const Matrix44Class& mat44a, const Matrix44Class& mat44b );


//------------------------------------------------------------------------------------------------
// New helper functions introduced here
//
bool IsMostlyEqual( const AABB3Class& box, float minX, float minY, float minZ,  float maxX, float maxY, float maxZ )
{
	return	box.AABB3_Mins.x == minX &&
			box.AABB3_Mins.y == minY &&
			box.AABB3_Mins.z == minZ &&
			box.AABB3_Maxs.x == maxX &&
			box.AABB3_Maxs.y == maxY &&
			box.AABB3_Maxs.z == maxZ;
}


//------------------------------------------------------------------------------------------------
float TestGetLength( float const* vec3 )
{
	float lengthSquared = (vec3[0] * vec3[0]) + (vec3[1] * vec3[1]) + (vec3[2] * vec3[2]);
	return sqrtf( lengthSquared );
}


//------------------------------------------------------------------------------------------------
float TestDot3D( float const* vec3_a, float const* vec3_b )
{
	return (vec3_a[0] * vec3_b[0]) + (vec3_a[1] * vec3_b[1]) + (vec3_a[2] * vec3_b[2]);
}


//------------------------------------------------------------------------------------------------
bool IsMostlyOrthonormal( Matrix44Class const& m )
{
	// Make sure it's safe to poke around in 64 bytes (16 floats) worth of memory
	int matrixSize = sizeof(Matrix44Class);
	if( matrixSize != 64 )
	{
		printf( "\n  ERROR: sizeof(Mat44) != 64 bytes" );
		return false;
	}

	float const* v = reinterpret_cast<float const*>( &m );

	// Check w values first (we should only be orthonormalizing simple affine matrices with w=0,0,0,1)
	if( !IsMostlyEqual( v[3], 0.f ) )
	{
		printf( "\n  ERROR: iBasis had w=%f (must be zero)", v[3] );
		return false;
	}
	if( !IsMostlyEqual( v[7], 0.f ) )
	{
		printf( "\n  ERROR: jBasis had w=%f (must be zero)", v[7] );
		return false;
	}
	if( !IsMostlyEqual( v[11], 0.f ) )
	{
		printf( "\n  ERROR: kBasis had w=%f (must be zero)", v[11] );
		return false;
	}
	if( !IsMostlyEqual( v[15], 1.f ) )
	{
		printf( "\n  ERROR: tBasis had w=%f (must be one)", v[15] );
		return false;
	}

	// Test normality first (are i,j,k bases all normalized in 3D?)
	float iLength = TestGetLength( &v[0] );
	float jLength = TestGetLength( &v[4] );
	float kLength = TestGetLength( &v[8] );
	if( !IsMostlyEqual( iLength, 1.f ) )
	{
		printf( "\n  ERROR: iBasis had length=%f (must be one)", iLength );
		return false;
	}
	if( !IsMostlyEqual( jLength, 1.f ) )
	{
		printf( "\n  ERROR: jBasis had length=%f (must be one)", jLength );
		return false;
	}
	if( !IsMostlyEqual( kLength, 1.f ) )
	{
		printf( "\n  ERROR: kBasis had length=%f (must be one)", kLength );
		return false;
	}

	// Test orthogonality next - are dot(i,j) and dot(j,k) and dot(k,i) all zero?
	float dotIJ = TestDot3D( &v[0], &v[4] );
	float dotJK = TestDot3D( &v[4], &v[8] );
	float dotKI = TestDot3D( &v[8], &v[0] );
	if( !IsMostlyEqual( dotIJ, 0.f ) )
	{
		printf( "\n  ERROR: iBasis and jBasis were not perpendicular" );
		return false;
	}
	if( !IsMostlyEqual( dotJK, 0.f ) )
	{
		printf( "\n  ERROR: jBasis and kBasis were not perpendicular" );
		return false;
	}
	if( !IsMostlyEqual( dotKI, 0.f ) )
	{
		printf( "\n  ERROR: kBasis and iBasis were not perpendicular" );
		return false;
	}

	return true;
}


//////////////////////////////////////////////////////////////////////////////////////////////////
// Tests follow
//////////////////////////////////////////////////////////////////////////////////////////////////


//-----------------------------------------------------------------------------------------------
int TestSet_MP2A2_AABB3_Basics()
{
#if defined( ENABLE_TestSet_MP2A2_AABB3_Basics )

	AABB3Class box1; // default construct
	AABB3Class box2( 1.2f, 3.4f, 5.6f,   7.8f, 9.1f, 2.3f );	// explicit construct from minX,minY,minZ, maxX,maxY,maxZ
	AABB3Class box3( Vector3Class( 2.3f, 4.5f, 6.7f ), Vector3Class( 8.9f, 1.3f, 2.4f ) ); // explicit construct from mins,maxs
	AABB3Class box4( box2 ); // copy construct
	AABB3Class box5;
	box5 = box3;
	const AABB3Class box6( box5 ); // construct const (read-only) object
	const AABB3Class box7( box6 ); // copy-construct should not try to modify (read-only) source object

	VerifyTestResult( sizeof( AABB3Class ) == 24, "sizeof(AABB3) was not 24 bytes" );
	VerifyTestResult( IsMostlyEqual( box2, 1.2f, 3.4f, 5.6f,   7.8f, 9.1f, 2.3f ), "AABB3 explicit constructor from (minX,minY,minZ, maxX,maxY,maxZ ) failed" );
	VerifyTestResult( IsMostlyEqual( box3, 2.3f, 4.5f, 6.7f,   8.9f, 1.3f, 2.4f ), "AABB3 explicit constructor from (mins, maxs) failed" );
	VerifyTestResult( IsMostlyEqual( box4, 1.2f, 3.4f, 5.6f,   7.8f, 9.1f, 2.3f ), "AABB3 copy constructor failed" );
	VerifyTestResult( IsMostlyEqual( box5, 2.3f, 4.5f, 6.7f,   8.9f, 1.3f, 2.4f ), "AABB3 assignment operator= failed" );

#endif
	return 5; // Number of tests expected
}


//-----------------------------------------------------------------------------------------------
int TestSet_MP2A2_DotProduct4D()
{
#if defined( ENABLE_TestSet_MP2A2_DotProduct4D )

	Vector4Class a( 1.2f, 2.3f, 3.4f, 4.5f );
	Vector4Class b( 5.6f, 6.7f, 7.8f, 8.9f );
	Vector4Class c( 3.f, 4.f, 5.f, 7.f );

	float dotAB = Function_DotProduct4D( a, b );
	float dotBC = Function_DotProduct4D( b, c );
	float dotCA = Function_DotProduct4D( c, a );
	VerifyTestResult( IsMostlyEqual( dotAB, 88.7f ), "DotProduct4D() failed for (1.2, 2.3, 3.4, 4.5) dot (5.6, 6.7, 7.8, 8.9)" );
	VerifyTestResult( IsMostlyEqual( dotBC, 144.9f ), "DotProduct4D() failed for (5.6, 6.7, 7.8, 8.9) dot (3, 4, 5, 7)" );
	VerifyTestResult( IsMostlyEqual( dotCA, 61.3f ), "DotProduct4D() failed for (3, 4, 5, 7) dot (1.2, 2.3, 3.4, 4.5)" );

#endif
	return 3; // Number of tests expected
}


//-----------------------------------------------------------------------------------------------
int TestSet_MP2A2_OrthoProjection()
{
#if defined( ENABLE_TestSet_MP2A2_OrthoProjection )

	Matrix44Class orthoIdentity		= Matrix44Class::Mat44_CreateOrthoProjection( -1.f, 1.f, -1.f, 1.f, 0.f, 1.f );
	Matrix44Class orthoBottomLeft	= Matrix44Class::Mat44_CreateOrthoProjection( 0.f, 1600.f, 0.f, 900.f, 0.f, 10.f );
	Matrix44Class orthoRoaming		= Matrix44Class::Mat44_CreateOrthoProjection( 10.f, 30.f, 40.f, 50.f, 60.f, 90.f );

	float const orthoIdentityValues[16]		= { 1.f, 0.f, 0.f, 0.f,  0.f, 1.f, 0.f, 0.f,  0.f, 0.f, 1.f, 0.f,  0.f, 0.f, 0.f, 1.f };
	float const orthoBottomLeftValues[16]	= { 0.00125f, 0.f, 0.f, 0.f,  0.f, 0.00222222228f, 0.f, 0.f,  0.f, 0.f, 0.1f, 0.f,  -1.0f, -1.0f, 0.f, 1.f };
	float const orthoRoamingValues[16]		= { 0.1f, 0.f, 0.f, 0.f,  0.f, 0.2f, 0.f, 0.f,  0.f, 0.f, 0.033333333f, 0.f,  -2.0f, -9.0f, -2.0f, 1.f };

	VerifyTestResult( IsMostlyEqual( orthoIdentity,		orthoIdentityValues ),		"Mat44::CreateOrtho() was incorrect for left=-1, right=1, bottom=-1, top=1, near=0, far=1; should be Identity!" );
	VerifyTestResult( IsMostlyEqual( orthoBottomLeft,	orthoBottomLeftValues ),	"Mat44::CreateOrtho() was incorrect for left=0, right=1600, bottom=0, top=900, near=0, far=10" );
	VerifyTestResult( IsMostlyEqual( orthoRoaming,		orthoRoamingValues ),		"Mat44::CreateOrtho() was incorrect for left=10, right=30, bottom=40, top=50, near=60, far=90" );

#endif
	return 3; // Number of tests expected
}


//-----------------------------------------------------------------------------------------------
int TestSet_MP2A2_PerspectiveProjection()
{
#if defined( ENABLE_TestSet_MP2A2_PerspectiveProjection )

	Matrix44Class perspectiveVanilla	= Matrix44Class::Mat44_CreatePerspectiveProjection( 90.f, 1.f, 0.f, 1.f );
	Matrix44Class perspectiveAndDepth	= Matrix44Class::Mat44_CreatePerspectiveProjection( 90.f, 1.f, 3.f, 5.f );
	Matrix44Class perspectiveFishEye	= Matrix44Class::Mat44_CreatePerspectiveProjection( 120.f, 1.f, 1.f, 2.f );
	Matrix44Class perspectiveZoomWide	= Matrix44Class::Mat44_CreatePerspectiveProjection( 75.f, 3.f, 0.1f, 100.f );

	float const perspectiveVanillaValues[16]	= { 1.f, 0.f, 0.f, 0.f,  0.f, 1.f, 0.f, 0.f,  0.f, 0.f, 1.f, 1.f,  0.f, 0.f, 0.f, 0.f };
	float const perspectiveAndDepthValues[16]	= { 1.f, 0.f, 0.f, 0.f,  0.f, 1.f, 0.f, 0.f,  0.f, 0.f, 2.5f, 1.f,  0.f, 0.f, -7.5f, 0.f };
	float const perspectiveFishEyeValues[16]	= { 0.577350199f, 0.f, 0.f, 0.f,  0.f, 0.577350199f, 0.f, 0.f,  0.f, 0.f, 2.f, 1.f,  0.f, 0.f, -2.f, 0.f };
	float const perspectiveZoomWideValues[16]	= { 0.434408426f, 0.f, 0.f, 0.f,  0.f, 1.30322528f, 0.f, 0.f,  0.f, 0.f, 1.00100100f, 1.f,  0.f, 0.f, -0.100100100f, 0.f };

	VerifyTestResult( IsMostlyEqual( perspectiveVanilla,	perspectiveVanillaValues ),		"Mat44::CreatePerspective() was incorrect for fovY=90, aspect=1, zNear=0, zFar=1" );
	VerifyTestResult( IsMostlyEqual( perspectiveAndDepth,	perspectiveAndDepthValues ),	"Mat44::CreatePerspective() was incorrect for fovY=90, aspect=1, zNear=3, zFar=5" );
	VerifyTestResult( IsMostlyEqual( perspectiveFishEye,	perspectiveFishEyeValues ),		"Mat44::CreatePerspective() was incorrect for fovY=120, aspect=1, zNear=1, zFar=2" );
	VerifyTestResult( IsMostlyEqual( perspectiveZoomWide,	perspectiveZoomWideValues ),	"Mat44::CreatePerspective() was incorrect for fovY=75, aspect=3, zNear=0.1, zFar=100" );

#endif
	return 4; // Number of tests expected
}


//-----------------------------------------------------------------------------------------------
int TestSet_MP2A2_TransposeInvert()
{
#if defined( ENABLE_TestSet_MP2A2_TransposeInvert )
	float const test1Values[16]			= { 0.f, 1.f, 2.f, 3.f,  4.f, 5.f, 6.f, 7.f,  8.f, 9.f, 10.f, 11.f,  12.f, 13.f, 14.f, 15.f };
	float const transposed1Values[16]	= { 0.f, 4.f, 8.f, 12.f,  1.f, 5.f, 9.f, 13.f,  2.f, 6.f, 10.f, 14.f,  3.f, 7.f, 11.f, 15.f };
	Matrix44Class test1( test1Values );
	Matrix44Class transposed( test1 );
	transposed.Mat44_Transpose();
	Matrix44Class transposedTwice( transposed );
	transposedTwice.Mat44_Transpose();

	VerifyTestResult( IsMostlyEqual( test1, test1Values ),				"Mat44 did not construct from float array correctly!" );
	VerifyTestResult( IsMostlyEqual( transposed, transposed1Values ),	"Mat44::Transpose was incorrect!" );
	VerifyTestResult( IsMostlyEqual( transposedTwice, test1Values ),	"Mat44::Transpose should go back to original matrix when Transposed twice!" );

	Matrix44Class identity;

	Matrix44Class yawPitchRoll;
	yawPitchRoll.Mat44_RotateZDegrees( 30.f );
	yawPitchRoll.Mat44_RotateYDegrees( 20.f );
	yawPitchRoll.Mat44_RotateXDegrees( 10.f );

	Matrix44Class antiYawPitchRoll; // manually construct the inverse rotation (opposite spins in opposite order)
	antiYawPitchRoll.Mat44_RotateXDegrees( -10.f );
	antiYawPitchRoll.Mat44_RotateYDegrees( -20.f );
	antiYawPitchRoll.Mat44_RotateZDegrees( -30.f );

	Matrix44Class affine;
	affine.Mat44_Translate3D( Vector3Class( 3.f, 4.f, 5.f ) );
	affine.Mat44_RotateZDegrees( 55.f );
	affine.Mat44_RotateYDegrees( 44.f );
	affine.Mat44_RotateXDegrees( 33.f );

	Matrix44Class affineInverse;
	affineInverse.Mat44_RotateXDegrees( -33.f );
	affineInverse.Mat44_RotateYDegrees( -44.f );
	affineInverse.Mat44_RotateZDegrees( -55.f );
	affineInverse.Mat44_Translate3D( Vector3Class( -3.f, -4.f, -5.f ) );

	VerifyTestResult( IsMostlyEqual( identity, identity.Mat44_GetOrthonormalInverse() ),				"Mat44::GetOrthoAffineInverse was incorrect for Identity" );
	VerifyTestResult( IsMostlyEqual( antiYawPitchRoll, yawPitchRoll.Mat44_GetOrthonormalInverse() ),	"Mat44::GetOrthoAffineInverse was incorrect for Rotation-only" );
	VerifyTestResult( IsMostlyEqual( affineInverse, affine.Mat44_GetOrthonormalInverse() ),				"Mat44::GetOrthoAffineInverse was incorrect for Rotation-with-Translation" );

#endif
	return 6; // Number of tests expected
}


//-----------------------------------------------------------------------------------------------
int TestSet_MP2A2_OrthoNormalize()
{
#if defined( ENABLE_TestSet_MP2A2_OrthoNormalize )

	float const identityValues[16]			= { 1.f, 0.f, 0.f, 0.f,  0.f, 1.f, 0.f, 0.f,  0.f, 0.f, 1.f, 0.f,  0.f, 0.f, 0.f, 1.f };
	float const translationValues[16]		= { 1.f, 0.f, 0.f, 0.f,  0.f, 1.f, 0.f, 0.f,  0.f, 0.f, 1.f, 0.f,  3.f, 4.f, 5.f, 1.f };
	float const uniformScaledValues[16]		= { 4.f, 0.f, 0.f, 0.f,  0.f, 4.f, 0.f, 0.f,  0.f, 0.f, 4.f, 0.f,  0.f, 0.f, 0.f, 1.f };
	float const nonUniformScaledValues[16]	= { 3.f, 0.f, 0.f, 0.f,  0.f, 4.f, 0.f, 0.f,  0.f, 0.f, 5.f, 0.f,  0.f, 0.f, 0.f, 1.f };
	float const badJBasisValues[16]			= { 1.f, 0.f, 0.f, 0.f,  1.f, 2.f, 3.f, 0.f,  0.f, 0.f, 1.f, 0.f,  0.f, 0.f, 0.f, 1.f };
	float const badKBasisValues[16]			= { 1.f, 0.f, 0.f, 0.f,  0.f, 1.f, 0.f, 0.f,  2.f, 3.f, 4.f, 0.f,  0.f, 0.f, 0.f, 1.f };
	float const badJKBasisValues[16]		= { 1.f, 0.f, 0.f, 0.f,  4.f, 5.f, 6.f, 0.f,  7.f, 8.f, 9.f, 0.f,  0.f, 0.f, 0.f, 1.f };
	float const crazyTownValues[16]			= { 2.f, 3.f, 4.f, 0.f,  5.f, 7.f, 6.f, 0.f,  9.f, 3.f, 8.f, 0.f,  10.f, 11.f, 12.f, 1.f };

	Matrix44Class identity( identityValues );
	Matrix44Class translation( translationValues );
	Matrix44Class uniformScaled( uniformScaledValues );
	Matrix44Class nonUniformScaled( nonUniformScaledValues );
	Matrix44Class badJBasis( badJBasisValues );
	Matrix44Class badKBasis( badKBasisValues );
	Matrix44Class badJKBasis( badJKBasisValues );
	Matrix44Class crazyTown( crazyTownValues );

	identity.Mat44_Orthonormalize_IFwd_JLeft_KUp();
	translation.Mat44_Orthonormalize_IFwd_JLeft_KUp();
	uniformScaled.Mat44_Orthonormalize_IFwd_JLeft_KUp();
	nonUniformScaled.Mat44_Orthonormalize_IFwd_JLeft_KUp();
	badJBasis.Mat44_Orthonormalize_IFwd_JLeft_KUp();
	badKBasis.Mat44_Orthonormalize_IFwd_JLeft_KUp();
	badJKBasis.Mat44_Orthonormalize_IFwd_JLeft_KUp();
	crazyTown.Mat44_Orthonormalize_IFwd_JLeft_KUp();

	VerifyTestResult( IsMostlyOrthonormal( identity ),			"Mat44::Orthonormalize failed for Identity" );
	VerifyTestResult( IsMostlyOrthonormal( translation ),		"Mat44::Orthonormalize failed for Translation" );
	VerifyTestResult( IsMostlyOrthonormal( uniformScaled ),		"Mat44::Orthonormalize failed for UniformScaled" );
	VerifyTestResult( IsMostlyOrthonormal( nonUniformScaled ),	"Mat44::Orthonormalize failed for NonUniformScaled" );
	VerifyTestResult( IsMostlyOrthonormal( badJBasis ),			"Mat44::Orthonormalize failed for Bad J Basis" );
	VerifyTestResult( IsMostlyOrthonormal( badKBasis ),			"Mat44::Orthonormalize failed for Bad K Basis" );
	VerifyTestResult( IsMostlyOrthonormal( badJKBasis ),		"Mat44::Orthonormalize failed for Bad J&K Bases" );
	VerifyTestResult( IsMostlyOrthonormal( crazyTown ),			"Mat44::Orthonormalize failed for arbirary crazy matrix" );

	float const* translationFixedValues = reinterpret_cast<float const*>( &translation );
	float const* crazyTownFixedValues = reinterpret_cast<float const*>( &crazyTown );
	VerifyTestResult( IsMostlyEqual( &translationFixedValues[12], &translationValues[12], 3 ), "Mat44::Orthonormalize should not modify Translation values" );
	VerifyTestResult( IsMostlyEqual( &crazyTownFixedValues[12], &crazyTownValues[12], 3 ), "Mat44::Orthonormalize should not modify Translation values" );

#endif
	return 10; // Number of tests expected
}



//-----------------------------------------------------------------------------------------------
void RunTests_MP2A2()
{
	printf( "Running tests for MP2-A2...\n" );
	RunTestSet( true, TestSet_MP2A2_AABB3_Basics,			"MP2-A2: AABB3 basics" );
	RunTestSet( true, TestSet_MP2A2_DotProduct4D,			"MP2-A2: DotProduct4D" );
	RunTestSet( true, TestSet_MP2A2_OrthoProjection,		"MP2-A2: Orthographic Projection" );
	RunTestSet( true, TestSet_MP2A2_PerspectiveProjection,	"MP2-A2: Perspective Projection" );
	RunTestSet( true, TestSet_MP2A2_TransposeInvert,		"MP2-A2: Transpose and Invert" );
	RunTestSet( true, TestSet_MP2A2_OrthoNormalize,			"MP2-A2: Orthonormalization" );
}


