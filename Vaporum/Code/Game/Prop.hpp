#pragma once
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Renderer/IndexBuffer.hpp"
#include "Game/Entity.hpp"
#include <string>

class Texture;
class Material;

// "public" allow the ptr conversion subclass to parent class
class Prop : public Entity
{
public:
	Prop();
	~Prop();

	void CreateVertexAndIndexBuffer();

	void CreateDebugTangentBasisVectors();
	void CreateDebugVertexBuffer();

	void Update() override;
	void Render() const override;

	void CreateCube();
	void CreateSphere();
	void CreateWorldGrid();

// protected:
	std::vector<Vertex_PCUTBN>	m_PCUTBNVertexes;
	std::vector<unsigned int>	m_indexes;
	VertexBuffer*				m_vertexBuffer = nullptr;
	IndexBuffer*				m_indexBuffer = nullptr;
	Material*					m_material = nullptr;

	std::vector<Vertex_PCU> m_unlitVertexes;
	Texture*				m_unlitTexture = nullptr;
	Rgba8					m_color = Rgba8::WHITE;

	std::vector<Vertex_PCU> m_debugVertexes;
	VertexBuffer*			m_debugVertexBuffer = nullptr;

	std::string				m_name;

	float					m_cubeSize = 2.f;
	float					m_sphereRadius = 1.f;
};