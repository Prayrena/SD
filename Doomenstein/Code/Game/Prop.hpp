#pragma once
#include "Game/Entity.hpp"
#include <string>

class Texture;

// "public" allow the ptr conversion subclass to parent class
class Prop : public Entity
{
public:
	Prop();
	virtual ~Prop();

	void Update() override;
	void ShutDown() override;
	void Render() const override;

	std::vector<Vertex_PCU> m_vertexes;
	Rgba8					m_color = Rgba8::WHITE;
	Texture*				m_texture = nullptr;

	std::string				m_name;

	std::vector<Vertex_PCUTBN> m_vertexPCUTBNs;
	VertexBuffer* m_vertexBuffer;
	std::vector<unsigned int>  m_indexArray;
	IndexBuffer* m_indexBuffer;
};