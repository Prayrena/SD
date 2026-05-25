#include "Engine/core/Vertex_PCU.hpp"

Vertex_PCU::Vertex_PCU(float x, float y, float z, unsigned char r, unsigned char g, unsigned char b, unsigned char a)
	:m_position(x, y, z)
	,m_color(r, g, b, a)
	,m_uvTexCoords(0.f, 0.f)
{

}

Vertex_PCU::Vertex_PCU(Vec3 inputVec3, Rgba8 inputRgba8 /*= Rgba8::WHITE*/, Vec2 inputUVTexCoords /*= Vec2()*/)
	:m_position(inputVec3)
	, m_color(inputRgba8)
	, m_uvTexCoords(inputUVTexCoords)
{

}
