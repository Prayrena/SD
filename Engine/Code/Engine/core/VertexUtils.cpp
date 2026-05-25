#include "Engine/core/VertexUtils.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/testing.hpp"
#include "Engine/Renderer/DebugRender.hpp"

using namespace std;

// just a function, not a member function from class
// Process the array and transform the verts from local space to world space
void TransformVertexArrayXY3D(int numberVerts, Vertex_PCU* verts, float scaleXY, float rotationDegreesAboutZ /*= 0.f*/, Vec2 const& translationXY /*= Vec2()*/)
{
	for (int vertexIndex = 0; vertexIndex < numberVerts; ++vertexIndex)
	{
		Vec3& pos = verts[vertexIndex].m_position;
		TransformPositionAroundPosOnXY(pos, scaleXY, rotationDegreesAboutZ, translationXY);
	}
}

void TransformVertexArrayXY3D(int numberVerts, Vertex_PCUTBN* verts, float rotationDegreesAboutZ /*= 0.f*/, Vec2 const& translationXY /*= Vec2()*/, float scaleXY /*= 1.f*/)
{
	for (int vertexIndex = 0; vertexIndex < numberVerts; ++vertexIndex)
	{
		Vec3& pos = verts[vertexIndex].m_position;
		TransformPositionAroundPosOnXY(pos, scaleXY, rotationDegreesAboutZ, translationXY);
	}
}

void TransformVertexArrayOnXY(int startIndex, int numVerts, Vertex_PCU* verts, float scaleXY, Vec2 const& rotationPos, float rotationDegreesAboutPos, Vec2 const& translationXY)
{
	for (int vertexIndex = startIndex; vertexIndex < (startIndex + numVerts); ++vertexIndex)
	{
		Vec3& pos = verts[vertexIndex].m_position;
		TransformPositionAroundPosOnXY(pos, scaleXY, rotationPos, rotationDegreesAboutPos, translationXY);
	}
}

void TransformPointOnXY2D(Vec2& pos, float scaleXY, Vec2 const& rotationPos, float rotationDegreesAboutPos, Vec2 const& translationXY)
{
	Vec3 pos3D(pos);
	TransformPositionAroundPosOnXY(pos3D, scaleXY, rotationPos, rotationDegreesAboutPos, translationXY);
	pos = Vec2(pos3D);
}

void TransformVertexArrayXY3D(std::vector<Vertex_PCU>& Verts, Vec2 const& iBasis, Vec2 const& translation)
{
	Vec2 const& jBasis = iBasis.GetRotated90Degrees();

	for (int vertexIndex = 0; vertexIndex < (int) Verts.size(); ++vertexIndex)
	{
		Vec3& positionToTransform = Verts[vertexIndex].m_position;
		TransformPositionAroundPosOnXY( positionToTransform, iBasis, jBasis, translation );
	}
}
 
void AddVertsForCapsule2D(std::vector<Vertex_PCU>& verts, Capsule2 const& capsule, Rgba8 const& color, int sides /*= 24*/)
{
	AddVertsForLineSegment2D(verts, capsule.m_start, capsule.m_end, capsule.m_radius * 2.f, color);
	Vec2 dispSE = capsule.m_end - capsule.m_start;
	float startdegreesA = 90.f - dispSE.GetOrientationDegrees();
	AddVertesForHalfDisc2D(verts, capsule.m_end, capsule.m_radius, startdegreesA, color, sides);
	Vec2 dispES = capsule.m_start - capsule.m_end;
	float startdegreesB = 90.f - dispES.GetOrientationDegrees();
	AddVertesForHalfDisc2D(verts, capsule.m_start, capsule.m_radius, startdegreesB, color, sides);
}

void AddVertsForCapsule2D(std::vector<Vertex_PCU>& verts, Vec2 const& boneStart, Vec2 const& boneEnd, float radius, Rgba8 const& color)
{
	Capsule2 capsule(boneStart, boneEnd, radius);
	AddVertsForLineSegment2D(verts, capsule.m_start, capsule.m_end, capsule.m_radius * 2.f, color);
	Vec2 dispSE = capsule.m_end - capsule.m_start;
	float startdegreesA = 90.f - dispSE.GetOrientationDegrees();
	AddVertesForHalfDisc2D(verts, capsule.m_end, capsule.m_radius, startdegreesA, color, 24);
	Vec2 dispES = capsule.m_start - capsule.m_end;
	float startdegreesB = 90.f - dispES.GetOrientationDegrees();
	AddVertesForHalfDisc2D(verts, capsule.m_start, capsule.m_radius, startdegreesB, color, 24);
}

void AddVertsForAABB2D(std::vector<Vertex_PCU>& verts, AABB2 const& bounds, Rgba8 const& color)
{
	// this part is delete because it may cause a long verts array to copy and extend just for 6 new slots
	// constexpr int NUM_TRIS = 2;
	// constexpr int NUM_VERTS = NUM_TRIS * 3;
	// verts.reserve( verts.size() + NUM_VERTS );

	Vec3 TL = Vec3(bounds.m_mins.x, bounds.m_maxs.y, 0.f);
	Vec3 TR = Vec3(bounds.m_maxs.x, bounds.m_maxs.y, 0.f);
	Vec3 BL = Vec3(bounds.m_mins.x, bounds.m_mins.y, 0.f);
	Vec3 BR = Vec3(bounds.m_maxs.x, bounds.m_mins.y, 0.f);

	verts.push_back(Vertex_PCU(BL, color, Vec2(0.f, 0.f)));
	verts.push_back(Vertex_PCU(BR, color, Vec2(1.f, 0.f)));
	verts.push_back(Vertex_PCU(TR, color, Vec2(1.f, 1.f)));

	verts.push_back(Vertex_PCU(BL, color, Vec2(0.f, 0.f)));
	verts.push_back(Vertex_PCU(TR, color, Vec2(1.f, 1.f)));
	verts.push_back(Vertex_PCU(TL, color, Vec2(0.f, 1.f)));
}

void AddVertsForAABB2DFrame(std::vector<Vertex_PCU>& verts, AABB2 const& bounds, float thickness, Rgba8 const& color)
{
	Vec3 TL = Vec3(bounds.m_mins.x, bounds.m_maxs.y, 0.f);
	Vec3 TR = Vec3(bounds.m_maxs.x, bounds.m_maxs.y, 0.f);
	Vec3 BL = Vec3(bounds.m_mins.x, bounds.m_mins.y, 0.f);
	Vec3 BR = Vec3(bounds.m_maxs.x, bounds.m_mins.y, 0.f);

	Vec3 FTL = TL + Vec3(thickness * (-1.f), thickness, 0.f);
	Vec3 FTR = TR + Vec3(thickness, thickness, 0.f);
	Vec3 FBL = BL + Vec3(thickness * (-1.f), thickness * (-1.f), 0.f);
	Vec3 FBR = BR + Vec3(thickness, thickness * (-1.f), 0.f);

	verts.push_back(Vertex_PCU(BL, color, Vec2(0.f, 0.f)));
	verts.push_back(Vertex_PCU(FBL, color, Vec2(0.f, 0.f)));
	verts.push_back(Vertex_PCU(BR, color, Vec2(0.f, 0.f)));

	verts.push_back(Vertex_PCU(BR, color, Vec2(0.f, 0.f)));
	verts.push_back(Vertex_PCU(FBL, color, Vec2(0.f, 0.f)));
	verts.push_back(Vertex_PCU(FBR, color, Vec2(0.f, 0.f)));	
	
	verts.push_back(Vertex_PCU(BR, color, Vec2(0.f, 0.f)));
	verts.push_back(Vertex_PCU(FBR, color, Vec2(0.f, 0.f)));
	verts.push_back(Vertex_PCU(TR, color, Vec2(0.f, 0.f)));	
	
	verts.push_back(Vertex_PCU(TR, color, Vec2(0.f, 0.f)));
	verts.push_back(Vertex_PCU(FBR, color, Vec2(0.f, 0.f)));
	verts.push_back(Vertex_PCU(FTR, color, Vec2(0.f, 0.f)));	
	
	verts.push_back(Vertex_PCU(TR, color, Vec2(0.f, 0.f)));
	verts.push_back(Vertex_PCU(FTR, color, Vec2(0.f, 0.f)));
	verts.push_back(Vertex_PCU(TL, color, Vec2(0.f, 0.f)));	
	
	verts.push_back(Vertex_PCU(TL, color, Vec2(0.f, 0.f)));
	verts.push_back(Vertex_PCU(FTR, color, Vec2(0.f, 0.f)));
	verts.push_back(Vertex_PCU(FTL, color, Vec2(0.f, 0.f)));	
	
	verts.push_back(Vertex_PCU(TL, color, Vec2(0.f, 0.f)));
	verts.push_back(Vertex_PCU(FTL, color, Vec2(0.f, 0.f)));
	verts.push_back(Vertex_PCU(BL, color, Vec2(0.f, 0.f)));	
	
	verts.push_back(Vertex_PCU(BL, color, Vec2(0.f, 0.f)));
	verts.push_back(Vertex_PCU(FTL, color, Vec2(0.f, 0.f)));
	verts.push_back(Vertex_PCU(FBL, color, Vec2(0.f, 0.f)));
}

void AddVertsUVForAABB2D(std::vector<Vertex_PCU>& verts, AABB2 const& posBounds, Rgba8 const& spriteTintColor, AABB2 uvBounds)
{
	Vec3 TL = Vec3(posBounds.m_mins.x, posBounds.m_maxs.y, 0.f);
	Vec3 TR = Vec3(posBounds.m_maxs.x, posBounds.m_maxs.y, 0.f);
	Vec3 BL = Vec3(posBounds.m_mins.x, posBounds.m_mins.y, 0.f);
	Vec3 BR = Vec3(posBounds.m_maxs.x, posBounds.m_mins.y, 0.f);

	verts.push_back(Vertex_PCU(BL, spriteTintColor, uvBounds.m_mins));
	verts.push_back(Vertex_PCU(BR, spriteTintColor, Vec2(uvBounds.m_maxs.x, uvBounds.m_mins.y)));
	verts.push_back(Vertex_PCU(TR, spriteTintColor, uvBounds.m_maxs));

	verts.push_back(Vertex_PCU(BL, spriteTintColor, uvBounds.m_mins));
	verts.push_back(Vertex_PCU(TR, spriteTintColor, uvBounds.m_maxs));
	verts.push_back(Vertex_PCU(TL, spriteTintColor, Vec2(uvBounds.m_mins.x, uvBounds.m_maxs.y)));
}

void AddVertsForOBB2D(std::vector<Vertex_PCU>& verts, OBB2 const& box, Rgba8 const& color)
{
	// we don't use std vector because it is used usually when the array's size should be dynamic
	// std::vector<Vec2> vec2s;
	// vec2s.resize(4);
	// Vec3 BL = Vec3(vec2s[0]);
	// Vec3 BR = Vec3(vec2s[1]);
	// Vec3 TL = Vec3(vec2s[2]);
	// Vec3 TR = Vec3(vec2s[3]);
	Vec2 corners[4];
	box.GetCornerPoints(corners);

	Vec3 BL = Vec3(corners[0]);
	Vec3 BR = Vec3(corners[1]);
	Vec3 TL = Vec3(corners[2]);
	Vec3 TR = Vec3(corners[3]);

	verts.push_back(Vertex_PCU(BL, color, Vec2(0.f, 0.f)));
	verts.push_back(Vertex_PCU(BR, color, Vec2(1.f, 0.f)));
	verts.push_back(Vertex_PCU(TR, color, Vec2(1.f, 1.f)));

	verts.push_back(Vertex_PCU(BL, color, Vec2(0.f, 0.f)));
	verts.push_back(Vertex_PCU(TR, color, Vec2(1.f, 1.f)));
	verts.push_back(Vertex_PCU(TL, color, Vec2(0.f, 1.f)));
}

// used for libra laser rendering
void AddVertsForTriangle2D(std::vector<Vertex_PCU>& verts, Vec2 startPos, Rgba8 const& startColor, Vec2 endPos, Rgba8 const& endColor, float halfThicknessOfBottom)
{
	Vec2 direct = (endPos - startPos).GetNormalized();
	Vec2 leftWingDirect = direct.GetRotated90Degrees();
	Vec2 rightWingDirect = direct.GetRotatedDegrees(-90.f);
	Vec2 leftWingPos = startPos + leftWingDirect * halfThicknessOfBottom;
	Vec2 rightWingPos = startPos + rightWingDirect * halfThicknessOfBottom;

	verts.push_back(Vertex_PCU(Vec3(leftWingPos), startColor, Vec2(0.f, 0.f)));
	verts.push_back(Vertex_PCU(Vec3(rightWingPos), startColor, Vec2(1.f, 0.f)));
	verts.push_back(Vertex_PCU(Vec3(endPos), endColor, Vec2(0.5f, 1.f)));
}

