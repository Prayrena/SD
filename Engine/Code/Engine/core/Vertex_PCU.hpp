#pragma once
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/core/Rgba8.hpp"


struct Vertex_PCU
{
public:
	Vertex_PCU() = default;

	Vec3 m_position;
	Rgba8 m_color;
	Vec2 m_uvTexCoords; 

	//explicit Vertex_PCU(Vec3 const& position, Rgba8 const& color, Vec2 const& uvTexCoords);
	explicit Vertex_PCU(float x, float y, float z, 
		unsigned char r, unsigned char g, unsigned char b, unsigned char a);

	explicit Vertex_PCU(Vec3 inputVec3, Rgba8 inputRgba8 = Rgba8::WHITE, Vec2 inputUVTexCoords = Vec2());
};