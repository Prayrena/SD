#pragma once
#include "Engine/core/Vertex_PCU.hpp"
#include <memory>

struct ID3D11Buffer;
struct ID3D11Device;

class VertexBuffer
{
public:// only the renderer class has the right to create and manage this class(this is changed)
	VertexBuffer(size_t size, size_t stride = sizeof(Vertex_PCU), bool isLinePrimitive = false, ID3D11Device* device = nullptr);
	VertexBuffer(VertexBuffer const& copy) = delete;
	virtual ~VertexBuffer(); // virtual deconstructor will also be triggered when its children deconstructor is deleted

	void Create();
	void Resize(size_t size);

	ID3D11Device*	m_device = nullptr;
	ID3D11Buffer*	m_buffer = nullptr;

	size_t			m_size = 0; // how many vertices are there in the VBO
	unsigned int	m_stride = 0; // how large is one vertex

	bool			m_isLinePrimitive = false;

private:
	// Custom deleter for the unique_ptr
	static void ReleaseBuffer(ID3D11Buffer* pBuffer);

public:
	// std::unique_ptr<ID3D11Buffer, decltype(&ReleaseBuffer)> m_buffer;
};