void AddVertesForDisc2D(std::vector<Vertex_PCU>& verts, Vec2 const& center, float radius, Rgba8 const& color, int numOfSides)
{
	// calculate how many triangles will be draw
	int numOfTris = numOfSides;
	int numOfVerts = numOfTris * 3;
	verts.reserve(verts.size() + numOfVerts);
	// go over each pie of the ring
	float degreesPerSide = 360.f / static_cast<float>(numOfSides);

	for (int sideIndex = 0; sideIndex < numOfSides; ++sideIndex)
	{
		// get the theta_degrees
		float startDegrees = degreesPerSide * static_cast<float>(sideIndex);
		float endDegrees = degreesPerSide * static_cast<float>(sideIndex + 1);

		// use polar system( cos radian and sin radian) to get Cartesian system
		float cosStrtPt = CosDegrees(startDegrees);
		float sinStrdPt = SinDegrees(startDegrees);
		float cosEndPt = CosDegrees(endDegrees);
		float sinEndPt = SinDegrees(endDegrees);

		// the local space position of four vertex
		Vec3 StartPos(radius * cosStrtPt, radius * sinStrdPt, 0.f);
		Vec3 EndPos(radius * cosEndPt, radius * sinEndPt, 0.f);

		// transform pos from local to world
		Vec3 centerPos = Vec3(center.x, center.y, 0.f);
		StartPos += centerPos;
		EndPos += centerPos;
		
		// add points into the verts
		verts.push_back(Vertex_PCU(centerPos, color, Vec2(0.f, 0.f)));
		verts.push_back(Vertex_PCU(StartPos, color, Vec2(1.f, 0.f)));
		verts.push_back(Vertex_PCU(EndPos, color, Vec2(0.f, 1.f)));
	}
}

void AddVertesForRing2D(std::vector<Vertex_PCU>& verts, Vec2 const& center, float radius, float thickness, Rgba8 const& color, int numOfSides /*= 12*/)
{
	// get the R
	float halfThickness = .5f * thickness;
	float innerRadius = radius - halfThickness;
	float outerRadius = radius + halfThickness;

	// go over each pie of the ring
	for (int sideIndex = 0; sideIndex < numOfSides; ++sideIndex)
	{
		// get the theta_degrees
		float degreesPerside = 360.f / numOfSides;
		float startDegrees = degreesPerside * static_cast<float>(sideIndex);
		float endDegrees = degreesPerside * static_cast<float>(sideIndex + 1);

		// use polar system( cos radian and sin radian) to get Cartesian system
		float cosStrtPt = CosDegrees(startDegrees);
		float sinStrdPt = SinDegrees(startDegrees);
		float cosEndPt = CosDegrees(endDegrees);
		float sinEndPt = SinDegrees(endDegrees);

		// the local space position of four vertex
		Vec3 innerStrtPos(innerRadius * cosStrtPt, innerRadius * sinStrdPt, 0.f);
		Vec3 outerStrtPos(outerRadius * cosStrtPt, outerRadius * sinStrdPt, 0.f);
		Vec3 innerEndPos(innerRadius * cosEndPt, innerRadius * sinEndPt, 0.f);
		Vec3 outerEndPos(outerRadius * cosEndPt, outerRadius * sinEndPt, 0.f);

		// transform pos from local to world
		Vec3 centerPos = Vec3(center.x, center.y, 0.f);
		innerStrtPos += centerPos;
		outerStrtPos += centerPos;
		innerEndPos += centerPos;
		outerEndPos += centerPos;

		// get all six verts
		Vertex_PCU VertA;
		Vertex_PCU VertB;
		Vertex_PCU VertC;
		Vertex_PCU VertD;
		Vertex_PCU VertE;
		Vertex_PCU VertF;

		// assign the vertex with pos info
		// A is inner start, B is outer Start, C is inner end
		// D is outer end, E is inner end, F is outer start
		VertA.m_position = innerStrtPos;
		VertB.m_position = outerStrtPos;
		VertC.m_position = innerEndPos;
		VertA.m_color = color;
		VertB.m_color = color;
		VertC.m_color = color;

		VertD.m_position = outerEndPos;
		VertE.m_position = innerEndPos;
		VertF.m_position = outerStrtPos;
		VertD.m_color = color;
		VertE.m_color = color;
		VertF.m_color = color;

		verts.push_back(VertA);
		verts.push_back(VertB);
		verts.push_back(VertC);

		verts.push_back(VertD);
		verts.push_back(VertE);
		verts.push_back(VertF);
	}
}

void AddVertesForHalfDisc2D(std::vector<Vertex_PCU>& verts, Vec2 const& center, float radius, float startDrawingDegrees, Rgba8 const& color, int numOfSides)
{
	// calculate how many triangles will be draw
	int numOfTris = numOfSides;
	int numOfVerts = numOfTris * 3;
	verts.reserve(verts.size() + numOfVerts);
	// go over each pie of the ring
	float degreesPerSide = 180.f / static_cast<float>(numOfSides);

	for (int sideIndex = 0; sideIndex < numOfSides; ++sideIndex)
	{
		// get the theta_degrees
		float startDegrees = degreesPerSide * static_cast<float>(sideIndex) - startDrawingDegrees;
		float endDegrees = degreesPerSide * static_cast<float>(sideIndex + 1) - startDrawingDegrees;

		// use polar system( cos radian and sin radian) to get Cartesian system
		float cosStrtPt = CosDegrees(startDegrees);
		float sinStrdPt = SinDegrees(startDegrees);
		float cosEndPt = CosDegrees(endDegrees);
		float sinEndPt = SinDegrees(endDegrees);

		// the local space position of four vertex
		Vec3 StartPos(radius * cosStrtPt, radius * sinStrdPt, 0.f);
		Vec3 EndPos(radius * cosEndPt, radius * sinEndPt, 0.f);

		// transform pos from local to world
		Vec3 centerPos = Vec3(center.x, center.y, 0.f);
		StartPos += centerPos;
		EndPos += centerPos;

		// add points into the verts
		verts.push_back(Vertex_PCU(centerPos, color, Vec2(0.f, 0.f)));
		verts.push_back(Vertex_PCU(StartPos, color, Vec2(1.f, 0.f)));
		verts.push_back(Vertex_PCU(EndPos, color, Vec2(0.f, 1.f)));
	}
}

void AddVertesForSector2D(std::vector<Vertex_PCU>& verts, Vec2 const& sectorTip, float sectorForwardDegrees, float sectorApertureDegrees, float sectorRadius, Rgba8 color, int numOfSides)
{
	float startDrawingDegrees = sectorForwardDegrees - sectorApertureDegrees * 0.5f;
	// calculate how many triangles will be draw
	int numOfTris = numOfSides;
	int numOfVerts = numOfTris * 3;
	verts.reserve(verts.size() + numOfVerts);
	// go over each pie of the ring
	float degreesPerSide = sectorApertureDegrees / static_cast<float>(numOfSides);

	for (int sideIndex = 0; sideIndex < numOfSides; ++sideIndex)
	{
		// get the theta_degrees
		float startDegrees = degreesPerSide * static_cast<float>(sideIndex) - startDrawingDegrees;
		float endDegrees = degreesPerSide * static_cast<float>(sideIndex + 1) - startDrawingDegrees;

		// use polar system( cos radian and sin radian) to get Cartesian system
		float cosStrtPt = CosDegrees(startDegrees);
		float sinStrdPt = SinDegrees(startDegrees);
		float cosEndPt = CosDegrees(endDegrees);
		float sinEndPt = SinDegrees(endDegrees);

		// the local space position of four vertex
		Vec3 StartPos(sectorRadius * cosStrtPt, sectorRadius * sinStrdPt, 0.f);
		Vec3 EndPos(sectorRadius * cosEndPt, sectorRadius * sinEndPt, 0.f);

		// transform pos from local to world
		Vec3 centerPos = Vec3(sectorTip.x, sectorTip.y, 0.f);
		StartPos += centerPos;
		EndPos += centerPos;

		// add points into the verts
		verts.push_back(Vertex_PCU(centerPos, color, Vec2(0.f, 0.f)));
		verts.push_back(Vertex_PCU(StartPos, color, Vec2(1.f, 0.f)));
		verts.push_back(Vertex_PCU(EndPos, color, Vec2(0.f, 1.f)));
	}
}

void AddVertesForSector2D(std::vector<Vertex_PCU>& verts, Vec2 const& sectorTip, Vec2 const& sectorForwardNormal, float sectorApertureDegrees, float sectorRadius, Rgba8 color, int numOfSides)
{
	float sectorForwardDegrees = sectorForwardNormal.GetOrientationDegrees();
	AddVertesForSector2D(verts, sectorTip, sectorForwardDegrees, sectorApertureDegrees, sectorRadius, color, numOfSides);
}

// I, J Basis to draw a sector in 3D
void AddVertesForZSector3D(std::vector<Vertex_PCU>& verts, Vec3 const& sectorTip, Vec3 const& sectorForwardNormal, float sectorApertureDegrees, float sectorRadius, Rgba8 color, int numOfSides)
{
	float sectorForwardDegrees = sectorForwardNormal.GetYawDegrees();

	float startDrawingDegrees = sectorForwardDegrees - sectorApertureDegrees * 0.5f;
	// calculate how many triangles will be draw
	int numOfTris = numOfSides;
	int numOfVerts = numOfTris * 3;
	verts.reserve(verts.size() + numOfVerts);
	// go over each pie of the ring
	float degreesPerSide = sectorApertureDegrees / static_cast<float>(numOfSides);

	for (int sideIndex = 0; sideIndex < numOfSides; ++sideIndex)
	{
		// get the theta_degrees
		float startDegrees = degreesPerSide * static_cast<float>(sideIndex) - startDrawingDegrees;
		float endDegrees = degreesPerSide * static_cast<float>(sideIndex + 1) - startDrawingDegrees;

		// use polar system( cos radian and sin radian) to get Cartesian system
		float cosStrtPt = CosDegrees(startDegrees);
		float sinStrdPt = SinDegrees(startDegrees);
		float cosEndPt = CosDegrees(endDegrees);
		float sinEndPt = SinDegrees(endDegrees);

		// the local space position of four vertex
		Vec3 StartPos(sectorRadius * cosStrtPt, sectorRadius * sinStrdPt, sectorTip.z);
		Vec3 EndPos(sectorRadius * cosEndPt, sectorRadius * sinEndPt, sectorTip.z);

		// transform pos from local to world
		Vec3 centerPos = Vec3(sectorTip.x, sectorTip.y, sectorTip.z);
		StartPos += centerPos;
		EndPos += centerPos;

		// add points into the verts
		verts.push_back(Vertex_PCU(centerPos, color, Vec2(0.f, 0.f)));
		verts.push_back(Vertex_PCU(StartPos, color, Vec2(1.f, 0.f)));
		verts.push_back(Vertex_PCU(EndPos, color, Vec2(0.f, 1.f)));
	}
}

void AddVertesForSector3D(std::vector<Vertex_PCU>& verts, Vec3 const& sectorTip, Vec3 const& IBasis, Vec3 const& JBasis, 
							FloatRange rollDegreesRange, float sectorRadius /*= 1.f*/, Rgba8 color /*= Rgba8::GREEN_TRANSPARENT*/, int numOfSides /*= 64*/)
{
	// Convert degrees to radians
	float rollRadian_neg = ConvertDegreesToRadians(rollDegreesRange.m_min);	// - 30.f
	// float rollRadian_pos = ConvertDegreesToRadians(rollDegreesRange.m_max);	// 30.f

	// Calculate the angular step for creating the sector
	float angleStep = ConvertDegreesToRadians(rollDegreesRange.GetRangeLength()) / static_cast<float>(numOfSides);

	// Reserve space for vertices
	verts.reserve(verts.size() + 3 * numOfSides);

	for (int i = 0; i < numOfSides; ++i)
	{
		// Start and end angles for this segment
		float startAngle = rollRadian_neg + i * angleStep;
		float endAngle = startAngle + angleStep;

		// Compute the start and end points of the arc in the plane defined by IBasis and JBasis
		Vec3 startPoint = sectorTip + IBasis * (sectorRadius * cosf(startAngle)) + JBasis * (sectorRadius * sinf(startAngle));
		Vec3 endPoint = sectorTip + IBasis * (sectorRadius * cosf(endAngle)) + JBasis * (sectorRadius * sinf(endAngle));

		// Add vertices for this triangle (tip, start, end)
		verts.push_back(Vertex_PCU(sectorTip, color));
		verts.push_back(Vertex_PCU(startPoint, color));
		verts.push_back(Vertex_PCU(endPoint, color));
	}
}

