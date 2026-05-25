#pragma once

struct ID3D11Buffer;
struct ID3D11Device;

class IndexBuffer
{
	// friend class Renderer;

public:// only the renderer class has the right to create and manage this class(this is changed)
	IndexBuffer(size_t size, size_t stride = sizeof(int), ID3D11Device* device = nullptr);
	IndexBuffer(IndexBuffer const& copy) = delete;
	virtual ~IndexBuffer(); // virtual deconstructor will also be triggered when its children deconstructor is deleted

	void Resize(size_t size);

	ID3D11Device* m_device = nullptr;
	ID3D11Buffer* m_buffer = nullptr;
	size_t			m_size = 0;
	unsigned int	m_stride = 0;
};