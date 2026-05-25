#include "Engine/core/VertexUtils.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/testing.hpp"
#include "Engine/core/EngineCommon.hpp"


void VertexUtils::TransformVertexArrayXY3D(int numberVerts, Vertex_PCU* verts,
	float scaleXY, float ZrotationDegrees, Vec2 const& translationXY)
{
	for (int vertexIndex = 0; vertexIndex < numberVerts; ++vertexIndex)
	{
		Vec3& pos = verts[vertexIndex].m_position;
		TransformPositionAroundPosOnXY(pos, scaleXY, ZrotationDegrees, translationXY);

	}
}