void AddDebugVertsForJointConstraint(std::vector<Vertex_PCU>& verts, Vec3 const& jointPos, Vec3 const& IBasis, Vec3 const& JBasis, Vec3 const& KBasis, 
										EulerAngles minRotation, EulerAngles maxRotation, 
											Rgba8 wedgeColor, Rgba8 sectorColor, float wedgeRadius /*= 0.1f*/, float sectorRadius /*= 0.1f*/, int numOfSides /*= 64*/)
{
	FloatRange yawDegrees(minRotation.m_yawDegrees, maxRotation.m_yawDegrees);
	FloatRange pitchDegrees(minRotation.m_pitchDegrees, maxRotation.m_pitchDegrees); 
	FloatRange rollDegrees(minRotation.m_rollDegrees, maxRotation.m_rollDegrees); 

	// we add a cone to debug the yaw and pitch, only if both constraint are not zero
	if (yawDegrees.GetRangeLength() != 0.f && pitchDegrees.GetRangeLength())
	{
		// the combination of yaw and pitch defines a sector of a sphere
		// it would be a spherical wedge or cone segment
		// Convert yaw and pitch degrees to radians
		// float yawMin = ConvertDegreesToRadians(yawDegrees.m_min);
		// float yawMax = ConvertDegreesToRadians(yawDegrees.m_max);
		// float pitchMin = ConvertDegreesToRadians(pitchDegrees.m_min);
		// float pitchMax = ConvertDegreesToRadians(pitchDegrees.m_max);		
		
		float yawMin = (yawDegrees.m_min);
		float yawMax = (yawDegrees.m_max);
		float pitchMin = (pitchDegrees.m_min);
		float pitchMax = (pitchDegrees.m_max);

		// Define the angle steps
		float yawStep = (yawMax - yawMin) / numOfSides;
		float pitchStep = (pitchMax - pitchMin) / numOfSides;

		// Loop to create the spherical wedge
		for (int i = 0; i < numOfSides; ++i)
		{
			// float yawStart = yawMin + i * yawStep;
			// float yawEnd = yawStart + yawStep;

			for (int j = 0; j < numOfSides; ++j)
			{
				// float pitchStart = pitchMin + j * pitchStep;
				// float pitchEnd = pitchStart + pitchStep;

				// Compute vertices at the 4 corners of the quad in the spherical wedge
				// Vec3 corner1 = jointPos + (KBasis * cosf(pitchStart) + JBasis * sinf(pitchStart) * cosf(yawStart) + 
				// 							IBasis * sinf(pitchStart) * sinf(yawStart)) * wedgeRadius;
				// 
				// Vec3 corner2 = jointPos + (KBasis * cosf(pitchStart) + JBasis * sinf(pitchStart) * cosf(yawEnd) + 
				// 							IBasis * sinf(pitchStart) * sinf(yawEnd)) * wedgeRadius;
				// 
				// Vec3 corner3 = jointPos + (KBasis * cosf(pitchEnd) + JBasis * sinf(pitchEnd) * cosf(yawStart) + 
				// 							IBasis * sinf(pitchEnd) * sinf(yawStart)) * wedgeRadius;
				// 
				// Vec3 corner4 = jointPos + (KBasis * cosf(pitchEnd) + JBasis * sinf(pitchEnd) * cosf(yawEnd) + 
				// 	IBasis * sinf(pitchEnd) * sinf(yawEnd)) * wedgeRadius; 
					
				// Add two triangles for this quad
				// verts.push_back(Vertex_PCU(corner1, wedgeColor));
				// verts.push_back(Vertex_PCU(corner2, wedgeColor));
				// verts.push_back(Vertex_PCU(corner3, wedgeColor));
				// 
				// verts.push_back(Vertex_PCU(corner3, wedgeColor));
				// verts.push_back(Vertex_PCU(corner2, wedgeColor));
				// verts.push_back(Vertex_PCU(corner4, wedgeColor));

				// Vec3 yawStart_pitchStart = jointPos + (KBasis * cosf(pitchStart) + JBasis * sinf(yawStart)+
				// 		IBasis * sinf(pitchStart) * sinf(yawStart)) * wedgeRadius;
				// 
				// Vec3 yawStart_pitchEnd = jointPos + (KBasis * cosf(pitchEnd) + JBasis * sinf(yawStart) +
				// 	IBasis * sinf(pitchStart) * sinf(yawEnd)) * wedgeRadius;
				// 
				// Vec3 yawEnd_pitchEnd = jointPos + (KBasis * cosf(pitchEnd) + JBasis * sinf(yawEnd)+
				// 	IBasis * sinf(pitchEnd) * sinf(yawStart)) * wedgeRadius;
				// 
				// Vec3 yawEnd_pitchStart = jointPos + (KBasis * cosf(pitchEnd) + JBasis * sinf(yawEnd)+
				// 	IBasis * sinf(pitchEnd) * sinf(yawEnd)) * wedgeRadius;
				// 
				// verts.push_back(Vertex_PCU(yawEnd_pitchStart, wedgeColor));
				// verts.push_back(Vertex_PCU(yawStart_pitchStart, wedgeColor));
				// verts.push_back(Vertex_PCU(yawEnd_pitchEnd, wedgeColor));
				// 
				// verts.push_back(Vertex_PCU(yawStart_pitchStart, wedgeColor));
				// verts.push_back(Vertex_PCU(yawStart_pitchEnd, wedgeColor));
				// verts.push_back(Vertex_PCU(yawEnd_pitchEnd, wedgeColor));

				//----------------------------------------------------------------------------------------------------------------------------------------------------
				Vec3 BL = jointPos + Vec3::MakeFromPolarDegrees((float)(i + 1) * yawStep + yawMin,
					pitchMin + (float)(j + 1) * pitchStep, wedgeRadius);
				Vec3 BR = jointPos + Vec3::MakeFromPolarDegrees((float)i * yawStep + yawMin,
					pitchMin + (float)j * pitchStep, wedgeRadius);
				Vec3 TR = jointPos + Vec3::MakeFromPolarDegrees((float)i * yawStep + yawMin,
					pitchMin + (float)j * pitchStep, wedgeRadius);
				Vec3 TL = jointPos + Vec3::MakeFromPolarDegrees((float)(i + 1) * yawStep + yawMin,
					pitchMin + (float)(j + 1) * pitchStep, wedgeRadius);

				verts.push_back(Vertex_PCU(BL, wedgeColor));
				verts.push_back(Vertex_PCU(BR, wedgeColor));
				verts.push_back(Vertex_PCU(TL, wedgeColor));

				verts.push_back(Vertex_PCU(BR, wedgeColor));
				verts.push_back(Vertex_PCU(TR, wedgeColor));
				verts.push_back(Vertex_PCU(TL, wedgeColor));
			}
		}
	}
	else
	{
		if (yawDegrees.GetRangeLength() != 0.f)			// draw sector to debug yaw
		{
			AddVertesForSector3D(verts, jointPos, IBasis, JBasis, yawDegrees, sectorRadius, wedgeColor, numOfSides);
		}
		else if (pitchDegrees.GetRangeLength() != 0.f)	// draw sector to debug pitch
		{
			
		}
	}
	
	// otherwise, we just add 

	// for debug the roll, we add a sector
	if (rollDegrees.GetRangeLength() != 0.f)
	{ 
		AddVertesForSector3D(verts, jointPos, KBasis * (-1.f), JBasis, rollDegrees, sectorRadius, sectorColor, numOfSides);
	}
}

void AddVertsForLineSegment2D(std::vector<Vertex_PCU>& verts, Vec2 const& start, Vec2 const& end, float thickness, Rgba8 const& color)
{
	//the goal is to draw a rectangular which includes two tris
	Vec2 disp = end - start;
	Vec2 orientation = disp.GetNormalized();

	//two tris are ABD and CDB
	Vec2 left = orientation.GetRotated90Degrees();
	Vec2 right = orientation.GetRotatedMinus90Degrees();

	Vec3 posA = Vec3(start + left * thickness * 0.5f);
	Vec3 posB = Vec3(start + right * thickness * 0.5f);
	Vec3 posC = Vec3(end + left * thickness * 0.5f);
	Vec3 posD = Vec3(end + right * thickness * 0.5f);

	// add points into the verts
	verts.push_back(Vertex_PCU(posA, color, Vec2(0.f, 0.f)));
	verts.push_back(Vertex_PCU(posB, color, Vec2(1.f, 0.f)));
	verts.push_back(Vertex_PCU(posC, color, Vec2(0.f, 1.f)));

	verts.push_back(Vertex_PCU(posD, color, Vec2(1.f, 1.f)));
	verts.push_back(Vertex_PCU(posC, color, Vec2(0.f, 1.f)));
	verts.push_back(Vertex_PCU(posB, color, Vec2(1.f, 0.f)));
}

void AddVertsForLineSegment2D(std::vector<Vertex_PCU>& verts, LineSegment2 const& ls, float thickness, Rgba8 const& color)
{
	AddVertsForLineSegment2D(verts, ls.m_start, ls.m_end, thickness, color);
}

void AddVertsForArrow2D(std::vector<Vertex_PCU>& verts, Vec2 tailPos, Vec2 tipPos, float arrowSize, float lineThickness, Rgba8 const& color)
{
	// the arrow is made of three line segments
	// the bone
	AddVertsForLineSegment2D(verts, tailPos, tipPos, lineThickness, color);

	// calculate two wing pos
	Vec2 iBasis = (tipPos - tailPos).GetNormalized();
	Vec2 jBasis = iBasis.GetRotated90Degrees();
	Vec2 leftWingPos = tipPos - iBasis * arrowSize + jBasis * arrowSize;
	Vec2 rightWingPos = tipPos - iBasis * arrowSize - jBasis * arrowSize;
	AddVertsForLineSegment2D(verts, tipPos, leftWingPos, lineThickness, color);
	AddVertsForLineSegment2D(verts, tipPos, rightWingPos, lineThickness, color);
}

void AddVertsForConvexPoly2(std::vector<Vertex_PCU>& verts, ConvexPoly2 const& convex, Rgba8 const& color /*= Rgba8::WHITE*/)
{
	vector<Vec2> convexVertices = convex.GetVertexPos();
	int numOfTris = ((int)convexVertices.size() - 1);
	Vertex_PCU v_0(Vec3(convexVertices[0], 0.f), color);				// First vertex (fan point)

	for (int triIndex = 1; triIndex < numOfTris; ++triIndex)
	{
		Vertex_PCU v_1(Vec3(convexVertices[triIndex], 0.f), color);		// Current vertex
		Vertex_PCU v_2(Vec3(convexVertices[triIndex + 1], 0.f), color);	// Next vertex

		verts.push_back(v_0);
		verts.push_back(v_1);
		verts.push_back(v_2);
	}
}

void AddVertsForQuad3D(std::vector<Vertex_PCU>& verts, Vec3 const& bottomLeft, Vec3 const& bottomRight, Vec3 const& topRight, Vec3 const& topLeft, 
	Rgba8 const& color /*= Rgba8::WHITE*/, AABB2 const& UVs /*= AABB2::ZERO_TO_ONE*/)
{
	// get all four points
	Vertex_PCU BL(bottomLeft, color, UVs.m_mins);
	Vertex_PCU BR(bottomRight, color, Vec2(UVs.m_maxs.x, UVs.m_mins.y));
	Vertex_PCU TL(topLeft, color, Vec2(UVs.m_mins.x, UVs.m_maxs.y));
	Vertex_PCU TR(topRight, color, UVs.m_maxs);

	// push back two tris into verts
	verts.push_back(BL);
	verts.push_back(BR);
	verts.push_back(TR);

	verts.push_back(BL);
	verts.push_back(TR);
	verts.push_back(TL);
}

