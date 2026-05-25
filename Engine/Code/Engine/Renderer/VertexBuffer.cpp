#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/core/EngineCommon.hpp"

#include <d3d11.h>

VertexBuffer::VertexBuffer(size_t size, size_t stride /*= sizeof(Vertex_PCU)*/, bool isLinePrimitive /*= false*/, ID3D11Device* device /*= nullptr*/)
	: m_size(size)
	, m_stride((unsigned int)(stride))
	, m_isLinePrimitive(isLinePrimitive)
	, m_device(device)
	// , m_buffer(nullptr, ReleaseBuffer)  // Initialize unique_ptr with nullptr and custom deleter
{
}

VertexBuffer::~VertexBuffer()
{
	DX_SAFE_RELEASE(m_buffer);
	m_buffer = nullptr;
}

void VertexBuffer::Resize(size_t size)
{
	m_size = size;
}

void VertexBuffer::ReleaseBuffer(ID3D11Buffer* pBuffer)
{
	if (pBuffer)
	{
		DX_SAFE_RELEASE(pBuffer);  // Release the D3D buffer when the unique_ptr goes out of scope
	}
}

