#include "Engine/Model/GPUMesh.hpp"
#include "Engine/Renderer/Renderer.hpp"

GPUMesh::GPUMesh(CPUMesh* const cpuMesh, Renderer* renderer)
	// : m_renderer(renderer)
{
	// create vertex buffer and index buffer
	m_vertexBuffer = renderer->CreateVertexBuffer((size_t)(cpuMesh->m_vertexes.size()), sizeof(Vertex_PCUTBN));
	m_indexBuffer = renderer->CreateIndexBuffer((size_t)(cpuMesh->m_indexes.size()));

	size_t vertexSize = sizeof(Vertex_PCUTBN);
	size_t vertexArrayDataSize = (cpuMesh->m_vertexes.size()) * vertexSize;
	renderer->CopyCPUToGPU(cpuMesh->m_vertexes.data(), vertexArrayDataSize, m_vertexBuffer);

	size_t indexSize = sizeof(int);
	m_numIndexes = (int)cpuMesh->m_indexes.size();
	size_t indexArrayDataSize = m_numIndexes * indexSize;
	renderer->CopyCPUToGPU(cpuMesh->m_indexes.data(), indexArrayDataSize, m_indexBuffer);
}

GPUMesh::~GPUMesh()
{
	if (m_vertexBuffer)
	{
		delete m_vertexBuffer;
	}

	if (m_indexBuffer)
	{
		delete m_indexBuffer;
	}
}

void GPUMesh::Render(Renderer* const renderer) const
{
	renderer->DrawVertexArrayWithIndexArray(m_vertexBuffer, m_indexBuffer, m_numIndexes);

	renderer->DrawVertexArrayWithIndexArrayToGetPlanarShadow(m_vertexBuffer, m_indexBuffer, m_numIndexes);
}