void AddVertsForQuad3D(std::vector<Vertex_PCUTBN>& verts, std::vector<unsigned int>& indexes, 
	Vec3 const& bottomLeft, Vec3 const& bottomRight, Vec3 const& topRight, Vec3 const& topLeft, 
	Rgba8 const& color /*= Rgba8::WHITE*/, AABB2 const& UVs /*= AABB2::ZERO_TO_ONE*/)
{
	// calculate the normal of the quad
	Vec3 BR_BL_disp = bottomRight - bottomLeft;
	Vec3 TR_BL_disp = topRight - bottomLeft;
	Vec3 quadNormal = CrossProduct3D(BR_BL_disp, TR_BL_disp).GetNormalized();

	// get all four points
	Vertex_PCUTBN BL(bottomLeft, color, UVs.m_mins, Vec3(), Vec3(), quadNormal);
	Vertex_PCUTBN BR(bottomRight, color, Vec2(UVs.m_maxs.x, UVs.m_mins.y), Vec3(), Vec3(), quadNormal);
	Vertex_PCUTBN TL(topLeft, color, Vec2(UVs.m_mins.x, UVs.m_maxs.y),  Vec3(), Vec3(), quadNormal);
	Vertex_PCUTBN TR(topRight, color, UVs.m_maxs, Vec3(), Vec3(), quadNormal);

	// calculate how many indexes are already exist before this function and take it as offset
	int offset = (int)(verts.size());

	// push back all four points
	verts.push_back(BL);
	verts.push_back(BR);
	verts.push_back(TR);
	verts.push_back(TL);
	
	indexes.push_back(offset + 0);
	indexes.push_back(offset + 1);
	indexes.push_back(offset + 2);

	indexes.push_back(offset + 0);
	indexes.push_back(offset + 2);
	indexes.push_back(offset + 3);

	CalculateTangentSpaceBasisVectors(verts, indexes, true, true);
}

void AddVertsForQuad3DFrame(std::vector<Vertex_PCU>& verts, 
	Vec3 const& bottomLeft, Vec3 const& bottomRight, Vec3 const& topRight, Vec3 const& topLeft, 
	float offsetOutwards /*= 0.1f*/, float offsetInwards /*= 0.1f*/, Rgba8 const& color /*= Rgba8::WHITE*/, AABB2 const& UVs /*= AABB2::ZERO_TO_ONE*/)
{
	Vec3 BL = bottomLeft;
	Vec3 BR = bottomRight;
	Vec3 TL = topLeft;
	Vec3 TR = topRight;

	// for quad line BL-TL
	Vec3 inwards = BR - BL;
	Vec3 outwards = inwards * (-1.f);
	Vec3 stretchOutwards = (TL - BL) * offsetOutwards;
	Vec3 BLTL_BL = BL + offsetOutwards * outwards + stretchOutwards * (-1.f);
	Vec3 BLTL_BR = BL + offsetInwards * inwards + stretchOutwards * (-1.f);
	Vec3 BLTL_TL = TL + offsetOutwards * outwards + stretchOutwards;
	Vec3 BLTL_TR = TL + offsetInwards * inwards + stretchOutwards;
	AddVertsForQuad3D(verts, BLTL_BL, BLTL_BR, BLTL_TR, BLTL_TL, color, UVs);
	
	// for quad line BR-TR
	inwards = BL - BR;
	outwards = inwards * (-1.f);
	Vec3 BRTR_BL = BR + offsetInwards * inwards + stretchOutwards * (-1.f);
	Vec3 BRTR_BR = BR + offsetOutwards * outwards + stretchOutwards * (-1.f);
	Vec3 BRTR_TL = TR + offsetInwards * inwards + stretchOutwards;
	Vec3 BRTR_TR = TR + offsetOutwards * outwards + stretchOutwards;
	AddVertsForQuad3D(verts, BRTR_BL, BRTR_BR, BRTR_TR, BRTR_TL, color, UVs);

	// for quad line BL-BR
	inwards = TL - BL;
	outwards = inwards * (-1.f);
	stretchOutwards = (BR - BL) * offsetOutwards;
	Vec3 BLBR_BL = BL + offsetOutwards * outwards + stretchOutwards * (-1.f);
	Vec3 BLBR_BR = BR + offsetOutwards * outwards + stretchOutwards;
	Vec3 BLBR_TL = BL + offsetInwards * inwards + stretchOutwards * (-1.f);
	Vec3 BLBR_TR = BR + offsetInwards * inwards + stretchOutwards;
	AddVertsForQuad3D(verts, BLBR_BL, BLBR_BR, BLBR_TR, BLBR_TL,color, UVs);


	// for quad line TL-TR
	inwards = BL - TL;
	outwards = inwards * (-1.f);
	Vec3 BLTR_BL = TL + offsetInwards * inwards + stretchOutwards * (-1.f);
	Vec3 BLTR_BR = TR + offsetInwards * inwards + stretchOutwards;
	Vec3 BLTR_TL = TL + offsetOutwards * outwards + stretchOutwards * (-1.f);
	Vec3 BLTR_TR = TR + offsetOutwards * outwards + stretchOutwards;
	AddVertsForQuad3D(verts, BLTR_BL, BLTR_BR, BLTR_TR, BLTR_TL, color, UVs);
}

// should I also use the index list for all the rounded quads?
void AddVertsForRoundedQuad3D(std::vector<Vertex_PCUTBN>& vertexes, Vec3 const& topLeft, Vec3 const& bottomLeft, 
	Vec3 const& bottomRight, Vec3 const& topRight, Rgba8 const& color /*= Rgba8::WHITE*/, AABB2 const& UVs /*= AABB2::ZERO_TO_ONE*/)
{
	// calculate the normal of the quad
	Vec3 BR_BL_disp = bottomRight - bottomLeft;
	Vec3 TR_BL_disp = topRight - bottomLeft;
	Vec3 quadNormal = CrossProduct3D(BR_BL_disp, TR_BL_disp).GetNormalized();
	// get the left and right normal
	Vec3 leftNormal = (topLeft - topRight).GetNormalized();
	Vec3 rightNormal = (topRight - topLeft).GetNormalized();

	// get all four points
	Vertex_PCUTBN BL(bottomLeft, color, UVs.m_mins, Vec3(), Vec3(), leftNormal);
	Vertex_PCUTBN TL(topLeft, color, Vec2(UVs.m_mins.x, UVs.m_maxs.y), Vec3(), Vec3(), leftNormal);

	Vertex_PCUTBN MB(Interpolate(bottomLeft, bottomRight, 0.5f), color, 
						Interpolate(UVs.m_mins, Vec2(UVs.m_maxs.x, UVs.m_mins.y), 0.5f), Vec3(), Vec3(), quadNormal);
	Vertex_PCUTBN MT(Interpolate(topLeft, topRight, 0.5f), color, 
						Interpolate(Vec2(UVs.m_mins.x, UVs.m_maxs.y), UVs.m_maxs, 0.5f), Vec3(), Vec3(), quadNormal);

	Vertex_PCUTBN BR(bottomRight, color, Vec2(UVs.m_maxs.x, UVs.m_mins.y), Vec3(), Vec3(), rightNormal);
	Vertex_PCUTBN TR(topRight, color, UVs.m_maxs, Vec3(), Vec3(), rightNormal);

	// push back four tris into verts
	vertexes.push_back(BL);
	vertexes.push_back(MB);
	vertexes.push_back(TL);

	vertexes.push_back(MB);
	vertexes.push_back(MT);
	vertexes.push_back(TL);

	vertexes.push_back(MB);
	vertexes.push_back(BR);
	vertexes.push_back(MT);

	vertexes.push_back(BR);
	vertexes.push_back(TR);
	vertexes.push_back(MT);
}

void AddVertsForAABB3D(std::vector<Vertex_PCUTBN>& verts, std::vector<unsigned int>& indexes, AABB3 const& bounds,
	Rgba8 const& color /*= Rgba8::WHITE*/, AABB2 const& UVs /*= AABB2::ZERO_TO_ONE*/)
{
	Vec3 BBL;
	Vec3 BBR;
	Vec3 BTR;
	Vec3 BTL;
	Vec3 FBL;
	Vec3 FBR;
	Vec3 FTR;
	Vec3 FTL;
	bounds.GetAllEightPointsOfTheCorners(BBL, BBR, BTR, BTL, FBL, FBR, FTR, FTL);


	// x faces: front and back
	AddVertsForQuad3D(verts, indexes, FBL, FBR, FTR, FTL, color, UVs); // x
	AddVertsForQuad3D(verts, indexes, BBL, BTL, BTR, BBR, color, UVs); // -x
	
	// y faces: left and right
	AddVertsForQuad3D(verts, indexes, FBR, BBR, BTR, FTR, color, UVs); // y
	AddVertsForQuad3D(verts, indexes, BBL, FBL, FTL, BTL, color, UVs); // -y
	
	// z faces: top and bottom faces
	AddVertsForQuad3D(verts, indexes, FTL, FTR, BTR, BTL, color, UVs); // z
	AddVertsForQuad3D(verts, indexes, BBL, BBR, FBR, FBL, color, UVs); // -z
}


void AddVertsForAABB3D(std::vector<Vertex_PCU>& verts, AABB3 const& bounds, Rgba8 const& xFacecolor /*= Rgba8::WHITE*/, Rgba8 const& xOppFacecolor /*= Rgba8::WHITE*/, 
	Rgba8 const& yFacecolor /*= Rgba8::WHITE*/, Rgba8 const& yOppFacecolor /*= Rgba8::WHITE*/, 
	Rgba8 const& zFacecolor /*= Rgba8::WHITE*/, Rgba8 const& zOppFacecolor /*= Rgba8::WHITE*/, 
	AABB2 const& UVs /*= AABB2::ZERO_TO_ONE*/)
{
	Vec3 max = bounds.m_maxs;
	Vec3 min = bounds.m_mins;

	Vec3 FBL = Vec3(max.x, min.y, min.z);
	Vec3 FBR = Vec3(max.x, max.y, min.z);
	Vec3 FTL = Vec3(max.x, min.y, max.z);
	Vec3 FTR = Vec3(max.x, max.y, max.z);

	Vec3 BBL = Vec3(min.x, min.y, min.z);
	Vec3 BBR = Vec3(min.x, max.y, min.z);
	Vec3 BTL = Vec3(min.x, min.y, max.z);
	Vec3 BTR = Vec3(min.x, max.y, max.z);

	// x faces: front and back
	AddVertsForQuad3D(verts, FBL, FBR, FTR, FTL, xFacecolor, UVs); // x
	AddVertsForQuad3D(verts, BBR, BBL, BTL, BTR, xOppFacecolor, UVs); // -x

	// y faces: left and right
	AddVertsForQuad3D(verts, FBR, BBR, BTR, FTR, yFacecolor, UVs); // y
	AddVertsForQuad3D(verts, BBL, FBL, FTL, BTL, yOppFacecolor, UVs); // -y

	// z faces: top and bottom faces
	AddVertsForQuad3D(verts, FTL, FTR, BTR, BTL, zFacecolor, UVs); // z
	AddVertsForQuad3D(verts, BBL, BBR, FBR, FBL, zOppFacecolor, UVs); // -z
}


void AddVertsForAABB3D(std::vector<Vertex_PCU>& verts, AABB3 const& bounds, Rgba8 const& facecolor /*= Rgba8::WHITE*/, AABB2 const& UVs /*= AABB2::ZERO_TO_ONE*/)
{
	AddVertsForAABB3D(verts, bounds, facecolor, facecolor, facecolor, facecolor, facecolor, facecolor, UVs);
}

