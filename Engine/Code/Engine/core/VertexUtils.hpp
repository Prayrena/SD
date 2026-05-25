#pragma once
#include "Engine/core/Vertex_PCU.hpp"
#include "Engine/core/Vertex_PCUTBN.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Math/OBB3.hpp"
#include "Engine/Math/OBB2.hpp"
#include "Engine/Math/Plane3.hpp"
#include "Engine/Math/Capsule2.hpp"
#include "Engine/Math/LineSegment2.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Math/Splines.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/Convex2.hpp"
#include <vector>
#include <string>

// translate, rotate and scale local vertex to world vertex
void TransformVertexArrayXY3D(int numberVerts, Vertex_PCU* verts, float scaleXY, float rotationDegreesAboutZ = 0.f, Vec2 const& translationXY = Vec2());
void TransformVertexArrayXY3D(int numberVerts, Vertex_PCUTBN* verts, float rotationDegreesAboutZ = 0.f, Vec2 const& translationXY = Vec2(), float scaleXY = 1.f);
void TransformVertexArrayOnXY(int startIndex, int numVerts, Vertex_PCU* verts, float scaleXY, Vec2 const& rotationPos, float rotationDegreesAboutPos, Vec2 const& translationXY);
void TransformPointOnXY2D(Vec2& pos, float scaleXY, Vec2 const& rotationPos, float rotationDegreesAboutPos, Vec2 const& translationXY);
void TransformVertexArrayXY3D(std::vector<Vertex_PCU>& Verts, Vec2 const& iBasis, Vec2 const& translation);
void TransformVertexArray3D(std::vector<Vertex_PCU>& verts, Mat44 const& transform);

// push a shape information into a vertex_PCU list
void AddVertsForCapsule2D(std::vector<Vertex_PCU>& verts, Capsule2 const& capsule, Rgba8 const& color, int sides = 24);
void AddVertsForCapsule2D(std::vector<Vertex_PCU>& verts, Vec2 const& boneStart, Vec2 const& boneEnd, float radius, Rgba8 const& color);
void AddVertsForAABB2D(std::vector<Vertex_PCU>& verts, AABB2 const& bounds, Rgba8 const& color);
void AddVertsForAABB2DFrame(std::vector<Vertex_PCU>& verts, AABB2 const& bounds, float thickness, Rgba8 const& color);
void AddVertsForOBB2D(std::vector<Vertex_PCU>& verts, OBB2 const& box, Rgba8 const& color);
void AddVertsForTriangle2D(std::vector<Vertex_PCU>& verts, Vec2 startPos, Rgba8 const& StartColor, Vec2 endPos, Rgba8 const& EndColor, float thicknessOfBottom);
void AddVertesForDisc2D(std::vector<Vertex_PCU>& verts, Vec2 const& center, float radius, Rgba8 const& color, int numOfSides = 12);
void AddVertesForRing2D(std::vector<Vertex_PCU>& verts, Vec2 const& center, float radius, float thickness, Rgba8 const& color, int numOfSides = 12);
void AddVertesForHalfDisc2D(std::vector<Vertex_PCU>& verts, Vec2 const& center, float radius, float startDrawingDegrees, Rgba8 const& color, int numOfSides);
void AddVertesForSector2D(std::vector<Vertex_PCU>& verts, Vec2 const& sectorTip, float sectorForwardDegrees, float sectorApertureDegrees, float sectorRadius, Rgba8 color, int numOfSides);
void AddVertesForSector2D(std::vector<Vertex_PCU>& verts, Vec2 const& sectorTip, Vec2 const& sectorForwardNormal, float sectorApertureDegrees, float sectorRadius, Rgba8 color, int numOfSides);
void AddVertesForZSector3D(std::vector<Vertex_PCU>& verts, Vec3 const& sectorTip, Vec3 const& sectorForwardNormal, float sectorApertureDegrees, float sectorRadius, Rgba8 color, int numOfSides);
void AddVertsForLineSegment2D(std::vector<Vertex_PCU>& verts, Vec2 const& start, Vec2 const& end, float thickness, Rgba8 const& color);
void AddVertsForLineSegment2D(std::vector<Vertex_PCU>& verts, LineSegment2 const& ls, float thickness, Rgba8 const& color);
void AddVertsForArrow2D(std::vector<Vertex_PCU>& verts, Vec2 tailPos, Vec2 tipPos, float arrowSize, float lineThickness, Rgba8 const& color);// the arrowSize means the radius of the circle that the end point and two wing points make
void AddVertsForConvexPoly2(std::vector<Vertex_PCU>& verts, ConvexPoly2 const& convex, Rgba8 const& color = Rgba8::WHITE);

