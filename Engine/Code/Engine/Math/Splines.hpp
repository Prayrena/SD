#pragma once
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include <vector>

// Bezier curve functions
// where A,B,C,D are the Cubic (3rd order) Bezier control points (A is the start pos, and D is the end pos)
// t is the parametric in [0,1]
float ComputeCubicBezier1D(float A, float B, float C, float D, float t);
Vec2  ComputeCubicBezier2D(Vec2 A, Vec2 B, Vec2 C, Vec2 D, float t);
Vec3  ComputeCubicBezier3D(Vec3 A, Vec3 B, Vec3 C, Vec3 D, float t);
// where A,B,C,D,E,F are the Quintic (5th order) Bezier control points (A is the start, and F is the end)
// t is the parametric in [0,1].
float ComputeQuinticBezier1D(float A, float B, float C, float D, float E, float F, float t);

class CubicHermiteCurve2D;
class CubicHermiteCurve3D;

class CubicBezierCurve2D
{
public:
	CubicBezierCurve2D() {}
	~CubicBezierCurve2D() = default;
	explicit CubicBezierCurve2D(Vec2 startPos, Vec2 guidePos1, Vec2 guidePos2, Vec2 endPos);
	explicit CubicBezierCurve2D(CubicHermiteCurve2D const& fromHermite);
	Vec2 EvaluateAtParametric(float parametricZeroToOne) const;
	float GetApproximateLength(int numSubdivisions = 64) const;
	Vec2 EvaluateAtApproximateDistance(float distanceAlongCurve, int numSubdivisions = 64) const;
public:
	Vec2 m_a; // start position
	Vec2 m_b; // first guide point
	Vec2 m_c; // second guide point
	Vec2 m_d; // end position
};

class CubicBezierCurve3D
{
public:
	CubicBezierCurve3D() {}
	~CubicBezierCurve3D() = default;
	explicit CubicBezierCurve3D(Vec3 startPos, Vec3 guidePos1, Vec3 guidePos2, Vec3 endPos);
	explicit CubicBezierCurve3D(CubicHermiteCurve3D const& fromHermite);
	Vec3 EvaluateAtParametric(float parametricZeroToOne) const;
	float GetApproximateLength(int numSubdivisions = 64) const;
	Vec3 EvaluateAtApproximateDistance(float distanceAlongCurve, int numSubdivisions = 64) const;
public:
	Vec3 m_a; // start position
	Vec3 m_b; // first guide point
	Vec3 m_c; // second guide point
	Vec3 m_d; // end position
};

class SamplePointsCurve3D
{
public:
	SamplePointsCurve3D(std::vector<Vec3>& samplePoints);
	SamplePointsCurve3D() = default;
	~SamplePointsCurve3D() = default;

	std::vector<Vec3> m_samplePoints;
};

//----------------------------------------------------------------------------------------------------------------------------------------------------
class CubicHermiteCurve2D
{
public:
	CubicHermiteCurve2D() {}
	~CubicHermiteCurve2D() = default;
	explicit CubicHermiteCurve2D(Vec2 startPos, Vec2 startVel, Vec2 endPos, Vec2 endVel);
	explicit CubicHermiteCurve2D(CubicBezierCurve2D const& fromBezier); // convert Bezier curve to hermite curve
	Vec2 EvaluateAtParametric(float parametricZeroToOne) const;
	float GetApproximateLength(int numSubdivisions = 64) const;
	Vec2 EvaluateAtApproximateDistance(float distanceAlongCurve, int numSubdividions = 64);

public:
	Vec2 m_startPos; // start position
	Vec2 m_startVel; // first guide point
	Vec2 m_endPos; // end position
	Vec2 m_endVel; // second guide point
};

class CubicHermiteCurve3D
{
public:
	CubicHermiteCurve3D() {}
	~CubicHermiteCurve3D() = default;
	explicit CubicHermiteCurve3D(Vec3 startPos, Vec3 startVel, Vec3 endPos, Vec3 endVel);
	explicit CubicHermiteCurve3D(CubicBezierCurve3D const& fromBezier); // convert Bezier curve to hermite curve
	Vec3 EvaluateAtParametric(float parametricZeroToOne) const;
	float GetApproximateLength(int numSubdivisions = 64) const;
	Vec3 EvaluateAtApproximateDistance(float distanceAlongCurve, int numSubdividions = 64);

public:
	Vec3 m_startPos; // start position
	Vec3 m_startVel; // first guide point
	Vec3 m_endPos; // end position
	Vec3 m_endVel; // second guide point
};

//----------------------------------------------------------------------------------------------------------------------------------------------------
// Catmull-Rom: an algorithm for choosing velocities in a cubic hermite spline
class CatmullRomSpline2D
{
public:
	CatmullRomSpline2D() = default;
	~CatmullRomSpline2D() = default;
	CatmullRomSpline2D(std::vector<Vec2> const& positions, Vec2 startVel = Vec2::ZERO, Vec2 endVel = Vec2::ZERO);
	void SetPositions(std::vector<Vec2> const& positions, Vec2 startVel = Vec2::ZERO, Vec2 endVel = Vec2::ZERO);
	Vec2 EvaluateAtParametric(float parametricZeroToNumCurveSections) const;
	float GetApproximateLength(int numSubdivisions = 64) const;
	Vec2 EvaluateAtApproximateDistance(float distanceAlongCurve, int numSubdivisions = 64) const;

public:
	std::vector<Vec2> m_positions; // welded curve section endpoints
	std::vector<Vec2> m_velocities; // welded velocities at each endpoint
};

class CatmullRomSpline3D
{
public:
	CatmullRomSpline3D() = default;
	~CatmullRomSpline3D() = default;
	CatmullRomSpline3D(std::vector<Vec3> const& positions, Vec3 startVel = Vec3::ZERO, Vec3 endVel = Vec3::ZERO);
	void SetPositions(std::vector<Vec3> const& positions, Vec3 startVel = Vec3::ZERO, Vec3 endVel = Vec3::ZERO);
	Vec3 EvaluateAtParametric(float parametricZeroToNumCurveSections) const;
	float GetApproximateLength(int numSubdivisions = 64) const;
	Vec3 EvaluateAtApproximateDistance(float distanceAlongCurve, int numSubdivisions = 64) const;
	Vec3 GetTangentDirectionAtApproximateDistance(float distanceAlongCurve, float step = 0.001f, int numSubdivisions = 64) const;

public:
	std::vector<Vec3> m_positions; // welded curve section endpoints
	std::vector<Vec3> m_velocities; // welded velocities at each endpoint
};