void AddVertsForAABB3Frame(std::vector<Vertex_PCU>& verts, AABB3 const& bounds, float framePipeRadius /*= 0.05f*/, Rgba8 const& facecolor /*= Rgba8::WHITE*/, AABB2 const& UVs /*= AABB2::ZERO_TO_ONE*/)
{
	Vec3 BBL;
	Vec3 BBR;
	Vec3 BTR;
	Vec3 BTL;
	Vec3 FBL;
	Vec3 FBR;
	Vec3 FTR;
	Vec3 FTL;
	bounds.GetAllEightPointsOfTheCorners(BBL, BBR, BTR, BTL, FBL, FBR, FTR, FTL);

	Vec3 addOn = Vec3(framePipeRadius, framePipeRadius, framePipeRadius);

	AddVertsForAABB3D(verts, AABB3((BBL - addOn), FBL + addOn), facecolor, UVs);
	AddVertsForAABB3D(verts, AABB3((FBL - addOn), FBR + addOn), facecolor, UVs);
	AddVertsForAABB3D(verts, AABB3((FBR - addOn), BBR + addOn), facecolor, UVs);
	AddVertsForAABB3D(verts, AABB3((BBR - addOn), BBL + addOn), facecolor, UVs);

	AddVertsForAABB3D(verts, AABB3((BBL - addOn), BTL + addOn), facecolor, UVs);
	AddVertsForAABB3D(verts, AABB3((FBL - addOn), FTL + addOn), facecolor, UVs);
	AddVertsForAABB3D(verts, AABB3((FBR - addOn), FTR + addOn), facecolor, UVs);
	AddVertsForAABB3D(verts, AABB3((BBR - addOn), BTR + addOn), facecolor, UVs);

	AddVertsForAABB3D(verts, AABB3((BTR - addOn), BTL + addOn), facecolor, UVs);
	AddVertsForAABB3D(verts, AABB3((BTL - addOn), FTL + addOn), facecolor, UVs);
	AddVertsForAABB3D(verts, AABB3((FTL - addOn), FTR + addOn), facecolor, UVs);
	AddVertsForAABB3D(verts, AABB3((FTR - addOn), BTR + addOn), facecolor, UVs);
}

void AddVertsForOBB3(std::vector<Vertex_PCU>& verts, OBB3 const& bounds, Rgba8 const& xFacecolor /*= Rgba8::WHITE*/, Rgba8 const& xOppFacecolor /*= Rgba8::WHITE*/, Rgba8 const& yFacecolor /*= Rgba8::WHITE*/, Rgba8 const& yOppFacecolor /*= Rgba8::WHITE*/, Rgba8 const& zFacecolor /*= Rgba8::WHITE*/, Rgba8 const& zOppFacecolor /*= Rgba8::WHITE*/, AABB2 const& UVs /*= AABB2::ZERO_TO_ONE*/)
{
	Vec3 pts[8];
	bounds.GetCornerPoints(pts);

	Vec3& BBL = pts[0];
	Vec3& BBR = pts[1];
	Vec3& BTL = pts[2];
	Vec3& BTR = pts[3];

	Vec3& TBL = pts[4];
	Vec3& TBR = pts[5];
	Vec3& TTL = pts[6];
	Vec3& TTR = pts[7];

	// z faces: top and bottom faces
	AddVertsForQuad3D(verts, BTL, BTR, BBR, BBL, zOppFacecolor, UVs); // -Z
	AddVertsForQuad3D(verts, TBL, TBR, TTR, TTL, zFacecolor, UVs); // Z

	// x faces: front and back
	AddVertsForQuad3D(verts, BBL, BBR, TBR, TBL, xOppFacecolor, UVs); // -X
	AddVertsForQuad3D(verts, BTR, BTL, TTL, TTR, xFacecolor, UVs); // X

	// y faces: left and right
	AddVertsForQuad3D(verts, BTL, BBL, TBL, TTL, yOppFacecolor, UVs); // -Y
	AddVertsForQuad3D(verts, BBR, BTR, TTR, TBR, yFacecolor, UVs); // Y
}

void AddVertsForSphere3D(std::vector<Vertex_PCU>& verts, Vec3 const& center, float radius, Rgba8 const& color /*= Rgba8::WHITE*/, 
	AABB2 const& UVs /*= AABB2::ZERO_TO_ONE*/, int numSlices /*= 32*/, int numStacks /*= 16*/)
{
	float stepOfLatitude = 180.f / (float)numStacks;
	float stepOfLongtitude = 360.f / (float)numSlices;

	Vec2 UVdimensions = UVs.GetDimensions();

	float stepOfU_EachSlice = UVdimensions.x / (float)numSlices;
	float stepOfV_EachStack = UVdimensions.y / (float)numStacks;

	// we are drawing from the bottom to the top
	// so the start of the longitude is 90
	// and the end of the longitude is -90
	for (int stackIndex = 0; stackIndex < (numStacks); ++stackIndex)
	{
		for (int sliceIndex = 0; sliceIndex < (numSlices); ++sliceIndex)
		{
			Vec3 BL = center + Vec3::MakeFromPolarDegrees((float)sliceIndex * stepOfLongtitude, 
															90.f - (float)stackIndex * stepOfLatitude, radius);
			Vec3 BR = center + Vec3::MakeFromPolarDegrees((float)(sliceIndex + 1) * stepOfLongtitude, 
															90.f - (float)stackIndex * stepOfLatitude, radius);
			Vec3 TL = center + Vec3::MakeFromPolarDegrees((float)sliceIndex * stepOfLongtitude, 
															90.f - (float)(stackIndex + 1) * stepOfLatitude, radius);
			Vec3 TR = center + Vec3::MakeFromPolarDegrees((float)(sliceIndex + 1) * stepOfLongtitude, 
															90.f - (float)(stackIndex + 1)* stepOfLatitude, radius);

			Vec2 BL_UV = UVs.m_mins + Vec2((stepOfU_EachSlice * sliceIndex), (stepOfV_EachStack * stackIndex));
			Vec2 BR_UV = BL_UV + Vec2(stepOfU_EachSlice, 0.f);
			Vec2 TL_UV = BL_UV + Vec2(0.f, stepOfV_EachStack);
			Vec2 TR_UV = BL_UV + Vec2(stepOfU_EachSlice, stepOfV_EachStack);

			verts.push_back(Vertex_PCU(BL, color, BL_UV));
			verts.push_back(Vertex_PCU(BR, color, BR_UV));
			verts.push_back(Vertex_PCU(TR, color, TR_UV));

			verts.push_back(Vertex_PCU(BL, color, BL_UV));
			verts.push_back(Vertex_PCU(TR, color, TR_UV));
			verts.push_back(Vertex_PCU(TL, color, TL_UV));
		}
	}
}

void AddVertsForSphere3D(std::vector<Vertex_PCUTBN>& verts, std::vector<unsigned int>& indexes, Vec3 const& center, float radius, 
		Rgba8 const& color /*= Rgba8::WHITE*/, AABB2 const& UVs /*= AABB2::ZERO_TO_ONE*/, int numSlices /*= 16*/, int numStacks /*= 8*/)
{
	unsigned int numVertexes = (unsigned int)indexes.size();

	// Add one vertex at the very bottom pole of the sphere
	verts.push_back(Vertex_PCUTBN(center - Vec3(0.f, 0.f, radius), color, Vec2()));

	float stepOfLatitude = 180.f / (float)numStacks;
	float stepOfLongtitude = 360.f / (float)numSlices;

	Vec2 UVdimensions = UVs.GetDimensions();

	float stepOfU_EachSlice = UVdimensions.x / (float)numSlices;
	float stepOfV_EachStack = UVdimensions.y / (float)numStacks;


	for (int stackIndex = 1; stackIndex < (numStacks); ++stackIndex) // For each stack, excluding the first and last
	{
		for (int sliceIndex = 0; sliceIndex < (numSlices + 1); ++sliceIndex)
		{
			if (sliceIndex == numSlices) // the same as SliceIndex = 0
			{
				Vec3 BL = center + Vec3::MakeFromPolarDegrees((float)0 * stepOfLongtitude,
					90.f - (float)stackIndex * stepOfLatitude, radius);

				Vec2 BL_UV = UVs.m_mins + Vec2(1.f, (stepOfV_EachStack * stackIndex));

				verts.push_back(Vertex_PCUTBN(BL, color, BL_UV));
			}
			else
			{
				Vec3 BL = center + Vec3::MakeFromPolarDegrees((float)sliceIndex * stepOfLongtitude,
					90.f - (float)stackIndex * stepOfLatitude, radius);

				Vec2 BL_UV = UVs.m_mins + Vec2((stepOfU_EachSlice * sliceIndex), (stepOfV_EachStack * stackIndex));

				verts.push_back(Vertex_PCUTBN(BL, color, BL_UV));
			}
		}
	}

	// Add one vertex at the top pole of the sphere
	verts.push_back(Vertex_PCUTBN(center + Vec3(0.f, 0.f, radius), color, Vec2(1.f, 1.f)));

	// indexes
	// If this is the bottom stack, Add one triangle
	for (int sliceIndex = 0; sliceIndex < (numSlices); ++sliceIndex)
	{
		indexes.push_back(numVertexes);
		indexes.push_back(numVertexes + sliceIndex + 2);
		indexes.push_back(numVertexes + sliceIndex + 1);
	}

	// If this is a middle stack
	// Add two triangles for the quad
	for (int stackIndex = 1; stackIndex < (numStacks - 1); ++stackIndex)
	{
		for (int sliceIndex = 0; sliceIndex < (numSlices); ++sliceIndex)
		{
			int BL = (stackIndex - 1) * (numSlices + 1) + sliceIndex + 1;
			int BR = BL + 1;
			int TR = BL + 1 + numSlices + 1;
			int TL = BL + numSlices + 1;

			indexes.push_back(numVertexes + BL);
			indexes.push_back(numVertexes + BR);
			indexes.push_back(numVertexes + TR);

			indexes.push_back(numVertexes + BL);
			indexes.push_back(numVertexes + TR);
			indexes.push_back(numVertexes + TL);
		}
	}

	// Add indices for the top cap
	int topPoleIndex = (unsigned int)verts.size() - 1;
	int baseIndex = numVertexes + (numStacks - 2) * (numSlices + 1) + 1;

	for (int sliceIndex = 0; sliceIndex < (numSlices); ++sliceIndex)
	{
		indexes.push_back(baseIndex + sliceIndex);
		indexes.push_back(baseIndex + sliceIndex + 1);

		indexes.push_back(topPoleIndex);
	}
}

void AddVertsForCylinder3D(std::vector<Vertex_PCU>& verts, Vec3 const& start, Vec3 const& end, float radius, 
	Rgba8 const& color /*= Rgba8::WHITE*/, AABB2 const& UVs /*= AABB2::ZERO_TO_ONE*/, int numSlices /*= 8*/)
{
	// construct everything in the cylinder space first
	// as the start is (0,0,0), end direction is in local x direction
	// Get bottom center and top center
	Vec3 BC(0.f, 0.f, 0.f);
	Vec3 worldZ = Vec3(0.f, 0.f, 1.f);
	Vec3 worldY = Vec3(0.f, 1.f, 0.f);
	Vec3 disp = end - start;
	float height = disp.GetLength();
	// add the height towards the forwards direction - x
	Vec3 TC = BC + Vec3(height, 0.f, 0.f);

	// loop through each slice, create four tris for each slice
	float stepDegree = 360.f / numSlices;
	float step_U = (UVs.m_maxs.x - UVs.m_mins.x) / numSlices;
	// float step_V = UVs.m_maxs.y - UVs.m_mins.y;

	std::vector<Vertex_PCU> tempVerts;

	for (int i = 0; i < numSlices; ++i)
	{
		float startDegree = i * stepDegree;
		float endDegree = (i + 1) * stepDegree;

		float startRadian = ConvertDegreesToRadians(startDegree);
		float endRadian = ConvertDegreesToRadians(endDegree);
		
		// calculate to start and end disp to get all points
		Vec3 disp_BC_BS = Vec3(0.f, (radius * cosf(startRadian)), (radius * sinf(startRadian))); // in the cylinder space, our disp is y and z
		Vec3 BS = disp_BC_BS + BC;
		Vec3 TS = disp_BC_BS + TC;
		Vec3 disp_BC_BE = Vec3(0.f, (radius * cosf(endRadian)), (radius * sinf(endRadian)));
		Vec3 BE = disp_BC_BE + BC;
		Vec3 TE = disp_BC_BE + TC;

		// calculate UV for two times: two bases surfaces and one side surface
		// first we calculate for the top and base surface
		Vec2 BC_UV(0.5f, 0.5f);
		Vec2 TC_UV(0.5f, 0.5f);
		Vec2 BS_UV;
		Vec2 BE_UV;

		BS_UV.x = RangeMapClamped((cosf(startRadian) * -1.f), -1.f, 1.f, 0.f, 1.f);
		BS_UV.y = RangeMapClamped(sinf(startRadian), -1.f, 1.f, 0.f, 1.f);

		BE_UV.x = RangeMapClamped((cosf(endRadian) * -1.f), -1.f, 1.f, 0.f, 1.f);
		BE_UV.y = RangeMapClamped(sinf(endRadian), -1.f, 1.f, 0.f, 1.f);

		Vec2 TS_UV;
		Vec2 TE_UV;

		TS_UV.x = RangeMapClamped(cosf(startRadian), -1.f, 1.f, 0.f, 1.f);
		TS_UV.y = RangeMapClamped(sinf(startRadian), -1.f, 1.f, 0.f, 1.f);

		TE_UV.x = RangeMapClamped(cosf(endRadian), -1.f, 1.f, 0.f, 1.f);
		TE_UV.y = RangeMapClamped(sinf(endRadian), -1.f, 1.f, 0.f, 1.f);

		Vertex_PCU BC_PCU(BC, color, BC_UV);
		Vertex_PCU BS_PCU(BS, color, BS_UV);
		Vertex_PCU BE_PCU(BE, color, BE_UV);

		Vertex_PCU TC_PCU(TC, color, TC_UV);
		Vertex_PCU TS_PCU(TS, color, TS_UV);
		Vertex_PCU TE_PCU(TE, color, TE_UV);

		tempVerts.push_back(BC_PCU);
		tempVerts.push_back(BE_PCU);
		tempVerts.push_back(BS_PCU);

		tempVerts.push_back(TC_PCU);
		tempVerts.push_back(TS_PCU);
		tempVerts.push_back(TE_PCU);

		//----------------------------------------------------------------------------------------------------------------------------------------------------
		// then we calculate UV for the side surface
		BS_PCU.m_uvTexCoords.x = UVs.m_mins.x + step_U * i;
		BS_PCU.m_uvTexCoords.y = UVs.m_mins.y;
		BE_PCU.m_uvTexCoords.x = UVs.m_mins.x + step_U * (i + 1);
		BE_PCU.m_uvTexCoords.y = UVs.m_mins.y;

		TS_PCU.m_uvTexCoords.x = BS_PCU.m_uvTexCoords.x;
		TS_PCU.m_uvTexCoords.y = UVs.m_maxs.y;
		TE_PCU.m_uvTexCoords.x = BE_PCU.m_uvTexCoords.x;
		TE_PCU.m_uvTexCoords.y = UVs.m_maxs.y;

		tempVerts.push_back(BS_PCU);
		tempVerts.push_back(BE_PCU);
		tempVerts.push_back(TE_PCU);

		tempVerts.push_back(BS_PCU);
		tempVerts.push_back(TE_PCU);
		tempVerts.push_back(TS_PCU);
	}

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	Vec3 dist = end - start;
	Vec3 Ic = dist.GetNormalized();
	Vec3 Jc;
	Vec3 Kc;
	if (fabs(DotProduct3D(Ic, worldZ)) != 1.f)
	{
		Jc = CrossProduct3D(worldZ, Ic).GetNormalized();
		Kc = CrossProduct3D(Ic, Jc);
	}
	else
	{
		Kc = CrossProduct3D(Ic, worldY).GetNormalized();
		Jc = CrossProduct3D(Kc, Ic);
	}
	Mat44 transformMat(Ic, Jc, Kc, start);
	TransformVertexArray3D(tempVerts, transformMat);

	for (int i = 0; i < (int)tempVerts.size(); ++i)
	{
		verts.push_back(tempVerts[i]);
	}
}

