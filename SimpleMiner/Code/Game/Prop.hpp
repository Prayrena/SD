#pragma once
#include "Game/Entity.hpp"
#include <string>

class Texture;

// "public" allow the ptr conversion subclass to parent class
class Prop : public Entity
{
public:
	Prop();
	~Prop();

	void Update() override;
	void Render() const override;

	std::vector<Vertex_PCU> m_vertexes;
	Rgba8					m_color = Rgba8::WHITE;
	Texture*				m_texture = nullptr;

	std::string				m_name;
};