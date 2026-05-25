#pragma once
#include "Game/Entity.hpp"
#include <string>

class Texture;

// "public" allow the ptr conversion subclass to parent class
class Prop
{
public:
	Prop();
	~Prop();

	void Update();
	void ShutDown();
	void Render() const;

	Mat44 GetModelMatrix() const;

	Vec3		m_position;
	Vec3		m_velocity;
	EulerAngles m_orientation;
	EulerAngles m_angularVelocity; // Euler angles per second

	std::vector<Vertex_PCU> m_vertexes;
	Rgba8					m_color = Rgba8::WHITE;
	Texture*				m_texture = nullptr;

	std::string				m_name;
};