// this function is used for add verts for line debug, not triangles for meshes
void AddVertsForPhysXCapsuleDebug(std::vector<Vertex_PCU>& verts, Vec3 const& start, Vec3 const& end, float radius, Rgba8 const& color /*= Rgba8::BLUE_MVTHL*/, int numSlices /*= 8*/, int numStacks /*= 8*/)
{
	// first part is adding points for the cylinder
	// construct everything in the cylinder space first
	// as the start is (0,0,0), end direction is in local x direction
	// Get bottom center and top center
	Vec3 BC(0.f, 0.f, 0.f);
	Vec3 worldZ = Vec3(0.f, 0.f, 1.f);
	Vec3 worldY = Vec3(0.f, 1.f, 0.f);
	Vec3 disp = end - start;
	float height = disp.GetLength();
	// add the height towards the forwards direction - x
	Vec3 TC = BC + Vec3(height, 0.f, 0.f);

	// loop through each slice, create four tris for each slice
	float stepDegree = 360.f / numSlices;

	std::vector<Vertex_PCU> tempVerts;

	for (int i = 0; i < numSlices; ++i)
	{
		// each time we just add two points for debug line
		float startDegree = i * stepDegree;
		// float endDegree = (i + 1) * stepDegree;

		float startRadian = ConvertDegreesToRadians(startDegree);
		// float endRadian = ConvertDegreesToRadians(endDegree);

		// calculate to start and end disp to get all points
		Vec3 disp_BC_BS = Vec3(0.f, (radius * cosf(startRadian)), (radius * sinf(startRadian))); // in the cylinder space, our disp is y and z
		Vec3 BS = disp_BC_BS + BC;
		Vec3 TS = disp_BC_BS + TC;

		// calculate UV for two times: two bases surfaces and one side surface
		// first we calculate for the top and base surface
		Vec2 BS_UV;
		Vec2 TS_UV;

		Vertex_PCU BS_PCU(BS, color, BS_UV);
		Vertex_PCU TS_PCU(TS, color, TS_UV);

		tempVerts.push_back(BS_PCU);
		tempVerts.push_back(TS_PCU);
	}

	// second is adding points for sphere
	// we need to add points for the top and bottom hemi-sphere
	Vec3 topCenter = TC;
	Vec3 bottomCenter = BC;
	float stepOfLatitude = 360.f / (float)numStacks;
	float stepOfLongtitude = 360.f / (float)numSlices;

	// we are drawing from the bottom to the top
	// so the start of the longitude is 90
	// and the end of the longitude is -90
	int quarterSlice = (numSlices/4);
	// float quarter = (numSlices/4);
	for (int stackIndex = 0; stackIndex < numStacks>>1; ++stackIndex)
	{
		for (int sliceIndex = -quarterSlice; sliceIndex <= quarterSlice; ++sliceIndex)
		{
			Vec3 BL = topCenter + Vec3::MakeFromPolarDegrees((float)sliceIndex * stepOfLongtitude,
				90.f - (float)stackIndex * stepOfLatitude, radius);
			Vec3 BR = topCenter + Vec3::MakeFromPolarDegrees((float)(sliceIndex + 1) * stepOfLongtitude,
				90.f - (float)stackIndex * stepOfLatitude, radius);
			Vec3 TL = topCenter + Vec3::MakeFromPolarDegrees((float)sliceIndex * stepOfLongtitude,
				90.f - (float)(stackIndex + 1) * stepOfLatitude, radius);

			Vec2 BL_UV;
			Vec2 TL_UV;

			Vec2 BR_UV;

			tempVerts.push_back(Vertex_PCU(BL, color, BL_UV));
			tempVerts.push_back(Vertex_PCU(TL, color, TL_UV));

			if (sliceIndex != quarterSlice)
			{
				tempVerts.push_back(Vertex_PCU(BL, color, BL_UV));
				tempVerts.push_back(Vertex_PCU(BR, color, BR_UV));
			}
		}
	}

	for (int stackIndex = 0; stackIndex < numStacks>>1; ++stackIndex)
	{
		for (int sliceIndex = quarterSlice * 3; sliceIndex >= quarterSlice; --sliceIndex)
		{
			Vec3 BL = bottomCenter + Vec3::MakeFromPolarDegrees((float)sliceIndex * stepOfLongtitude,
				90.f - (float)stackIndex * stepOfLatitude, radius);
			Vec3 BR = bottomCenter + Vec3::MakeFromPolarDegrees((float)(sliceIndex - 1) * stepOfLongtitude,
				90.f - (float)stackIndex * stepOfLatitude, radius);
			Vec3 TL = bottomCenter + Vec3::MakeFromPolarDegrees((float)sliceIndex * stepOfLongtitude,
				90.f - (float)(stackIndex + 1) * stepOfLatitude, radius);

			Vec2 BL_UV;
			Vec2 TL_UV;

			Vec2 BR_UV;

			tempVerts.push_back(Vertex_PCU(BL, color, BL_UV));
			tempVerts.push_back(Vertex_PCU(TL, color, TL_UV));

			if (sliceIndex != quarterSlice)
			{
				tempVerts.push_back(Vertex_PCU(BL, color, BL_UV));
				tempVerts.push_back(Vertex_PCU(BR, color, BR_UV));
			}
		}
	}

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// transform the local cylinder aline with the start - end direction
	Vec3 dist = end - start;
	Vec3 Ic = dist.GetNormalized();
	Vec3 Jc;
	Vec3 Kc;
	if (fabs(DotProduct3D(Ic, worldZ)) != 1.f)
	{
		Jc = CrossProduct3D(worldZ, Ic).GetNormalized();
		Kc = CrossProduct3D(Ic, Jc);
	}
	else
	{
		Kc = CrossProduct3D(Ic, worldY).GetNormalized();
		Jc = CrossProduct3D(Kc, Ic);
	}
	Mat44 transformMat(Ic, Jc, Kc, start);
	TransformVertexArray3D(tempVerts, transformMat);

	for (int i = 0; i < (int)tempVerts.size(); ++i)
	{
		verts.push_back(tempVerts[i]);
	}
}

void AddVertsForCone3D(std::vector<Vertex_PCU>& verts, Vec3 const& start, Vec3 const& end, float radius, 
	Rgba8 const& color /*= Rgba8::WHITE*/, AABB2 const& UVs /*= AABB2::ZERO_TO_ONE*/, int numSlices /*= 8*/)
{
	// construct everything in the cylinder space first
	// as the start is (0,0,0), end direction is in local x direction
	// Get bottom center and top center
	Vec3 BC(0.f, 0.f, 0.f);
	Vec3 worldZ = Vec3(0.f, 0.f, 1.f);
	Vec3 worldY = Vec3(0.f, 1.f, 0.f);
	Vec3 disp = end - start;
	float height = disp.GetLength();
	// add the height towards the forwards direction - x
	Vec3 TC = BC + Vec3(height, 0.f, 0.f);

	// loop through each slice, create four tris for each slice
	float stepDegree = 360.f / numSlices;
	float step_U = (UVs.m_maxs.x - UVs.m_mins.x) / numSlices;
	// float step_V = UVs.m_maxs.y - UVs.m_mins.y;

	std::vector<Vertex_PCU> tempVerts;

	for (int i = 0; i < numSlices; ++i)
	{
		float startDegree = i * stepDegree;
		float endDegree = (i + 1) * stepDegree;

		float startRadian = ConvertDegreesToRadians(startDegree);
		float endRadian = ConvertDegreesToRadians(endDegree);

		// calculate to start and end disp to get all points
		Vec3 disp_BC_BS = Vec3(0.f, (radius * cosf(startRadian)), (radius * sinf(startRadian))); // in the cylinder space, our disp is y and z
		Vec3 BS = disp_BC_BS + BC;
		Vec3 disp_BC_BE = Vec3(0.f, (radius * cosf(endRadian)), (radius * sinf(endRadian)));
		Vec3 BE = disp_BC_BE + BC;

		// calculate UV for two times: two bases surfaces and one side surface
		// first we calculate for the base surface
		Vec2 BC_UV(0.5f, 0.5f);
		Vec2 BS_UV;
		Vec2 BE_UV;

		BS_UV.x = RangeMapClamped((cosf(startRadian) * -1.f), -1.f, 1.f, 0.f, 1.f);
		BS_UV.y = RangeMapClamped(sinf(startRadian), -1.f, 1.f, 0.f, 1.f);

		BE_UV.x = RangeMapClamped((cosf(endRadian) * -1.f), -1.f, 1.f, 0.f, 1.f);
		BE_UV.y = RangeMapClamped(sinf(endRadian), -1.f, 1.f, 0.f, 1.f);


		Vertex_PCU BC_PCU(BC, color, BC_UV);
		Vertex_PCU BE_PCU(BE, color, BE_UV);
		Vertex_PCU BS_PCU(BS, color, BS_UV);

		tempVerts.push_back(BC_PCU);
		tempVerts.push_back(BE_PCU);
		tempVerts.push_back(BS_PCU);

		//----------------------------------------------------------------------------------------------------------------------------------------------------
		// then we calculate UV for the side surface
		BS_PCU.m_uvTexCoords.x = UVs.m_mins.x + step_U * i;
		BS_PCU.m_uvTexCoords.y = UVs.m_mins.y;
		BE_PCU.m_uvTexCoords.x = UVs.m_mins.x + step_U * (i + 1);
		BE_PCU.m_uvTexCoords.y = UVs.m_mins.y;

		Vec2 TC_UV((UVs.m_maxs.x - UVs.m_mins.x) * 0.5f + UVs.m_mins.x, UVs.m_maxs.y);
		Vertex_PCU TC_PCU(TC, color, TC_UV);

		tempVerts.push_back(TC_PCU);
		tempVerts.push_back(BS_PCU);
		tempVerts.push_back(BE_PCU);
	}

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	Vec3 dist = end - start;
	Vec3 Ic = dist.GetNormalized();
	Vec3 Jc;
	Vec3 Kc;
	if (fabs(DotProduct3D(Ic, worldZ)) != 1.f)
	{
		Jc = CrossProduct3D(worldZ, Ic).GetNormalized();
		Kc = CrossProduct3D(Ic, Jc);
	}
	else
	{
		Kc = CrossProduct3D(Ic, worldY).GetNormalized();
		Jc = CrossProduct3D(Kc, Ic);
	}
	Mat44 transformMat(Ic, Jc, Kc, start);
	TransformVertexArray3D(tempVerts, transformMat); // if we do transform verts, it may also transfer the verts that are not cone's

	for (int i = 0; i < (int)tempVerts.size(); ++i)
	{
		verts.push_back(tempVerts[i]);
	}
}

