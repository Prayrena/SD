#pragma once

#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Model/CPUMesh.hpp"
#include "Engine/Model/GPUMesh.hpp"
#include "Game/Entity.hpp"
#include <string>

class Material;

class Model : public Entity
{
friend class Game;

public:
	Model(Game* game);
	virtual ~Model();

	virtual void Update() override;
	virtual void Render() const;

protected:
	bool LoadXml(std::string const& fileName);
	bool LoadObj(std::string const& fileName, Mat44 const& transform = Mat44());

	std::string m_objectFileName;

	CPUMesh*	m_cpuMesh = nullptr;
	GPUMesh*	m_gpuMesh = nullptr;
	Material*	m_material = nullptr;

	void CreateDebugTangentBasisVectors();
	void CreateDebugVertexBuffer();

	std::vector<Vertex_PCU> m_debugVertexes;
	VertexBuffer* m_debugVertexBuffer = nullptr;
};