#pragma once
#include "Engine/core/Vertex_PCU.hpp"

class VertexUtils
{
public:
	VertexUtils() = default;


	// translate, rotate and scale local vertex to world vertex
	void TransformVertexArrayXY3D(int numberVerts, Vertex_PCU* verts,
		float scaleXY, float rotationDegreesAboutZ, Vec2 const& translationXY);


private:

};