void AddVertsForPlane3(std::vector<Vertex_PCU>& verts, Plane3 const& plane, float pipeRadius /*= 0.03f*/, float centerRadius /*= 0.12f*/, 
	float arrowLength /*= 2.f*/, float arrowRadius /*= 0.09f*/,
	int numX /*= 15*/, float distX /*= 5.f*/, int numY /*= 15*/, float distY /*= 5.f*/,
	Rgba8 const& arrowColor /*= Rgba8::CYAN*/, Rgba8 const& colorX /*= Rgba8::RED*/, Rgba8 const& colorY /*= Rgba8::GREEN*/, Rgba8 const& lineColor /*= Rgba8::GRAY_TRANSPARENT*/, Rgba8 const& originColor /*= Rgba8::GRAY*/)
{
	Vec3 planeOrigin = plane.m_normal * plane.m_distAlongNormalFromOrigin;
	AddVertsForSphere3D(verts, planeOrigin, centerRadius, originColor);
	AddVertsForArrow3D(verts, planeOrigin, (planeOrigin + plane.m_normal * arrowLength), arrowRadius, arrowColor, arrowColor);

	Vec3 skyward = Vec3(0.f, 0.f, 1.f);
	// todo: check if the normal is skyward and handle special case
	Vec3 tangent = CrossProduct3D(skyward, plane.m_normal).GetNormalized();
	Vec3 biTangent = CrossProduct3D(tangent, plane.m_normal).GetNormalized();

	float gridHalfLengthX = distX * numX * 0.5f;
	float gridHalfLengthY = distY * numY * 0.5f;

	// GREEN lane
	int yLines = numY;
	for (int i = 0; i < numY; ++i)
	{
		if ((yLines % 2) == 1)// odd
		{
			if (i == (int)((float)(numY - 1) * 0.5))
			{
				Vec3 pipeStart = planeOrigin - gridHalfLengthY * biTangent + ((float)i - ((float)numY - 1.f) * 0.5f) * distY * tangent;
				Vec3 pipeEnd = planeOrigin + gridHalfLengthY * biTangent + ((float)i - ((float)numY - 1.f) * 0.5f) * distY * tangent;

				AddVertsForCylinder3D(verts, pipeStart, pipeEnd, pipeRadius * 2.f, colorY, AABB2::ZERO_TO_ONE, 4);
			}
			else
			{
				Vec3 pipeStart = planeOrigin - gridHalfLengthY * biTangent + ((float)i - ((float)numY - 1.f) * 0.5f) * distY * tangent;
				Vec3 pipeEnd = planeOrigin + gridHalfLengthY * biTangent + ((float)i - ((float)numY - 1.f) * 0.5f) * distY * tangent;

				AddVertsForCylinder3D(verts, pipeStart, pipeEnd, pipeRadius, colorY, AABB2::ZERO_TO_ONE, 4);
			}
		}
		else
		{
			Vec3 pipeStart = planeOrigin - gridHalfLengthY * biTangent + ((float)i - ((float)numY - 1.f) * 0.5f) * distY * tangent;
			Vec3 pipeEnd = planeOrigin + gridHalfLengthY * biTangent + ((float)i - ((float)numY - 1.f) * 0.5f) * distY * tangent;

			AddVertsForCylinder3D(verts, pipeStart, pipeEnd, pipeRadius, colorY, AABB2::ZERO_TO_ONE, 4);
		}
	}
	// RED lane
	int xLines = numX;
	for (int i = 0; i < numX; ++i)
	{
		if ((xLines % 2) == 1)
		{
			if (i == (int)((float)(numX - 1) * 0.5))
			{
				Vec3 pipeStart = planeOrigin - gridHalfLengthX * tangent + ((float)i - ((float)numX - 1.f) * 0.5f) * distX * biTangent;
				Vec3 pipeEnd = planeOrigin + gridHalfLengthX * tangent + ((float)i - ((float)numX - 1.f) * 0.5f) * distX * biTangent;

				AddVertsForCylinder3D(verts, pipeStart, pipeEnd, pipeRadius * 2.f, colorX, AABB2::ZERO_TO_ONE, 4);
			}
			else
			{
				Vec3 pipeStart = planeOrigin - gridHalfLengthX * tangent + ((float)i - ((float)numX - 1.f) * 0.5f) * distX * biTangent;
				Vec3 pipeEnd = planeOrigin + gridHalfLengthX * tangent + ((float)i - ((float)numX - 1.f) * 0.5f) * distX * biTangent;

				AddVertsForCylinder3D(verts, pipeStart, pipeEnd, pipeRadius, colorX, AABB2::ZERO_TO_ONE, 4);
			}
		}
		else
		{
			Vec3 pipeStart = planeOrigin - gridHalfLengthX * tangent + ((float)i - ((float)numX - 1.f) * 0.5f) * distX * biTangent;
			Vec3 pipeEnd = planeOrigin + gridHalfLengthX * tangent + ((float)i - ((float)numX - 1.f) * 0.5f) * distX * biTangent;

			AddVertsForCylinder3D(verts, pipeStart, pipeEnd, pipeRadius, colorX, AABB2::ZERO_TO_ONE, 4);
		}
	}

	// line from origin to plane center to world origin
	AddVertsForCylinder3D(verts, planeOrigin, Vec3(), pipeRadius * 0.5f, lineColor, AABB2::ZERO_TO_ONE, 4);
}

void AddVertsForArrow3D(std::vector<Vertex_PCU>& verts, Vec3 const& start, Vec3 const& end, float radius, 
					Rgba8 const& cylinderColor /*= Rgba8::WHITE*/, Rgba8 const& arrowColor /*= Rgba8::WHITE*/, float cylinderFraction /*= 0.75f*/)
{
	// the length of the arrow is unit 1 in the world
	// first construct a cylinder facing x forward
	Vec3 disp = end - start;
	Vec3 cylinderEnd = start + disp * cylinderFraction;
	AddVertsForCylinder3D(verts, start, cylinderEnd, radius, cylinderColor);

	// cone
	AddVertsForCone3D(verts, cylinderEnd, end, radius * 1.5f, arrowColor);
}

//void AddVertsForEquiangularPolygonFrame(std::vector<Vertex_PCU>& verts, std::vector<unsigned int>& indexes, 
//Vec3 const& center, float circumradius, float innerFrameOffset, int sides /*= 6*/, Rgba8 const& color /*= Rgba8::WHITE*/)
//{
//	
//}

void AddVertsForHexagonFrame(std::vector<Vertex_PCU>& verts, std::vector<unsigned int>& indexes, 
	Vec3 const& center, float radius /*= 1.f*/, float halfOffset /*= 0.1f*/, float startingAngleDegree /*= 0.f*/, 
	bool isInradius /*= true*/, Rgba8 const& color /*= Rgba8::WHITE*/)
{
	// get inRadius and circumRadius
	float circumRadius;
	float inRadius;
	if (isInradius)
	{
		inRadius = radius;
		circumRadius = 2.f * inRadius * oneOverSqrt3; 
	}
	else
	{
		circumRadius = radius;
		inRadius = sqrt3 * 0.5f * circumRadius;
	}

	// get each vertices
	int startIndex = (int)verts.size();
	float angleSpacing = 60.f;
	for (int i = 0; i < 6; ++i)
	{
		// we are trying to get ABCD vertices of the trapezoid
		// and first, we are going to get the EF on the hexagon
		float theta = startingAngleDegree + (i * angleSpacing);

		// hypotenuse is circumRadius
		float E_x = circumRadius * cosf( ConvertDegreesToRadians(theta) );
		float E_y = circumRadius * sinf( ConvertDegreesToRadians(theta) );
		Vec3 E_pos = center + Vec3(E_x, E_y, center.z);
		Vec3 center_E = (E_pos - center).GetNormalized();

		float F_x = circumRadius * cosf( ConvertDegreesToRadians(theta + angleSpacing) );
		float F_y = circumRadius * sinf( ConvertDegreesToRadians(theta + angleSpacing) );
		Vec3 F_pos = center + Vec3(F_x, F_y, center.z);
		Vec3 center_F = (F_pos - center).GetNormalized();

		// there are total 12 vertices need to be pushed into verts
		// each time we just push two
		Vec3 A_pos = center_E * -1.f * halfOffset + E_pos;
		Vec3 B_pos = center_E * halfOffset + E_pos;
		Vertex_PCU A(A_pos, color);
		Vertex_PCU B(B_pos, color);
		verts.push_back(A); // start Index + i * 2 = 0
		verts.push_back(B); // start Index + i * 2 + 1 = 1

		// Vec3 C_pos = center_F * -1.f * halfOffset + F_pos;
		// Vec3 D_pos = center_F * halfOffset + F_pos;
		// Vertex_PCU C(C_pos, color); // start Index + (i + 1) * 2 = 2
		// Vertex_PCU D(D_pos, color); // start Index + (i + 1) * 2 + 1 = 3

		// push 2 tris to the indexes
		if (i != 5)
		{
			indexes.push_back(startIndex + i * 2);			// A
			indexes.push_back(startIndex + i * 2 + 1);		// B
			indexes.push_back(startIndex + (i + 1) * 2);	// C		
		
			indexes.push_back(startIndex + (i + 1) * 2);	// C
			indexes.push_back(startIndex + i * 2 + 1);		// B
			indexes.push_back(startIndex + (i + 1) * 2 + 1);// D
		}
		else
		{
			indexes.push_back(startIndex + i * 2);			// A
			indexes.push_back(startIndex + i * 2 + 1);		// B
			indexes.push_back(startIndex);					// 0

			indexes.push_back(startIndex);					// 0
			indexes.push_back(startIndex + i * 2 + 1);		// C
			indexes.push_back(startIndex + 1);				// 1
		}
	}
}

void AddVertsForHexagonFrame(std::vector<Vertex_PCU>& verts, 
		Vec3 const& center, float radius /*= 1.f*/, float halfOffset /*= 0.1f*/, float startingAngleDegree /*= 0.f*/, bool isInradius /*= true*/, 
		Rgba8 const& color /*= Rgba8::WHITE*/)
{
	// get inRadius and circumRadius
	float circumRadius;
	float inRadius;
	if (isInradius)
	{
		inRadius = radius;
		circumRadius = 2.f * inRadius * oneOverSqrt3;
	}
	else
	{
		circumRadius = radius;
		inRadius = sqrt3 * 0.5f * circumRadius;
	}

	// get each vertices
	float angleSpacing = 60.f;
	for (int i = 0; i < 6; ++i)
	{
		// we are trying to get ABCD vertices of the trapezoid
		// and first, we are going to get the EF on the hexagon
		float theta = startingAngleDegree + (i * angleSpacing);

		// hypotenuse is circumRadius
		float E_x = circumRadius * cosf(ConvertDegreesToRadians(theta));
		float E_y = circumRadius * sinf(ConvertDegreesToRadians(theta));
		Vec3 E_pos = center + Vec3(E_x, E_y, center.z);
		Vec3 center_E = (E_pos - center).GetNormalized();

		float F_x = circumRadius * cosf(ConvertDegreesToRadians(theta + angleSpacing));
		float F_y = circumRadius * sinf(ConvertDegreesToRadians(theta + angleSpacing));
		Vec3 F_pos = center + Vec3(F_x, F_y, center.z);
		Vec3 center_F = (F_pos - center).GetNormalized();

		// there are total 12 vertices need to be pushed into verts
		// each time we just push two
		Vec3 A_pos = center_E * -1.f * halfOffset + E_pos;
		Vec3 B_pos = center_E * halfOffset + E_pos;
		Vertex_PCU A(A_pos, color); // start Index + i * 2 = 0
		Vertex_PCU B(B_pos, color); // start Index + i * 2 + 1 = 1

		Vec3 C_pos = center_F * -1.f * halfOffset + F_pos;
		Vec3 D_pos = center_F * halfOffset + F_pos;
		Vertex_PCU C(C_pos, color); // start Index + (i + 1) * 2 = 2
		Vertex_PCU D(D_pos, color); // start Index + (i + 1) * 2 + 1 = 3

		// push 2 tris to the indexes
		verts.push_back(A);
		verts.push_back(B);
		verts.push_back(C);		
		
		verts.push_back(C);
		verts.push_back(B);
		verts.push_back(D);
	}
}

