#include "Engine/Math/Convex2.hpp"
#include "Engine/Math/MathUtils.hpp"

using namespace std;

ConvexPoly2::ConvexPoly2(std::vector<Vec2> convexVertices)
			: m_vertexPos(convexVertices)
{

}

std::vector<Vec2> ConvexPoly2::GetVertexPos() const
{
	return m_vertexPos;
}

void ConvexPoly2::MoveAllVerticesByDisplacement(Vec2 const& disp)
{
	for (Vec2& p : m_vertexPos)
	{
		p += disp;
	}
}

void ConvexPoly2::RotateAllVerticesByDegrees(float degrees, Vec2 centerPos)
{
	for (Vec2& p : m_vertexPos)
	{
		TransformPositionAroundPosOnXY(p, 1.f, centerPos, degrees);
	}
}

void ConvexPoly2::ScaleAllVertices(float multiplier, Vec2 centerPos)
{
	for (Vec2& p : m_vertexPos)
	{
		TransformPositionAroundPosOnXY(p, multiplier, centerPos);
	}
}

void ConvexPoly2::AddVertexPosition(Vec2 const& disp)
{
	m_vertexPos.push_back(disp);
}

int ConvexPoly2::GetNumsOfVertices() const
{
	return (int)m_vertexPos.size();
}

void ConvexHull2::AddPlanes(Plane2 plane)
{
	m_boundingPlanes.push_back(plane);
}

ConvexHull2::ConvexHull2(ConvexPoly2 convexPolly)
{
	// loop through all the edges to get the 2D planes
	vector<Vec2> const& vertices = convexPolly.GetVertexPos();
	int numVertices = (int)vertices.size();
	for (int i = 0; i < numVertices; ++i)
	{
		Vec2 v0;
		Vec2 v1;

		if (i != numVertices - 1)
		{
			v0 = vertices[i];
			v1 = vertices[i + 1];
		}
		else {
			v0 = vertices[i];
			v1 = vertices[0];
		}

		// get the plane normal
		Vec2 disp = v1 - v0;
		Vec2 normal = disp.GetRotatedMinus90Degrees().GetNormalized();

		// get the shortest dist to the plane
		// for that, we need to get the projected length on normal
		float projectedLen = DotProduct2D(v0, normal);

		Plane2* newPlane = new Plane2(normal, projectedLen);
		m_boundingPlanes.push_back(*newPlane);
	}
}

bool ConvexHull2::IsPointInside(Vec2 const& pt)
{
	for (Plane2 plane : m_boundingPlanes)
	{
		if (plane.IsThePointInFrontOfPlane(pt))
		{
			return false;
		}
		else
		{
			continue;
		}
	}

	return true;
}

