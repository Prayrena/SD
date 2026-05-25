#pragma once

#include "Engine/Model/CPUMesh.hpp"
#include "Engine/Renderer/IndexBuffer.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"

class Renderer;

class GPUMesh
{
public:
	GPUMesh();
	GPUMesh(CPUMesh* const cpuMesh, Renderer* renderer); // Create allocates and initializes the vertex and index buffers according to the CPUMesh data
	virtual ~GPUMesh();

	void Create(CPUMesh* const cpuMesh);
	void Render(Renderer* const renderer) const;

protected:
	IndexBuffer* m_indexBuffer = nullptr;
	VertexBuffer* m_vertexBuffer = nullptr;
	unsigned int m_numIndexes = 0;
};