void AddVertsForHexagon(std::vector<Vertex_PCU>& verts, std::vector<unsigned int>& indexes, 
		Vec3 const& center, float radius /*= 1.f*/, float startingAngleDegree /*= 0.f*/, bool isInradius /*= true*/, 
		Rgba8 const& color /*= Rgba8::WHITE*/)
{
	// get inRadius and circumRadius
	float circumRadius;
	float inRadius;
	if (isInradius)
	{
		inRadius = radius;
		circumRadius = 2.f * inRadius * oneOverSqrt3;
	}
	else
	{
		circumRadius = radius;
		inRadius = sqrt3 * 0.5f * circumRadius;
	}

	// get each vertices
	int startIndex = (int)verts.size();
	float angleSpacing = 60.f;
	// there are total 6 + 1 vertices need to be pushed into verts
	Vertex_PCU C(center, color);
	verts.push_back(C);

	for (int i = 0; i < 6; ++i)
	{
		// we are trying to get ABCD vertices of the trapezoid
		// and first, we are going to get the EF on the hexagon
		float theta = startingAngleDegree + (i * angleSpacing);

		// hypotenuse is circumRadius
		float A_x = circumRadius * cosf(ConvertDegreesToRadians(theta));
		float A_y = circumRadius * sinf(ConvertDegreesToRadians(theta));
		Vec3 A_pos = center + Vec3(A_x, A_y, center.z);

		// float B_x = circumRadius * cosf(ConvertDegreesToRadians(theta + angleSpacing));
		// float B_y = circumRadius * sinf(ConvertDegreesToRadians(theta + angleSpacing));
		// Vec3 B_pos = center + Vec3(B_x, B_y, center.z);

		// each time we just push two
		Vertex_PCU A(A_pos, color);
		// Vertex_PCU B(B_pos, color);
		verts.push_back(A); // start Index + i * 2 + 1 = 0
		// verts.push_back(B); // start Index + i * 2 + 2 = 1

		// push 1 tris to the indexes
		if (i != 5)
		{
			indexes.push_back(startIndex);					// Center		
			indexes.push_back(startIndex + i + 1);		// A
			indexes.push_back(startIndex + i + 2);		// B
		}
		else
		{
			indexes.push_back(startIndex);					// Center	
			indexes.push_back(startIndex + i + 1);		// A
			indexes.push_back(startIndex + 1);				// B
		}
	}
}

void AddBoundryVerticesForHexagon(Vec3* v, Vec3 const& center, float radius /*= 1.f*/, float startingAngleDegree /*= 0.f*/, bool isInradius /*= true*/)
{
	// get inRadius and circumRadius
	float circumRadius;
	float inRadius;
	if (isInradius)
	{
		inRadius = radius;
		circumRadius = 2.f * inRadius * oneOverSqrt3;
	}
	else
	{
		circumRadius = radius;
		inRadius = sqrt3 * 0.5f * circumRadius;
	}

	// get each vertices
	float angleSpacing = 60.f;
	for (int i = 0; i < 6; ++i)
	{
		// we are trying to get ABCD vertices of the trapezoid
		// and first, we are going to get the EF on the hexagon
		float theta = startingAngleDegree + (i * angleSpacing);

		// hypotenuse is circumRadius
		float p_x = circumRadius * cosf(ConvertDegreesToRadians(theta));
		float p_y = circumRadius * sinf(ConvertDegreesToRadians(theta));
		Vec3 p = center + Vec3(p_x, p_y, 0.f);

		v[i] = p;
	}
}

bool IsPointInHexagonXY(Vec3 const& p, Vec3 const* v)
{
	for (int i = 0; i < 6; ++i)
	{
		Vec3 AB;
		if (i != 5)
		{
			AB = v[i + 1] - v[i];
		}
		else
		{
			AB = v[0] - v[5];
		}
		Vec3 AP = p - v[i];

		Vec3 cp = CrossProduct3D(AB, AP);
		if (cp.z > 0)
		{
			continue;
		}
		else
		{
			return false;
		}
	}

	// If cross product z-component is positive for all vertexes, point is inside
	return true;
}

// use the transform matrix to transform all the points into another coordinate system
void TransformVertexArray3D(std::vector<Vertex_PCU>& verts, Mat44 const& transform)
{
	for (int i = 0; i < (int)verts.size(); ++i) 
	{
		verts[i].m_position = transform.TransformPosition3D(verts[i].m_position);
	}
}

void CalculateTangentSpaceBasisVectors(std::vector<Vertex_PCUTBN>& vertexes, std::vector<unsigned int>& indexes, bool computeNormals /*= true*/, bool computeTangents /*= true*/)
{	
	if (!computeNormals && !computeTangents)
	{
		return; // for safety, no need to calculate
	}

	for (size_t triIndex = 0; triIndex < ((int)indexes.size() - 2); triIndex += 3) 
	{
		// If computeNormals is true, calculate normals for all triangle faces but do not average them. Otherwise use the normals supplied with the vertexes
		// Indices for the triangle vertices
		unsigned int i0 = indexes[triIndex];
		unsigned int i1 = indexes[triIndex + 1];
		unsigned int i2 = indexes[triIndex + 2];

		Vertex_PCUTBN& v0 = vertexes[i0];
		Vertex_PCUTBN& v1 = vertexes[i1];
		Vertex_PCUTBN& v2 = vertexes[i2];

		// If computeTangents is false, set the tangent and bitangent vectors to zero and do not calculate them\
		// calculate the normal of the quad
		Vec3 p1p0 = v1.m_position - v0.m_position;
		Vec3 p2p0 = v2.m_position - v0.m_position;
		Vec3 triNormal = CrossProduct3D(p1p0, p2p0).GetNormalized();

		if (computeNormals)
		{
			v0.m_normal += triNormal;
			v1.m_normal += triNormal;
			v2.m_normal += triNormal;
		}

		if (computeTangents)
		{
			// Tangent: The tangent is a vector that points in the direction of the texture's "U" axis on a surface. 
			// It's used in conjunction with normal and bitangent vectors to form a tangent space, which is necessary for techniques like normal mapping in 3D rendering
			// The tangent vector lies on the surface of the mesh and is perpendicular to the normal vector.

			// Bitangent(or Binormal) : The bitangent is a vector perpendicular to both the normal and the tangent.
			// It corresponds to the texture's "V" axis. Together with the tangent and normal, it forms an orthogonal basis for the tangent space.
			
			// Get the UV coordinates
			Vec2 const& uv_0 = v0.m_uvTexCoords;
			Vec2 const& uv_1 = v1.m_uvTexCoords;
			Vec2 const& uv_2 = v2.m_uvTexCoords;

			// Calculate the edges of the triangle in UV space
			float deltaU_0 = uv_1.x - uv_0.x;
			float deltaU_1 = uv_2.x - uv_0.x;
			float deltaV_0 = uv_1.y - uv_0.y;
			float deltaV_1 = uv_2.y - uv_0.y;

			float r = 1.f / (deltaU_0 * deltaV_1 - deltaU_1 * deltaV_0);

			// Get the delta edge length
			Vec3 deltaE_0 = v1.m_position - v0.m_position;
			Vec3 deltaE_1 = v2.m_position - v0.m_position;
			
			// Calculate the tangent and bitangent for all three vertexes
			Vec3 tangent = r * (deltaV_1 * deltaE_0 - deltaV_0 * deltaE_1);
			Vec3 biTangent = r * (deltaU_0 * deltaE_1 - deltaU_1 * deltaE_0);

			// Normalize the tangent and bitangent vectors
			tangent = tangent.GetNormalized();
			biTangent = biTangent.GetNormalized();

			// Apply Gram–Schmidt ortho-normalization keeping N unchanged
			// Subtracting this projection removes any component of the tangent that is parallel to the normal, leaving the tangent orthogonal to the normal
			tangent = (tangent - triNormal * DotProduct3D(triNormal, tangent)).GetNormalized();

			// Recalculate the bitangent as the cross product of the normal and the tangent
			biTangent = CrossProduct3D(triNormal, tangent);

			// Assign the calculated tangent and bitangent to each vertex
			v0.m_tangent += tangent;
			v1.m_tangent += tangent;
			v2.m_tangent += tangent;

			v0.m_bitangent += biTangent;
			v1.m_bitangent += biTangent;
			v2.m_bitangent += biTangent;
		}
	}

	// normalize the normals and tangent and bitangent
	for (int i = 0; i < vertexes.size(); ++i)
	{
		if (computeNormals)
		{
			vertexes[i].m_normal = vertexes[i].m_normal.GetNormalized();
		}
		if (computeTangents)
		{
			vertexes[i].m_tangent = vertexes[i].m_tangent.GetNormalized();
			vertexes[i].m_bitangent = vertexes[i].m_bitangent.GetNormalized();
		}
	}
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
Vertex_PCU GetPCUFromPCUTBN(Vertex_PCUTBN vertex)
{
	return Vertex_PCU(vertex.m_position, vertex.m_color, vertex.m_uvTexCoords);
}

void AddVertsForCubicCatmullRomCurve(std::vector<Vertex_PCU>& verts, CatmullRomSpline3D const spline, float radius /*= 0.1f*/, Rgba8 color /*= Rgba8::GRAY*/, 
																	int numSection /*= 64*/, int sectionIndexAddingVerts /*= 0*/, bool drawParts /*= false*/)
{
	float step = 1.f / numSection;
	int numControlPts = (int)spline.m_positions.size();
	std::vector<Vec3> samplePts;

	for (int i = 0; i < ((numControlPts - 1) * numSection - 1); ++i)
	{
		if (!drawParts)
		{
			float currentFraction = step * i;
			float nextFraction = step * (i + 1);
			Vec3 thisPos = spline.EvaluateAtParametric(currentFraction);
			Vec3 nextPos = spline.EvaluateAtParametric(nextFraction);

			AddVertsForCylinder3D(verts, thisPos, nextPos, radius, color);

		}
		else
		{
			if (i % numSection == sectionIndexAddingVerts)
			{
				float currentFraction = step * i;
				float nextFraction = step * (i + 1);
				Vec3 thisPos = spline.EvaluateAtParametric(currentFraction);
				Vec3 nextPos = spline.EvaluateAtParametric(nextFraction);

				AddVertsForCylinder3D(verts, thisPos, nextPos, radius, color, AABB2::ZERO_TO_ONE, 4);
			}
		}
	}
}

void AddVertsForSamplePointsCurve(std::vector<Vertex_PCU>& verts, SamplePointsCurve3D const spline, float radius /*= 0.1f*/, Rgba8 color /*= Rgba8::GRAY*/, 
									bool drawParts /*= false*/, int numSection /*= 64*/, int sectionIndexAddingVerts /*= 0*/)
{
	for (int segmentIndex = 0; segmentIndex < (int)spline.m_samplePoints.size() - 1; ++segmentIndex)
	{
		if (drawParts)
		{
			if ((segmentIndex % numSection) == sectionIndexAddingVerts)
			{
				Vec3 thisPos = spline.m_samplePoints[segmentIndex];
				Vec3 nextPos = spline.m_samplePoints[segmentIndex + 1];

				AddVertsForCylinder3D(verts, thisPos, nextPos, radius, color, AABB2::ZERO_TO_ONE, 4);
			}
			else
			{
				continue;
			}
		}
		else
		{
			Vec3 thisPos = spline.m_samplePoints[segmentIndex];
			Vec3 nextPos = spline.m_samplePoints[segmentIndex + 1];

			AddVertsForCylinder3D(verts, thisPos, nextPos, radius, color, AABB2::ZERO_TO_ONE, 4);
		}
	}
}