// adding verts and set up UV for texture
void AddVertsUVForAABB2D(std::vector<Vertex_PCU>& verts, AABB2 const& spriteBounds, Rgba8 const& spriteTintColor, AABB2 uvBounds=AABB2::ZERO_TO_ONE);

// IK debugging:
void AddVertesForSector3D(std::vector<Vertex_PCU>& verts, Vec3 const& sectorTip, Vec3 const& IBasis, Vec3 const& JBasis, FloatRange rollDegreesRange, float sectorRadius = 1.f, Rgba8 color = Rgba8::GREEN_TRANSPARENT, int numOfSides = 64);
void AddDebugVertsForJointConstraint(std::vector<Vertex_PCU>& verts, Vec3 const& jointPos, Vec3 const& IBasis, Vec3 const& JBasis, Vec3 const& KBasis, 
											EulerAngles minRotation, EulerAngles maxRotation, 
											Rgba8 wedgeColor = Rgba8::RED_TRANSPARENT, Rgba8 sectorColor = Rgba8::GREEN_TRANSPARENT, float wedgeRadius = 0.15f, float sectorRadius = 0.2f, int numOfSides = 64);

//----------------------------------------------------------------------------------------------------------------------------------------------------
// 3D shapes
// quad
void AddVertsForQuad3D(std::vector<Vertex_PCU>& verts,
	Vec3 const& bottomLeft, Vec3 const& bottomRight, Vec3 const& topRight, Vec3 const& topLeft,
	Rgba8 const& color = Rgba8::WHITE, AABB2 const& UVs = AABB2::ZERO_TO_ONE);
void AddVertsForQuad3DFrame(std::vector<Vertex_PCU>& verts,
	Vec3 const& bottomLeft, Vec3 const& bottomRight, Vec3 const& topRight, Vec3 const& topLeft,
	float offsetOutwards = 0.1f, float offsetInwards = 0.1f, 
	Rgba8 const& color = Rgba8::WHITE, AABB2 const& UVs = AABB2::ZERO_TO_ONE);
void AddVertsForQuad3D(std::vector<Vertex_PCUTBN>& verts, std::vector<unsigned int>& indexes,
	Vec3 const& bottomLeft, Vec3 const& bottomRight, Vec3 const& topRight, Vec3 const& topLeft,
	Rgba8 const& color = Rgba8::WHITE, AABB2 const& UVs = AABB2::ZERO_TO_ONE);
void AddVertsForRoundedQuad3D(std::vector<Vertex_PCUTBN>& vertexes, Vec3 const& topLeft, Vec3 const& bottomLeft, 
	Vec3 const& bottomRight, Vec3 const& topRight, Rgba8 const& color = Rgba8::WHITE, AABB2 const& UVs = AABB2::ZERO_TO_ONE); // For doomenstein monster

