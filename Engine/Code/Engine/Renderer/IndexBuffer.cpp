#include "Engine/Renderer/IndexBuffer.hpp"
#include "Engine/core/EngineCommon.hpp"

#include <d3d11.h>

IndexBuffer::IndexBuffer(size_t size, size_t stride /*= sizeof(int)*/, ID3D11Device* device /*= nullptr*/)
	: m_size(size)
	, m_stride((unsigned int)(stride))
	, m_device(device)
{

}

IndexBuffer::~IndexBuffer()
{
	DX_SAFE_RELEASE(m_buffer);
}

void IndexBuffer::Resize(size_t size)
{
	m_size = size;
}
