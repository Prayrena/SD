#pragma once
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Texture.hpp"
#include <string>

class Shader;

class Material
{
public:
	Material();
	virtual ~Material();

	bool Load(std::string const& xmlFileName);

	std::string m_name;

	std::string m_shaderName;
	std::string m_vertexTypeName;
	std::string m_diffuseTextureName;
	std::string m_normalTextureName;
	std::string m_specGlossTextureName;

	Shader*		m_shader = nullptr;
	VertexType	m_vertexType = VertexType::Vertex_PCUTBN;
	Texture*	m_diffuseTexture = nullptr;
	Texture*	m_normalTexture = nullptr;
	Texture*	m_specGlossTexture = nullptr;
	Rgba8		m_color = Rgba8::WHITE;
};