// AABB3
void AddVertsForAABB3D(std::vector<Vertex_PCU>& verts, AABB3 const& bounds, Rgba8 const& xFacecolor = Rgba8::WHITE, Rgba8 const& xOppFacecolor = Rgba8::WHITE,
	Rgba8 const& yFacecolor = Rgba8::WHITE, Rgba8 const& yOppFacecolor = Rgba8::WHITE,
	Rgba8 const& zFacecolor = Rgba8::WHITE, Rgba8 const& zOppFacecolor = Rgba8::WHITE,
	AABB2 const& UVs = AABB2::ZERO_TO_ONE);
void AddVertsForAABB3D(std::vector<Vertex_PCU>& verts, AABB3 const& bounds, Rgba8 const& facecolor = Rgba8::WHITE, AABB2 const& UVs = AABB2::ZERO_TO_ONE);
void AddVertsForAABB3D(std::vector<Vertex_PCUTBN>& verts, std::vector<unsigned int>& indexes, 
	AABB3 const& bounds, Rgba8 const& facecolor = Rgba8::WHITE, AABB2 const& UVs = AABB2::ZERO_TO_ONE);

void AddVertsForAABB3Frame(std::vector<Vertex_PCU>& verts, AABB3 const& bounds, float framePipeRadius = 0.05f,
	Rgba8 const& facecolor = Rgba8::WHITE, AABB2 const& UVs = AABB2::ZERO_TO_ONE);

// OBB3
void AddVertsForOBB3(std::vector<Vertex_PCU>& verts, OBB3 const& bounds,
	Rgba8 const& xFacecolor = Rgba8::WHITE, Rgba8 const& xOppFacecolor = Rgba8::WHITE, Rgba8 const& yFacecolor = Rgba8::WHITE, 
	Rgba8 const& yOppFacecolor = Rgba8::WHITE, Rgba8 const& zFacecolor = Rgba8::WHITE, Rgba8 const& zOppFacecolor = Rgba8::WHITE,
	AABB2 const& UVs = AABB2::ZERO_TO_ONE);

// Sphere
void AddVertsForSphere3D(std::vector<Vertex_PCU>& verts, Vec3 const& center, float radius, Rgba8 const& color = Rgba8::WHITE, AABB2 const& UVs = AABB2::ZERO_TO_ONE,
	int numSlices = 32, int numStacks = 16); 
	
void AddVertsForSphere3D(std::vector<Vertex_PCUTBN>& verts, std::vector<unsigned int>& indexes,  Vec3 const& center, float radius, 
		Rgba8 const& color = Rgba8::WHITE, AABB2 const& UVs = AABB2::ZERO_TO_ONE,
		int numSlices = 16, int numStacks = 8);

AABB2 GetVertexBounds2D(std::vector<Vertex_PCU> const& verts);

void AddVertsForCylinder3D(std::vector<Vertex_PCU>& verts,
	Vec3 const& start, Vec3 const& end, float radius,
	Rgba8 const& color = Rgba8::WHITE,
	AABB2 const& UVs = AABB2::ZERO_TO_ONE, int numSlices = 12);

void AddVertsForPhysXCapsuleDebug(std::vector<Vertex_PCU>& verts, Vec3 const& start, Vec3 const& end, 
	float radius, Rgba8 const& color = Rgba8::BLUE_MVTHL,
	int numSlices = 16, int numStacks = 16);

void AddVertsForCone3D(std::vector<Vertex_PCU>& verts,
	Vec3 const& start, Vec3 const& end, float radius,
	Rgba8 const& color = Rgba8::WHITE,
	AABB2 const& UVs = AABB2::ZERO_TO_ONE,
	int numSlices = 8); 

void AddVertsForSquarePyramid(std::vector<Vertex_PCU>& verts,
		Vec3 const& start, Vec3 const& end, float baseLength,
		Rgba8 const& color = Rgba8::WHITE);

