#include "Game/GameCommon.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/Renderer.hpp"

extern Renderer* g_theRenderer;

void DebugDrawLine(Vec2 StartPos, Vec2 EndPos, float thickness, Rgba8 const& color)
{
	// the goal is to draw a rectangular which includes two tris
	// the rectangular include two circles which are placed at the startpos and endpos
	// and the radius is the thickness
	Vec2 displacement = EndPos - StartPos;
	Vec2 orientation = displacement.GetNormalized();

	float dotRadius = thickness * .5f;

	Vertex_PCU verts[6] = {};

	//two tris are ABD and CDB
	Vec2 fwd = dotRadius * orientation;
	Vec2 left = fwd.GetRotated90Degrees();
	Vec2 right = fwd.GetRotatedMinus90Degrees();
	Vec2 posA = StartPos + (fwd * -1.f) + right;
	Vec2 posD = StartPos + (fwd * -1.f) + left;
	Vec2 posB = EndPos + (fwd * 1.f) + right;
	Vec2 posC = EndPos + (fwd * 1.f) + left;

	// assign the pos to the vertexes array
	verts[0].m_position = Vec3(posA.x, posA.y, 0.f);
	verts[1].m_position = Vec3(posB.x, posB.y, 0.f);
	verts[2].m_position = Vec3(posD.x, posD.y, 0.f);

	verts[3].m_position = Vec3(posC.x, posC.y, 0.f);
	verts[4].m_position = Vec3(posD.x, posD.y, 0.f);
	verts[5].m_position = Vec3(posB.x, posB.y, 0.f);

	// assign the color towards all the vertexes in the array
	for (int i = 0; i < 6; ++i)
	{
		verts[i].m_color = color;
	}

	g_theRenderer->DrawVertexArray(6, &verts[0]);
}

void DebugDrawRing(Vec2 const& center, float radius, float thickness, Rgba8 const& color)
{
	// the goal is to draw trapezoid for DEBUG_NUM_SIDES times in a circle

	// get the R
	float halfThickness = .5f * thickness;
	float innerRadius = radius - halfThickness;
	float outerRadius = radius + halfThickness;

	//get each pie's degree

	// all the vertexes will be contained in an array
	Vertex_PCU verts[DEBUG_NUM_VERTS] = {};

	// go over each pie of the ring
	for ( int sideIndex = 0; sideIndex < DEBUG_NUM_SIDES; ++sideIndex)
	{
		// get the theta_degrees
		float startDegrees = DEBUG_RING_DEGREES_PERSIDE * static_cast<float>(sideIndex);
		float endDegrees = DEBUG_RING_DEGREES_PERSIDE * static_cast<float>(sideIndex + 1);

		// use polar system( cos radian and sin radian) to get Cartesian system
		float cosStrtPt = CosDegrees(startDegrees);
		float sinStrdPt = SinDegrees(startDegrees);
		float cosEndPt = CosDegrees(endDegrees);
		float sinEndPt = SinDegrees(endDegrees);

		// the local space position of four vertex
		Vec3 innerStrtPos(innerRadius * cosStrtPt,  innerRadius * sinStrdPt, 0.f);
		Vec3 outerStrtPos(outerRadius * cosStrtPt, outerRadius * sinStrdPt, 0.f);
		Vec3 innerEndPos(innerRadius * cosEndPt, innerRadius * sinEndPt, 0.f);
		Vec3 outerEndPos(outerRadius * cosEndPt, outerRadius * sinEndPt, 0.f);

		// transform pos from local to world
		Vec3 centerPos = Vec3(center.x, center.y, 0.f);
		innerStrtPos += centerPos;
		outerStrtPos += centerPos;
		innerEndPos += centerPos;
		outerEndPos += centerPos;

		// trapezoid is made of two tris: tri ABC & tri DEF
		// C and E share the same spot, B and D share the same spot
		// match up the tri vertex with render vert array Index
		int VertIndexA = (sideIndex * 6) + 0;
		int VertIndexB = (sideIndex * 6) + 1;
		int VertIndexC = (sideIndex * 6) + 2;
		int VertIndexD = (sideIndex * 6) + 3;
		int VertIndexE = (sideIndex * 6) + 4;
		int VertIndexF = (sideIndex * 6) + 5;

		// assign the vertex with pos info
		// A is inner start, B is outer Start, C is inner end
		// D is outer end, E is inner end, F is outer start
		verts[VertIndexA].m_position = innerStrtPos;
		verts[VertIndexB].m_position = outerStrtPos;
		verts[VertIndexC].m_position = innerEndPos;
		verts[VertIndexA].m_color = color;
		verts[VertIndexB].m_color = color;
		verts[VertIndexC].m_color = color;


		verts[VertIndexD].m_position = outerEndPos;
		verts[VertIndexE].m_position = innerEndPos;
		verts[VertIndexF].m_position = outerStrtPos;
		verts[VertIndexD].m_color = color;
		verts[VertIndexE].m_color = color;
		verts[VertIndexF].m_color = color;
	}
	g_theRenderer->DrawVertexArray(DEBUG_NUM_VERTS, &verts[0]);
}

void DrawDisk(Vec2 const& center, float radius, Rgba8 const& centerColor, Rgba8 const& edgeColor)
{
	// all the vertexes will be contained in an array
	Vertex_PCU verts[DISK_NUM_VERTS] = {};

	// go over each pie of the ring
	for (int sideIndex = 0; sideIndex < DISK_NUM_SIDES; ++sideIndex)
	{
		// get the theta_degrees
		float startDegrees = DISK_DEGREES_PERSIDE * static_cast<float>(sideIndex);
		float endDegrees   = DISK_DEGREES_PERSIDE * static_cast<float>(sideIndex + 1);

		// use polar system( cos radian and sin radian) to get Cartesian system
		float cosStrtPt = CosDegrees(startDegrees);
		float sinStrdPt = SinDegrees(startDegrees);
		float cosEndPt = CosDegrees(endDegrees);
		float sinEndPt = SinDegrees(endDegrees);

		// the local space position of four vertex
		Vec3 StrtPos(radius * cosStrtPt, radius * sinStrdPt, 0.f);
		Vec3 EndPos(radius * cosEndPt, radius * sinEndPt, 0.f);


		// transform pos from local to world
		Vec3 centerPos = Vec3(center.x, center.y, 0.f);
		StrtPos += centerPos;
		EndPos += centerPos;

		// trapezoid is made of one tri: tri ABC 
		// match up the tri vertex with render vert array Index
		int VertIndexA = (sideIndex * 3) + 0;
		int VertIndexB = (sideIndex * 3) + 1;
		int VertIndexC = (sideIndex * 3) + 2;


		// assign the vertex with pos info
		// A is inner start, B is outer Start, C is inner end
		// D is outer end, E is inner end, F is outer start
		verts[VertIndexA].m_position = centerPos;
		verts[VertIndexB].m_position = StrtPos;
		verts[VertIndexC].m_position = EndPos;
		verts[VertIndexA].m_color = centerColor;
		verts[VertIndexB].m_color = edgeColor;
		verts[VertIndexC].m_color = edgeColor;

	}
	g_theRenderer->DrawVertexArray(DISK_NUM_VERTS, &verts[0]);

}

