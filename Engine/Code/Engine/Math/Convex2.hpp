#pragma once
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Plane2.hpp"
#include <vector>

// because this is a complex struct, let's use class instead
class ConvexPoly2	// easy to do render
{
public:
	ConvexPoly2() = default;
	~ConvexPoly2() = default;

	explicit ConvexPoly2(std::vector<Vec2> convexVertices);

	std::vector<Vec2> GetVertexPos() const;

	void MoveAllVerticesByDisplacement(Vec2 const& disp);
	void RotateAllVerticesByDegrees(float degrees, Vec2 centerPos);
	void ScaleAllVertices(float multiplier, Vec2 centerPos);
	void AddVertexPosition(Vec2 const& disp);

	int GetNumsOfVertices() const;
private:
	// this vector is private because adding new vertices need to determined, not just pushed back
	std::vector<Vec2> m_vertexPos;	// in a counterclockwise order(positive angle direction X->Y)
};

// some numbers of bounding planes that all vertices of the convex is behind of the plane
class ConvexHull2	// easy to do raycast analysis
{
public:

	ConvexHull2() = default;
	~ConvexHull2() = default;

	ConvexHull2(ConvexPoly2 convexPolly);

	void AddPlanes(Plane2 plane);
	bool IsPointInside(Vec2 const& pt);

	std::vector<Plane2> m_boundingPlanes;
};