// Grid
void AddVertsForPlane3(std::vector<Vertex_PCU>& verts, Plane3 const& plane, 
	float pipeRadius = 0.03f, float centerRadius = 0.12f, float arrowLength = 2.f, float arrowRadius = 0.09f,
	int numX = 15, float distX = 5.f,
	int numY = 15, float distY = 5.f,
	Rgba8 const& arrowColor = Rgba8::CYAN,
	Rgba8 const& colorX = Rgba8::RED,
	Rgba8 const& colorY = Rgba8::GREEN,
	Rgba8 const& lineColor = Rgba8::GRAY_TRANSPARENT,
	Rgba8 const& originColor = Rgba8::GRAY);

// arrow: cylinder + cone
void AddVertsForArrow3D(std::vector<Vertex_PCU>& verts, Vec3 const& start, Vec3 const& end, float radius,
	Rgba8 const& cylinderColor = Rgba8::WHITE,
	Rgba8 const& arrowColor = Rgba8::WHITE, float cylinderFraction = 0.75f);

// Hex
//void AddVertsForEquiangularPolygonFrame(std::vector<Vertex_PCU>& verts, std::vector<unsigned int>& indexes,
//	Vec3 const& center, float circumradius, float innerFrameOffset, int sides = 6,
//	Rgba8 const& color = Rgba8::WHITE); 

// we need to offset outwards and inwards of the halfOffset distance
// starting angle degree is controlling the overall rotation of the hexagon
// is the isInradius is false, then the argument is circumradius
void AddVertsForHexagonFrame(std::vector<Vertex_PCU>& verts, std::vector<unsigned int>& indexes,
		Vec3 const& center, float radius = 1.f,  float halfOffset = 0.1f, float startingAngleDegree = 0.f,
	bool isInradius = true, Rgba8 const& color = Rgba8::WHITE); 
	
void AddVertsForHexagon(std::vector<Vertex_PCU>& verts, std::vector<unsigned int>& indexes,
		Vec3 const& center, float radius = 1.f, float startingAngleDegree = 0.f,
		bool isInradius = true, Rgba8 const& color = Rgba8::WHITE);
	
void AddVertsForHexagonFrame(std::vector<Vertex_PCU>& verts,
		Vec3 const& center, float radius = 1.f, float halfOffset = 0.1f, float startingAngleDegree = 0.f,
		bool isInradius = true, Rgba8 const& color = Rgba8::WHITE);
	
void AddBoundryVerticesForHexagon(Vec3* v, Vec3 const& center, 
	float radius = 1.f, float startingAngleDegree = 0.f, bool isInradius = true);

// for the v, pass in a pointer to a constant array of 6 points in counter clockwise order
bool IsPointInHexagonXY(Vec3 const& p, Vec3 const* v);

void CalculateTangentSpaceBasisVectors(
	std::vector<Vertex_PCUTBN>& vertexes,
	std::vector<unsigned int>& indexes,
	bool computeNormals = true,
	bool computeTangents = true);

// PCUTBN and PCU
Vertex_PCU		GetPCUFromPCUTBN(Vertex_PCUTBN vertex);
Vertex_PCUTBN	GetPCUTBNFromPCU(Vertex_PCU vertex);

// splines
// by changing sectionIndex, we can simulate the line flowing
void	AddVertsForCubicCatmullRomCurve(std::vector<Vertex_PCU>& verts,  CatmullRomSpline3D const spline, float radius = 0.1f, Rgba8 color = Rgba8::GRAY, int numSection = 64, int sectionIndexAddingVerts = 0, bool drawParts = false);
void	AddVertsForCubicBezierCurve(std::vector<Vertex_PCU>& verts,  CubicBezierCurve3D const spline, float radius = 0.1f, Rgba8 color = Rgba8::GRAY, int numSection = 64, int sectionIndexAddingVerts = 0);
void	AddVertsForSamplePointsCurve(std::vector<Vertex_PCU>& verts,  SamplePointsCurve3D const spline, float radius = 0.1f, Rgba8 color = Rgba8::GRAY, bool drawParts = false, int numSection = 64, int sectionIndexAddingVerts = 0);
